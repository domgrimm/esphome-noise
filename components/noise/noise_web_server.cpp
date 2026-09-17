#include "noise_web_server.h"

#if defined(USE_NETWORK) && !defined(USE_ZEPHYR)

#include "noise_dashboard_index.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/application.h"

#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif
#ifdef USE_WIFI
#define protected public
#define private public
#include "esphome/components/wifi/wifi_component.h"
#undef protected
#undef private
#include "esphome/components/network/ip_address.h"
#endif

namespace esphome::noise {

static const char *const TAG = "noise.web_server";

#ifdef USE_WIFI
static bool s_ap_grace_active = false;
static uint32_t s_ap_grace_start = 0;
static uint32_t s_save_requested_time = 0;
static constexpr uint32_t AP_GRACE_PERIOD_MS = 60000;
#endif

bool NoiseWebHandler::canHandle(AsyncWebServerRequest *request) const {
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
#else
  const auto &url = request->url();
#endif
  auto method = request->method();

  if (url == "/api/noise" || url == "/api/noise/" || url == "/api/wifi_status" || url == "/api/wifi_status/" ||
      url == "/api/close_ap" || url == "/api/close_ap/")
    return (method == HTTP_GET || method == HTTP_POST);

#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) {
    // Intercept /wifisave for custom grace period handling
    if (url == "/wifisave")
      return true;
    // Let native captive portal handle GET /config.json
    if (url == "/config.json")
      return false;
    // Intercept all other GET requests (captive portal probe URLs, root, index.html, etc.)
    if (method == HTTP_GET)
      return true;
  }
#endif

  if (method == HTTP_GET && (url == "/" || url == "/index.html" || url == "/favicon.ico"))
    return true;

  return false;
}

void NoiseWebHandler::handleRequest(AsyncWebServerRequest *request) {
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
#else
  const auto &url = request->url();
#endif

  if (url == "/favicon.ico") {
    request->send(204);
    return;
  }

  if (url == "/api/noise" || url == "/api/noise/") {
    this->handle_api_request_(request);
    return;
  }

  if (url == "/api/wifi_status" || url == "/api/wifi_status/") {
    this->handle_wifi_status_request_(request);
    return;
  }

  if (url == "/api/close_ap" || url == "/api/close_ap/") {
    this->handle_close_ap_request_(request);
    return;
  }

#ifdef USE_CAPTIVE_PORTAL
  if (url == "/wifisave") {
    this->handle_wifisave_request_(request);
    return;
  }
#endif

  this->handle_index_request_(request);
}

void NoiseWebHandler::handle_index_request_(AsyncWebServerRequest *request) {
#if defined(USE_ESP8266) || (defined(USE_ARDUINO) && !defined(USE_ESP32))
  auto *response = request->beginResponse_P(200, "text/html", NOISE_INDEX_HTML_GZ, NOISE_INDEX_HTML_GZ_SIZE);
#else
  auto *response = request->beginResponse(200, "text/html", NOISE_INDEX_HTML_GZ, NOISE_INDEX_HTML_GZ_SIZE);
#endif
  response->addHeader("Content-Encoding", "gzip");
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

void NoiseWebHandler::defer_(std::function<void()> &&f) {
  if (this->parent_ != nullptr) {
    App.scheduler.set_timeout(this->parent_, static_cast<const char *>(nullptr), 0, std::move(f));
  }
}

void NoiseWebHandler::handle_api_request_(AsyncWebServerRequest *request) {
  if (this->parent_ == nullptr) {
    request->send(500, "application/json", "{\"error\":\"noise component not found\"}");
    return;
  }

  if (request->method() == HTTP_POST) {
    if (request->hasArg("action")) {
      std::string action = request->arg("action");
      auto *parent = this->parent_;

      if (action == "play") {
        std::string variant = request->hasArg("variant") ? request->arg("variant") : "";
        optional<float> vol;
        if (request->hasArg("volume")) {
          auto v = parse_number<float>(request->arg("volume"));
          if (v.has_value()) {
            vol = (*v > 1.0f) ? (*v / 100.0f) : *v;
          }
        }
        this->defer_([parent, variant, vol]() { parent->play(variant, 0, vol); });
      } else if (action == "stop") {
        this->defer_([parent]() { parent->stop(); });
      } else if (action == "pause") {
        this->defer_([parent]() { parent->pause(); });
      } else if (action == "resume") {
        this->defer_([parent]() { parent->resume(); });
      } else if (action == "master_volume" || action == "speaker_volume") {
        if (request->hasArg("value")) {
          auto val = parse_number<float>(request->arg("value"));
          if (val.has_value()) {
            float v = (*val > 1.0f) ? (*val / 100.0f) : *val;
            this->defer_([parent, v]() { parent->set_speaker_volume(v); });
          }
        }
      } else if (action == "master_mute" || action == "speaker_mute") {
        this->defer_([parent]() { parent->set_speaker_muted(!parent->is_speaker_muted()); });
      } else if (action == "volume") {
        if (request->hasArg("value")) {
          auto val = parse_number<float>(request->arg("value"));
          if (val.has_value()) {
            float v = (*val > 1.0f) ? (*val / 100.0f) : *val;
            this->defer_([parent, v]() { parent->set_volume(v); });
          }
        }
      } else if (action == "tone") {
        if (request->hasArg("value")) {
          auto val = parse_number<float>(request->arg("value"));
          if (val.has_value()) {
            float t = (*val > 1.0f) ? (*val / 100.0f) : *val;
            this->defer_([parent, t]() { parent->set_tone(t); });
          }
        }
      } else if (action == "sleep_timer") {
        if (request->hasArg("minutes")) {
          auto val = parse_number<float>(request->arg("minutes"));
          if (val.has_value()) {
            float m = *val;
            this->defer_([parent, m]() { parent->set_sleep_timer(m); });
          }
        }
      } else if (action == "mute") {
        this->defer_([parent]() { parent->set_muted(!parent->is_muted()); });
      }
    }
    request->send(200, "application/json", "{\"success\":true}");
    return;
  }

  // GET: Read-only serialization on heap
  std::string buf;
  buf.reserve(512);
  char temp[32];
  buf += "{\"title\":\"";
  buf += this->title_;
  buf += "\",\"show_components\":";
  buf += this->show_components_ ? "true" : "false";
  buf += ",\"running\":";
  buf += this->parent_->is_running() ? "true" : "false";
  buf += ",\"paused\":";
  buf += this->parent_->is_paused() ? "true" : "false";
  buf += ",\"variant\":\"";
  buf += this->parent_->get_last_variant();
  buf += "\",\"volume\":";
  snprintf(temp, sizeof(temp), "%.2f", this->parent_->get_volume());
  buf += temp;
  buf += ",\"master_volume\":";
  snprintf(temp, sizeof(temp), "%.2f", this->parent_->get_speaker_volume());
  buf += temp;
  buf += ",\"has_master_volume\":";
  buf += this->parent_->has_speaker_volume() ? "true" : "false";
  buf += ",\"master_muted\":";
  buf += this->parent_->is_speaker_muted() ? "true" : "false";
  buf += ",\"tone\":";
  snprintf(temp, sizeof(temp), "%.2f", this->parent_->get_tone());
  buf += temp;
  buf += ",\"sleep_timer\":";
  snprintf(temp, sizeof(temp), "%.1f", this->parent_->get_sleep_timer());
  buf += temp;
  buf += ",\"sleep_timer_remaining\":";
  snprintf(temp, sizeof(temp), "%.0f", this->parent_->get_sleep_timer_remaining_sec());
  buf += temp;
  buf += ",\"time_left\":";
  float rem_sec = this->parent_->get_sleep_timer_remaining_sec();
  snprintf(temp, sizeof(temp), "%.0f", rem_sec > 0.0f ? std::ceil(rem_sec / 60.0f) : 0.0f);
  buf += temp;
  buf += ",\"muted\":";
  buf += this->parent_->is_muted() ? "true" : "false";
#ifdef USE_CAPTIVE_PORTAL
  buf += ",\"captive_portal\":";
  buf += (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) ? "true" : "false";
#else
  buf += ",\"captive_portal\":false";
#endif
#ifdef USE_WIFI
  std::string ip_str = "";
  bool is_conn = false;
  if (wifi::global_wifi_component != nullptr) {
    is_conn = wifi::global_wifi_component->is_connected();
    auto ips = wifi::global_wifi_component->wifi_sta_ip_addresses();
    if (!ips.empty()) {
      char ip_buf[network::IP_ADDRESS_BUFFER_SIZE];
      ip_str = ips[0].str_to(ip_buf);
    }
  }
  buf += ",\"wifi_connected\":";
  buf += is_conn ? "true" : "false";
  buf += ",\"ip_address\":\"";
  buf += ip_str;
  buf += "\"";
#else
  buf += ",\"wifi_connected\":false,\"ip_address\":\"\"";
#endif
  buf += ",\"hostname\":\"";
  buf += App.get_name();
  buf += ".local\"";
  buf += "}";

  auto *response = request->beginResponse(200, "application/json", buf.c_str());
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void NoiseWebHandler::handle_wifi_status_request_(AsyncWebServerRequest *request) {
  std::string buf;
  buf.reserve(160);
  std::string ip_str = "";
  bool connected = false;
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr) {
    connected = wifi::global_wifi_component->is_connected();
    auto ips = wifi::global_wifi_component->wifi_sta_ip_addresses();
    if (!ips.empty()) {
      char ip_buf[network::IP_ADDRESS_BUFFER_SIZE];
      ip_str = ips[0].str_to(ip_buf);
    }
  }
#endif
  buf += "{\"connected\":";
  buf += connected ? "true" : "false";
  buf += ",\"ip\":\"";
  buf += ip_str;
  buf += "\",\"hostname\":\"";
  buf += App.get_name();
  buf += ".local\"}";

  auto *response = request->beginResponse(200, "application/json", buf.c_str());
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void NoiseWebHandler::handle_wifisave_request_(AsyncWebServerRequest *request) {
  const auto &ssid = request->arg("ssid");
  const auto &psk = request->arg("psk");
  ESP_LOGI(TAG, "Captive portal saving WiFi credentials: SSID='%s'", ssid.c_str());

#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr) {
    // Keep SoftAP broadcasting during connection attempt so the user can see the assigned IP
    wifi::global_wifi_component->has_ap_ = false;
    s_ap_grace_active = true;
    s_ap_grace_start = 0;
    s_save_requested_time = millis();

    this->defer_([ssid, psk]() {
      wifi::global_wifi_component->save_wifi_sta(ssid.c_str(), psk.c_str());
    });
  }
#endif

  auto *response = request->beginResponse(200, "text/plain", "Saved. Connecting...");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void NoiseWebHandler::handle_close_ap_request_(AsyncWebServerRequest *request) {
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr) {
    s_ap_grace_active = false;
    wifi::global_wifi_component->has_ap_ = true;
#ifdef USE_CAPTIVE_PORTAL
    if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) {
      captive_portal::global_captive_portal->end();
    }
#endif
    wifi::global_wifi_component->wifi_mode_({}, false);
    ESP_LOGI(TAG, "SoftAP closed via user request");
  }
#endif
  auto *response = request->beginResponse(200, "application/json", "{\"closed\":true}");
  response->addHeader("Access-Control-Allow-Origin", "*");
  request->send(response);
}

void NoiseWebServer::loop() {
#if defined(USE_WIFI) && defined(USE_CAPTIVE_PORTAL)
  if (s_ap_grace_active && wifi::global_wifi_component != nullptr) {
    if (wifi::global_wifi_component->is_connected()) {
      if (s_ap_grace_start == 0) {
        s_ap_grace_start = millis();
        ESP_LOGI(TAG, "WiFi connected! Keeping SoftAP active for 60s so user can read assigned IP...");
      } else if (millis() - s_ap_grace_start > AP_GRACE_PERIOD_MS) {
        ESP_LOGI(TAG, "SoftAP grace period (60s) expired. Disabling AP now.");
        s_ap_grace_active = false;
        wifi::global_wifi_component->has_ap_ = true;
        if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active()) {
          captive_portal::global_captive_portal->end();
        }
        wifi::global_wifi_component->wifi_mode_({}, false);
      }
    } else if (s_save_requested_time != 0 && millis() - s_save_requested_time > 35000) {
      ESP_LOGW(TAG, "WiFi STA connection attempt timed out; restoring AP flag");
      s_ap_grace_active = false;
      s_save_requested_time = 0;
      wifi::global_wifi_component->has_ap_ = true;
    }
  }
#endif
}

void NoiseWebServer::setup() {
  if (this->base_ == nullptr) {
    this->base_ = esphome::web_server_base::global_web_server_base;
  }
  if (this->base_ == nullptr) {
    ESP_LOGE(TAG, "No web_server_base configured; cannot start noise web dashboard");
    this->mark_failed();
    return;
  }

  this->base_->init();

  this->handler_ = new NoiseWebHandler(this->parent_, this->title_, this->show_components_);
  this->base_->add_handler(this->handler_);
  ESP_LOGI(TAG, "Registered mobile-first noise web dashboard at / (route hijacked)");
}

void NoiseWebServer::dump_config() {
  ESP_LOGCONFIG(TAG, "Noise Web Server Dashboard:");
  ESP_LOGCONFIG(TAG, "  Title: %s", this->title_.c_str());
  ESP_LOGCONFIG(TAG, "  Show Components: %s", YESNO(this->show_components_));
  ESP_LOGCONFIG(TAG, "  Route: / (hijacked, webserver-listcomponents compatible)");
}

}  // namespace esphome::noise

#endif

#include "noise_web_server.h"

#if defined(USE_NETWORK) && !defined(USE_ZEPHYR)

#include "noise_dashboard_index.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/application.h"

namespace esphome::noise {

static const char *const TAG = "noise.web_server";

bool NoiseWebHandler::canHandle(AsyncWebServerRequest *request) const {
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
#else
  const auto &url = request->url();
#endif
  auto method = request->method();

  if (method == HTTP_GET && (url == "/" || url == "/index.html" || url == "/favicon.ico"))
    return true;

  if (url == "/api/noise" || url == "/api/noise/")
    return (method == HTTP_GET || method == HTTP_POST);

  return false;
}

void NoiseWebHandler::handleRequest(AsyncWebServerRequest *request) {
  bool is_index = false;
  bool is_api = false;
  bool is_favicon = false;

  {
#ifdef USE_ESP32
    char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
    StringRef url = request->url_to(url_buf);
#else
    const auto &url = request->url();
#endif
    if (url == "/" || url == "/index.html") {
      is_index = true;
    } else if (url == "/api/noise" || url == "/api/noise/") {
      is_api = true;
    } else if (url == "/favicon.ico") {
      is_favicon = true;
    }
  }

  if (is_favicon) {
    request->send(204);
    return;
  }

  if (is_index) {
    this->handle_index_request_(request);
    return;
  }

  if (is_api) {
    this->handle_api_request_(request);
    return;
  }
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
  buf.reserve(384);
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
  buf += "}";

  request->send(200, "application/json", buf.c_str());
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

#include "noise_web_server.h"

#if defined(USE_NETWORK) && !defined(USE_ZEPHYR)

#include "noise_dashboard_index.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

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

  if (method == HTTP_GET && (url == "/" || url == "/index.html"))
    return true;

  if (url == "/api/noise" || url == "/api/noise/")
    return (method == HTTP_GET || method == HTTP_POST);

  return false;
}

void NoiseWebHandler::handleRequest(AsyncWebServerRequest *request) {
#ifdef USE_ESP32
  char url_buf[AsyncWebServerRequest::URL_BUF_SIZE];
  StringRef url = request->url_to(url_buf);
#else
  const auto &url = request->url();
#endif

  if (url == "/" || url == "/index.html") {
    this->handle_index_request_(request);
    return;
  }

  if (url == "/api/noise" || url == "/api/noise/") {
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

void NoiseWebHandler::handle_api_request_(AsyncWebServerRequest *request) {
  if (this->parent_ == nullptr) {
    request->send(500, "application/json", "{\"error\":\"noise component not found\"}");
    return;
  }

  if (request->method() == HTTP_POST) {
    if (request->hasArg("action")) {
      std::string action = request->arg("action");
      if (action == "play") {
        std::string variant = request->hasArg("variant") ? request->arg("variant") : "";
        optional<float> vol;
        if (request->hasArg("volume")) {
          auto v = parse_number<float>(request->arg("volume"));
          if (v.has_value()) {
            vol = (*v > 1.0f) ? (*v / 100.0f) : *v;
          }
        }
        this->parent_->play(variant, 0, vol);
      } else if (action == "stop") {
        this->parent_->stop();
      } else if (action == "pause") {
        this->parent_->pause();
      } else if (action == "resume") {
        this->parent_->resume();
      } else if (action == "volume") {
        if (request->hasArg("value")) {
          auto val = parse_number<float>(request->arg("value"));
          if (val.has_value()) {
            float v = (*val > 1.0f) ? (*val / 100.0f) : *val;
            this->parent_->set_volume(v);
          }
        }
      } else if (action == "tone") {
        if (request->hasArg("value")) {
          auto val = parse_number<float>(request->arg("value"));
          if (val.has_value()) {
            float t = (*val > 1.0f) ? (*val / 100.0f) : *val;
            this->parent_->set_tone(t);
          }
        }
      } else if (action == "sleep_timer") {
        if (request->hasArg("minutes")) {
          auto val = parse_number<float>(request->arg("minutes"));
          if (val.has_value()) {
            this->parent_->set_sleep_timer(*val);
          }
        }
      } else if (action == "mute") {
        this->parent_->set_muted(!this->parent_->is_muted());
      }
    }
  }

  char buf[384];
  snprintf(buf, sizeof(buf),
           "{\"title\":\"%s\",\"show_components\":%s,\"running\":%s,\"paused\":%s,\"variant\":\"%s\",\"volume\":%.2f,\"tone\":%.2f,\"sleep_timer\":%.1f,\"muted\":%s}",
           this->title_.c_str(),
           this->show_components_ ? "true" : "false",
           this->parent_->is_running() ? "true" : "false",
           this->parent_->is_paused() ? "true" : "false",
           this->parent_->get_last_variant().c_str(),
           this->parent_->get_volume(),
           this->parent_->get_tone(),
           this->parent_->get_sleep_timer(),
           this->parent_->is_muted() ? "true" : "false");

  request->send(200, "application/json", buf);
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

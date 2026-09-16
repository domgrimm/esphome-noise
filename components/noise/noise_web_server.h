#pragma once

#include "noise.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"

#if defined(USE_NETWORK) && !defined(USE_ZEPHYR)
#include "esphome/components/web_server_base/web_server_base.h"

namespace esphome::noise {

class NoiseWebServer;

class NoiseWebHandler : public AsyncWebHandler {
 public:
  NoiseWebHandler(NoiseComponent *parent, std::string title, bool show_components)
      : parent_(parent), title_(std::move(title)), show_components_(show_components) {}

  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;

 protected:
  void handle_index_request_(AsyncWebServerRequest *request);
  void handle_api_request_(AsyncWebServerRequest *request);

  NoiseComponent *parent_{nullptr};
  std::string title_{"Noise Machine"};
  bool show_components_{true};
};

class NoiseWebServer : public Component {
 public:
  NoiseWebServer(NoiseComponent *parent, web_server_base::WebServerBase *base)
      : parent_(parent), base_(base) {}

  void set_title(const std::string &title) { this->title_ = title; }
  void set_show_components(bool show_components) { this->show_components_ = show_components; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::WIFI; }

 protected:
  NoiseComponent *parent_{nullptr};
  web_server_base::WebServerBase *base_{nullptr};
  NoiseWebHandler *handler_{nullptr};
  std::string title_{"Noise Machine"};
  bool show_components_{true};
};

}  // namespace esphome::noise

#endif

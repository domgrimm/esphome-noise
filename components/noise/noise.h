#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/speaker/speaker.h"

#include <cstdint>
#include <string>
#include <vector>

namespace esphome::noise {

enum class NoiseVariant : uint8_t {
  WHITE,
  PINK,
  BROWN,
  GRAY,
};

/// Procedural noise generator.
///
/// Hardware-agnostic: binds to any `speaker::Speaker` and follows that speaker's
/// configured audio stream info (sample rate, channels) so it can be dropped onto
/// ESP32 devices with any supported speaker/amp. Generates PCM in a dedicated
/// FreeRTOS task and writes it to the speaker ring buffer; no file, no network.
class NoiseComponent : public Component {
 public:
  void setup() override;
  void loop() override {}
  void dump_config() override;

  void set_speaker(speaker::Speaker *speaker) { this->speaker_ = speaker; }
  /// 0 = follow the speaker's own sample rate.
  void set_sample_rate(int sample_rate) { this->sample_rate_ = sample_rate; }

  void play(const std::string &variant);
  void stop();

 protected:
  static void noise_task_(void *param);
  inline void task_loop_();
  void generate_chunk_(int16_t *samples, size_t frames);

  uint32_t rng_{0x9E3779B9u};
  float y1_{0.f}, y2_{0.f}, y3_{0.f}, brown_{0.f};
  bool running_{false};
  bool stop_req_{false};
  NoiseVariant variant_{NoiseVariant::WHITE};
  int sample_rate_{0};
  uint32_t rate_{16000};
  uint8_t channels_{1};
  std::vector<int16_t> pcm_;
  speaker::Speaker *speaker_{nullptr};
  static constexpr size_t FRAMES_PER_CHUNK = 1024;
};

// Action support: noise.start / noise.stop
template<typename... Ts> class NoiseStartAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, variant)
  void play(const Ts &...x) override {
    auto variant = this->variant_.optional_value(x...);
    if (variant.has_value())
      this->parent_->play(*variant);
  }
};

template<typename... Ts> class NoiseStopAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->stop(); }
};

}  // namespace esphome::noise
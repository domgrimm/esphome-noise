#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/helpers.h"
#ifdef USE_MEDIA_PLAYER
#include "esphome/components/media_player/media_player.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#include "esphome/components/speaker/speaker.h"

#include "freertos/FreeRTOS.h"

#include <cstdint>
#include <string>
#include <vector>

#ifdef USE_NOISE_AIRPLAY
#include "esp_err.h"
namespace esphome::airplay_receiver {
esp_err_t audio_output_write(const void *data, size_t bytes, TickType_t wait);
bool audio_output_is_ready();
}  // namespace esphome::airplay_receiver
#endif

namespace esphome::noise {

enum class NoiseVariant : uint8_t {
  WHITE,
  PINK,
  BROWN,
  GRAY,
  WAVES,
  WIND,
  STREAM,
  FAN,
  RAIN,
  CAMPFIRE,
  HEARTBEAT,
  BEEP,
};

const char *noise_variant_to_string(NoiseVariant variant);
optional<NoiseVariant> string_to_noise_variant(const std::string &name);

class NoiseComponent;

#ifdef USE_SELECT
/// Select entity for choosing noise profiles and Off
class NoiseSelect : public select::Select {
 public:
  void set_parent(NoiseComponent *parent) { this->parent_ = parent; }
  void control(const std::string &value) override;

 protected:
  NoiseComponent *parent_{nullptr};
};
#endif

#ifdef USE_MEDIA_PLAYER
/// Media player entity for full HA media controls (Play/Pause/Stop/Volume)
class NoiseMediaPlayer : public media_player::MediaPlayer {
 public:
  void set_parent(NoiseComponent *parent) { this->parent_ = parent; }
  media_player::MediaPlayerTraits get_traits() override;
  void control(const media_player::MediaPlayerCall &call) override;
  bool is_muted() const override;

 protected:
  NoiseComponent *parent_{nullptr};
};
#endif

#ifdef USE_NUMBER
/// Volume number entity (0-100%)
class NoiseVolumeNumber : public number::Number {
 public:
  void set_parent(NoiseComponent *parent) { this->parent_ = parent; }
  void control(float value) override;

 protected:
  NoiseComponent *parent_{nullptr};
};

/// Tone number entity (0-100% low-pass acoustic filter)
class NoiseToneNumber : public number::Number {
 public:
  void set_parent(NoiseComponent *parent) { this->parent_ = parent; }
  void control(float value) override;

 protected:
  NoiseComponent *parent_{nullptr};
};

/// Sleep timer number entity (0-180 min auto-off)
class NoiseSleepTimerNumber : public number::Number {
 public:
  void set_parent(NoiseComponent *parent) { this->parent_ = parent; }
  void control(float value) override;

 protected:
  NoiseComponent *parent_{nullptr};
};
#endif

/// Procedural noise generator
class NoiseComponent : public Component {
 public:
  using Component::defer;

  void setup() override;
  void loop() override {}
  void dump_config() override;

  void set_speaker(speaker::Speaker *speaker) { this->speaker_ = speaker; }
#ifdef USE_MEDIA_PLAYER
  void set_airplay_receiver(media_player::MediaPlayer *ap) {
    this->airplay_receiver_ = ap;
    this->add_pause_source(ap);
  }
  void add_duck_source(media_player::MediaPlayer *player) {
    this->duck_sources_.push_back(player);
  }
  void add_pause_source(media_player::MediaPlayer *player) {
    this->pause_sources_.push_back(player);
  }
#endif
  void set_default_duck_level(float level) {
    this->duck_level_ = clamp(level, 0.0f, 1.0f);
  }

  void set_sample_rate(int sample_rate) { this->sample_rate_config_ = sample_rate; }
  void set_channels(uint8_t channels) { this->channels_config_ = channels; }
  void set_fade_in_time(uint32_t ms) { this->fade_in_time_ms_ = ms; }
  void set_fade_out_time(uint32_t ms) { this->fade_out_time_ms_ = ms; }

#ifdef USE_SELECT
  void set_select(select::Select *sel) { this->select_ = sel; }
#endif
#ifdef USE_MEDIA_PLAYER
  void set_media_player(media_player::MediaPlayer *mp) { this->media_player_ = mp; }
#endif
#ifdef USE_NUMBER
  void set_volume_number(number::Number *num) { this->volume_number_ = num; }
  void set_tone_number(number::Number *num) { this->tone_number_ = num; }
  void set_sleep_timer_number(number::Number *num) { this->sleep_timer_number_ = num; }
#endif

  void play(const std::string &variant, uint32_t duration_ms = 0, optional<float> volume = {});
  void play(NoiseVariant variant, uint32_t duration_ms = 0, optional<float> volume = {});
  void stop();
  void pause();
  void resume();
  void duck(float level = 0.2f);
  void unduck();
  void set_volume(float volume);
  void set_tone(float tone);
  void set_sleep_timer(float minutes);
  void set_muted(bool muted);

  bool is_running() const { return this->running_; }
  bool is_paused() const { return this->paused_; }
  bool is_muted() const { return this->muted_; }
  float get_volume() const { return this->volume_; }
  float get_tone() const { return this->tone_; }
  float get_sleep_timer() const { return this->sleep_timer_minutes_; }
  float get_sleep_timer_remaining_sec() const;
  std::string get_last_variant() const { return this->last_variant_; }

  template<typename F> void add_on_play_callback(F &&callback) {
    this->play_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_stop_callback(F &&callback) {
    this->stop_callback_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_variant_changed_callback(F &&callback) {
    this->variant_changed_callback_.add(std::forward<F>(callback));
  }

 protected:
  static void noise_task_(void *param);
  void task_loop_();
  void generate_chunk_(int16_t *samples, size_t frames);
  void finish_();
#ifdef USE_MEDIA_PLAYER
  void on_external_player_state_changed_();
#endif

  speaker::Speaker *speaker_{nullptr};
#ifdef USE_MEDIA_PLAYER
  media_player::MediaPlayer *airplay_receiver_{nullptr};
  std::vector<media_player::MediaPlayer *> duck_sources_;
  std::vector<media_player::MediaPlayer *> pause_sources_;
#endif

#ifdef USE_SELECT
  select::Select *select_{nullptr};
#endif
#ifdef USE_MEDIA_PLAYER
  media_player::MediaPlayer *media_player_{nullptr};
#endif
#ifdef USE_NUMBER
  number::Number *volume_number_{nullptr};
  number::Number *tone_number_{nullptr};
  number::Number *sleep_timer_number_{nullptr};
#endif

  int sample_rate_config_{0};
  uint8_t channels_config_{0};
  uint32_t fade_in_time_ms_{100};
  uint32_t fade_out_time_ms_{100};

  uint32_t rate_{16000};
  uint8_t channels_{1};
  uint32_t fade_in_samples_{1600};
  uint32_t fade_out_samples_{1600};

  float volume_{1.0f};
  float tone_{1.0f};
  float duck_level_{0.2f};
  float sleep_timer_minutes_{0.0f};

  bool running_{false};
  bool stop_req_{false};
  bool paused_{false};
  bool ducked_{false};
  bool muted_{false};
  bool external_paused_{false};
  bool external_ducked_{false};

  float current_gain_{0.0f};

  NoiseVariant variant_{NoiseVariant::BROWN};
  std::string last_variant_{"Brown"};

  uint64_t max_samples_{0};
  uint32_t beep_len_{0};
  uint64_t time_smp_{0};

  void *task_handle_{nullptr};

  // DSP state variables (L and R decorrelated for true stereo)
  uint32_t rng_l_{0x9E3779B9u};
  uint32_t rng_r_{0x85EBCA6Bu};

  float y1_l_{0.f}, y2_l_{0.f}, y3_l_{0.f}, brown_l_{0.f}, lp_l_{0.f};
  float y1_r_{0.f}, y2_r_{0.f}, y3_r_{0.f}, brown_r_{0.f}, lp_r_{0.f};

  float wind_target_{1.f}, wind_amp_l_{1.f}, wind_amp_r_{1.f};
  float tone_lp_l_{0.f}, tone_lp_r_{0.f};

  float rain_drop_l_{0.f}, rain_drop_r_{0.f};
  float campfire_crackle_l_{0.f}, campfire_crackle_r_{0.f};
  float campfire_freq_l_{1000.f}, campfire_freq_r_{1000.f};
  float campfire_phase_l_{0.f}, campfire_phase_r_{0.f};

  std::vector<int16_t> pcm_;
  static constexpr size_t FRAMES_PER_CHUNK = 1024;

  CallbackManager<void()> play_callback_;
  CallbackManager<void()> stop_callback_;
  CallbackManager<void(std::string)> variant_changed_callback_;
};

// Automation Actions
template<typename... Ts> class NoiseStartAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  TEMPLATABLE_VALUE(std::string, variant)
  TEMPLATABLE_VALUE(uint32_t, duration)
  TEMPLATABLE_VALUE(float, volume)

  void play(const Ts &...x) override {
    auto v = this->variant_.optional_value(x...);
    auto dur = this->duration_.optional_value(x...);
    auto vol = this->volume_.optional_value(x...);
    std::string variant_str = v.has_value() ? *v : "";
    uint32_t d = dur.has_value() ? *dur : 0;
    this->parent_->play(variant_str, d, vol);
  }
};

template<typename... Ts> class NoiseStopAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->stop(); }
};

template<typename... Ts> class NoisePauseAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->pause(); }
};

template<typename... Ts> class NoiseResumeAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->resume(); }
};

template<typename... Ts> class NoiseDuckAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  TEMPLATABLE_VALUE(float, level)
  void play(const Ts &...x) override {
    auto lvl = this->level_.optional_value(x...);
    this->parent_->duck(lvl.value_or(0.2f));
  }
};

template<typename... Ts> class NoiseUnduckAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  void play(const Ts &...x) override { this->parent_->unduck(); }
};

template<typename... Ts> class NoiseSetVolumeAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  TEMPLATABLE_VALUE(float, volume)
  void play(const Ts &...x) override {
    auto vol = this->volume_.optional_value(x...);
    if (vol.has_value())
      this->parent_->set_volume(*vol);
  }
};

template<typename... Ts> class NoiseSetToneAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  TEMPLATABLE_VALUE(float, tone)
  void play(const Ts &...x) override {
    auto t = this->tone_.optional_value(x...);
    if (t.has_value())
      this->parent_->set_tone(*t);
  }
};

template<typename... Ts> class NoiseSetSleepTimerAction : public Action<Ts...>, public Parented<NoiseComponent> {
 public:
  TEMPLATABLE_VALUE(float, sleep_timer)
  void play(const Ts &...x) override {
    auto m = this->sleep_timer_.optional_value(x...);
    if (m.has_value())
      this->parent_->set_sleep_timer(*m);
  }
};

}  // namespace esphome::noise
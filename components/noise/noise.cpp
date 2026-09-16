#include "noise.h"

#include "esphome/core/log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cmath>
#include <algorithm>

namespace esphome::noise {

static const char *const TAG = "noise";

const char *noise_variant_to_string(NoiseVariant variant) {
  switch (variant) {
    case NoiseVariant::WHITE: return "White";
    case NoiseVariant::PINK: return "Pink";
    case NoiseVariant::BROWN: return "Brown";
    case NoiseVariant::GRAY: return "Gray";
    case NoiseVariant::WAVES: return "Waves";
    case NoiseVariant::WIND: return "Wind";
    case NoiseVariant::STREAM: return "Stream";
    case NoiseVariant::FAN: return "Fan";
    case NoiseVariant::RAIN: return "Rain";
    case NoiseVariant::CAMPFIRE: return "Campfire";
    case NoiseVariant::HEARTBEAT: return "Heartbeat";
    case NoiseVariant::BEEP: return "Beep";
    default: return "Brown";
  }
}

optional<NoiseVariant> string_to_noise_variant(const std::string &name) {
  std::string lower;
  lower.reserve(name.size());
  for (char c : name)
    lower += (c >= 'A' && c <= 'Z') ? char(c + 32) : c;

  if (lower == "white") return NoiseVariant::WHITE;
  if (lower == "pink") return NoiseVariant::PINK;
  if (lower == "brown") return NoiseVariant::BROWN;
  if (lower == "gray") return NoiseVariant::GRAY;
  if (lower == "waves") return NoiseVariant::WAVES;
  if (lower == "wind") return NoiseVariant::WIND;
  if (lower == "stream") return NoiseVariant::STREAM;
  if (lower == "fan") return NoiseVariant::FAN;
  if (lower == "rain") return NoiseVariant::RAIN;
  if (lower == "campfire") return NoiseVariant::CAMPFIRE;
  if (lower == "heartbeat") return NoiseVariant::HEARTBEAT;
  if (lower == "beep") return NoiseVariant::BEEP;
  return {};
}

#ifdef USE_SELECT
// Select entity
void NoiseSelect::control(const std::string &value) {
  if (value == "Off") {
    this->parent_->stop();
  } else {
    auto v = string_to_noise_variant(value);
    if (v.has_value()) {
      this->parent_->play(*v);
    }
  }
  this->publish_state(value);
}
#endif

#ifdef USE_MEDIA_PLAYER
// Media player entity
media_player::MediaPlayerTraits NoiseMediaPlayer::get_traits() {
  auto traits = media_player::MediaPlayerTraits();
  traits.set_supports_pause(true);
  traits.set_supports_turn_off_on(true);
  traits.add_feature_flags(media_player::MediaPlayerEntityFeature::PLAY);
  return traits;
}

void NoiseMediaPlayer::control(const media_player::MediaPlayerCall &call) {
  if (this->parent_ == nullptr)
    return;
  if (call.get_volume().has_value()) {
    this->parent_->set_volume(*call.get_volume());
  }
  if (call.get_media_url().has_value() && !call.get_media_url()->empty()) {
    this->parent_->play(*call.get_media_url());
    return;
  }
  if (call.get_command().has_value()) {
    switch (*call.get_command()) {
      case media_player::MEDIA_PLAYER_COMMAND_PLAY:
      case media_player::MEDIA_PLAYER_COMMAND_TURN_ON:
        this->parent_->resume();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_PAUSE:
        this->parent_->pause();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_STOP:
      case media_player::MEDIA_PLAYER_COMMAND_TURN_OFF:
        this->parent_->stop();
        break;
      case media_player::MEDIA_PLAYER_COMMAND_MUTE:
        this->parent_->set_muted(true);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_UNMUTE:
        this->parent_->set_muted(false);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_TOGGLE:
        if (this->parent_->is_running() && !this->parent_->is_paused()) {
          this->parent_->pause();
        } else {
          this->parent_->resume();
        }
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_UP:
        this->parent_->set_volume(this->parent_->get_volume() + 0.05f);
        break;
      case media_player::MEDIA_PLAYER_COMMAND_VOLUME_DOWN:
        this->parent_->set_volume(this->parent_->get_volume() - 0.05f);
        break;
      default:
        break;
    }
  }
}

bool NoiseMediaPlayer::is_muted() const {
  return this->parent_ != nullptr && this->parent_->is_muted();
}
#endif

#ifdef USE_NUMBER
// Number entities
void NoiseVolumeNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr)
    this->parent_->set_volume(value / 100.0f);
}

void NoiseToneNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr)
    this->parent_->set_tone(value / 100.0f);
}

void NoiseSleepTimerNumber::control(float value) {
  this->publish_state(value);
  if (this->parent_ != nullptr)
    this->parent_->set_sleep_timer(value);
}
#endif

// Component setup and config dump
void NoiseComponent::setup() {
#if defined(USE_NOISE_AIRPLAY) && defined(USE_MEDIA_PLAYER)
  if (this->speaker_ == nullptr && this->airplay_receiver_ == nullptr) {
    ESP_LOGE(TAG, "Neither speaker nor airplay_receiver configured");
#else
  if (this->speaker_ == nullptr) {
    ESP_LOGE(TAG, "Speaker not configured");
#endif
    this->mark_failed();
    return;
  }

#ifdef USE_MEDIA_PLAYER
  for (auto *player : this->duck_sources_) {
    player->add_on_state_callback([this](media_player::MediaPlayerState) {
      this->on_external_player_state_changed_();
    });
  }
  for (auto *player : this->pause_sources_) {
    player->add_on_state_callback([this](media_player::MediaPlayerState) {
      this->on_external_player_state_changed_();
    });
  }
#endif

#ifdef USE_NUMBER
  if (this->volume_number_ != nullptr) {
    this->volume_number_->publish_state(this->volume_ * 100.0f);
  }
  if (this->tone_number_ != nullptr) {
    this->tone_number_->publish_state(this->tone_ * 100.0f);
  }
  if (this->sleep_timer_number_ != nullptr) {
    this->sleep_timer_number_->publish_state(this->sleep_timer_minutes_);
  }
#endif
#ifdef USE_SELECT
  if (this->select_ != nullptr) {
    this->select_->publish_state("Off");
  }
#endif
#ifdef USE_MEDIA_PLAYER
  if (this->media_player_ != nullptr) {
    this->media_player_->state = media_player::MEDIA_PLAYER_STATE_IDLE;
    this->media_player_->volume = this->volume_;
    this->media_player_->publish_state();
  }
#endif
#ifdef USE_SENSOR
  if (this->time_left_sensor_ != nullptr) {
    this->time_left_sensor_->publish_state(0.0f);
  }
#endif
}

void NoiseComponent::loop() {
#ifdef USE_SENSOR
  if (this->time_left_sensor_ != nullptr) {
    uint32_t now = millis();
    if (now - this->last_time_left_check_ms_ >= 500) {
      this->last_time_left_check_ms_ = now;
      float cur_val = 0.0f;
      if (this->running_ && this->max_samples_ > 0 && this->time_smp_ < this->max_samples_) {
        float remaining_sec = static_cast<float>(this->max_samples_ - this->time_smp_) / static_cast<float>(this->rate_);
        if (this->time_left_sensor_->get_unit_of_measurement() == "s" ||
            this->time_left_sensor_->get_unit_of_measurement() == "sec") {
          cur_val = std::ceil(remaining_sec);
        } else {
          cur_val = std::ceil(remaining_sec / 60.0f);
        }
      }
      if (cur_val != this->last_time_left_published_) {
        this->last_time_left_published_ = cur_val;
        this->time_left_sensor_->publish_state(cur_val);
      }
    }
  }
#endif
}

void NoiseComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Noise generator:");
  if (this->speaker_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Backend: Standard Speaker");
#if defined(USE_NOISE_AIRPLAY) && defined(USE_MEDIA_PLAYER)
  } else if (this->airplay_receiver_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Backend: AirPlay 2 Receiver Direct I2S");
#endif
  }
  ESP_LOGCONFIG(TAG, "  Sample Rate: %s", this->sample_rate_config_ > 0 ? "override" : "default");
  ESP_LOGCONFIG(TAG, "  Channels: %s",
                this->channels_config_ > 0 ? (this->channels_config_ == 2 ? "stereo override" : "mono override")
                                           : "default");
  ESP_LOGCONFIG(TAG, "  Fade In: %u ms, Fade Out: %u ms", (unsigned) this->fade_in_time_ms_,
                (unsigned) this->fade_out_time_ms_);
  ESP_LOGCONFIG(TAG, "  Volume: %.0f%%", this->volume_ * 100.0f);
  ESP_LOGCONFIG(TAG, "  Tone: %.0f%%", this->tone_ * 100.0f);
  if (this->sleep_timer_minutes_ > 0.0f)
    ESP_LOGCONFIG(TAG, "  Sleep Timer: %.0f min", this->sleep_timer_minutes_);
#ifdef USE_MEDIA_PLAYER
  if (!this->duck_sources_.empty())
    ESP_LOGCONFIG(TAG, "  Multi-Source: %zu duck source(s) configured", this->duck_sources_.size());
  if (!this->pause_sources_.empty())
    ESP_LOGCONFIG(TAG, "  Multi-Source: %zu pause source(s) configured", this->pause_sources_.size());
#endif
}

#ifdef USE_MEDIA_PLAYER
void NoiseComponent::on_external_player_state_changed_() {
  bool should_pause = false;
  for (auto *p : this->pause_sources_) {
    if (p != nullptr && (p->state == media_player::MEDIA_PLAYER_STATE_PLAYING ||
                         p->state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING)) {
      should_pause = true;
      break;
    }
  }

  if (should_pause) {
    if (this->running_ && !this->external_paused_) {
      this->external_paused_ = true;
      this->pause();
    }
    return;
  } else if (this->external_paused_) {
    this->external_paused_ = false;
    this->resume();
  }

  bool should_duck = false;
  for (auto *p : this->duck_sources_) {
    if (p != nullptr && (p->state == media_player::MEDIA_PLAYER_STATE_PLAYING ||
                         p->state == media_player::MEDIA_PLAYER_STATE_ANNOUNCING)) {
      should_duck = true;
      break;
    }
  }

  if (should_duck != this->external_ducked_) {
    this->external_ducked_ = should_duck;
    if (should_duck) {
      this->duck(this->duck_level_);
    } else {
      this->unduck();
    }
  }
}
#endif

void NoiseComponent::set_volume(float volume) {
  this->volume_ = clamp(volume, 0.0f, 1.0f);
#ifdef USE_NUMBER
  if (this->volume_number_ != nullptr && this->volume_number_->state != this->volume_ * 100.0f) {
    this->volume_number_->publish_state(this->volume_ * 100.0f);
  }
#endif
#ifdef USE_MEDIA_PLAYER
  if (this->media_player_ != nullptr && this->media_player_->volume != this->volume_) {
    this->media_player_->volume = this->volume_;
    this->media_player_->publish_state();
  }
#endif
}

void NoiseComponent::set_tone(float tone) {
  this->tone_ = clamp(tone, 0.0f, 1.0f);
#ifdef USE_NUMBER
  if (this->tone_number_ != nullptr && this->tone_number_->state != this->tone_ * 100.0f) {
    this->tone_number_->publish_state(this->tone_ * 100.0f);
  }
#endif
}

float NoiseComponent::get_sleep_timer_remaining_sec() const {
  if (!this->running_ || this->max_samples_ == 0 || this->time_smp_ >= this->max_samples_)
    return 0.0f;
  return static_cast<float>(this->max_samples_ - this->time_smp_) / static_cast<float>(this->rate_);
}

void NoiseComponent::set_sleep_timer(float minutes) {
  this->sleep_timer_minutes_ = std::max(0.0f, minutes);
  if (this->running_) {
    if (this->sleep_timer_minutes_ > 0.0f) {
      this->max_samples_ = this->time_smp_ + static_cast<uint64_t>(this->sleep_timer_minutes_ * 60.0f * static_cast<float>(this->rate_));
      ESP_LOGI(TAG, "Sleep timer set to %.1f min while running", this->sleep_timer_minutes_);
    } else {
      this->max_samples_ = 0;
      ESP_LOGI(TAG, "Sleep timer cancelled while running");
    }
  }
#ifdef USE_NUMBER
  if (this->sleep_timer_number_ != nullptr && this->sleep_timer_number_->state != this->sleep_timer_minutes_) {
    this->sleep_timer_number_->publish_state(this->sleep_timer_minutes_);
  }
#endif
}

void NoiseComponent::duck(float level) {
  this->duck_level_ = clamp(level, 0.0f, 1.0f);
  this->ducked_ = true;
  ESP_LOGD(TAG, "Noise ducked to %.0f%%", this->duck_level_ * 100.0f);
}

void NoiseComponent::unduck() {
  this->ducked_ = false;
  ESP_LOGD(TAG, "Noise unducked");
}

void NoiseComponent::set_muted(bool muted) {
  this->muted_ = muted;
#ifdef USE_MEDIA_PLAYER
  if (this->media_player_ != nullptr)
    this->media_player_->publish_state();
#endif
}

void NoiseComponent::pause() {
  if (!this->running_ || this->paused_)
    return;
  this->paused_ = true;
#ifdef USE_MEDIA_PLAYER
  if (this->media_player_ != nullptr) {
    this->media_player_->state = media_player::MEDIA_PLAYER_STATE_PAUSED;
    this->media_player_->publish_state();
  }
#endif
  ESP_LOGI(TAG, "Noise paused");
}

void NoiseComponent::resume() {
  if (!this->running_) {
    this->play(this->variant_);
    return;
  }
  if (this->paused_) {
    this->paused_ = false;
#ifdef USE_MEDIA_PLAYER
    if (this->media_player_ != nullptr) {
      this->media_player_->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      this->media_player_->publish_state();
    }
#endif
    ESP_LOGI(TAG, "Noise resumed");
  }
}

void NoiseComponent::play(const std::string &variant, uint32_t duration_ms, optional<float> volume) {
  NoiseVariant v = this->variant_;
  if (!variant.empty()) {
    auto opt_v = string_to_noise_variant(variant);
    if (opt_v.has_value()) {
      v = *opt_v;
    } else {
      ESP_LOGW(TAG, "Unknown variant '%s', using '%s'", variant.c_str(), this->last_variant_.c_str());
    }
  }
  this->play(v, duration_ms, volume);
}

void NoiseComponent::play(NoiseVariant variant, uint32_t duration_ms, optional<float> volume) {
  if (volume.has_value()) {
    this->set_volume(*volume);
  }

  this->variant_ = variant;
  this->last_variant_ = noise_variant_to_string(variant);

  if (variant == NoiseVariant::BEEP) {
    this->max_samples_ = 0;
    this->beep_len_ = (uint32_t) (0.25f * this->rate_);
  } else if (duration_ms > 0) {
    this->max_samples_ = ((uint64_t) duration_ms * this->rate_) / 1000;
  } else if (this->sleep_timer_minutes_ > 0.0f) {
    this->max_samples_ = (uint64_t) (this->sleep_timer_minutes_ * 60.0f * (float) this->rate_);
  } else {
    this->max_samples_ = 0;
  }

  this->time_smp_ = 0;
  this->paused_ = false;

  if (this->running_) {
    this->stop_req_ = false;
    ESP_LOGI(TAG, "Switched noise to %s", this->last_variant_.c_str());
#ifdef USE_SELECT
    if (this->select_ != nullptr)
      this->select_->publish_state(this->last_variant_);
#endif
#ifdef USE_MEDIA_PLAYER
    if (this->media_player_ != nullptr) {
      this->media_player_->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
      this->media_player_->publish_state();
    }
#endif
    this->variant_changed_callback_.call(this->last_variant_);
    return;
  }

  uint32_t rate = 16000;
  uint8_t ch = 1;

  if (this->speaker_ != nullptr) {
    auto info = this->speaker_->get_audio_stream_info();
    rate = info.get_sample_rate() ? info.get_sample_rate() : 16000;
    ch = info.get_channels() ? info.get_channels() : 1;
    if (info.get_bits_per_sample() != 0 && info.get_bits_per_sample() != 16) {
      ESP_LOGW(TAG, "Speaker uses %u-bit samples; generating 16-bit", info.get_bits_per_sample());
    }
#if defined(USE_NOISE_AIRPLAY) && defined(USE_MEDIA_PLAYER)
  } else if (this->airplay_receiver_ != nullptr) {
    rate = 44100;
    ch = 2;
#endif
  }

  if (this->sample_rate_config_ > 0)
    rate = this->sample_rate_config_;
  if (this->channels_config_ > 0)
    ch = this->channels_config_;

  this->rate_ = rate;
  this->channels_ = ch;

  this->fade_in_samples_ = (this->fade_in_time_ms_ * this->rate_) / 1000;
  this->fade_out_samples_ = (this->fade_out_time_ms_ * this->rate_) / 1000;

  if (this->speaker_ != nullptr) {
    this->speaker_->set_audio_stream_info(audio::AudioStreamInfo(16, this->channels_, this->rate_));
    this->speaker_->start();
  }

  this->pcm_.resize(FRAMES_PER_CHUNK * this->channels_);

  this->current_gain_ = 0.0f;
  this->stop_req_ = false;
  this->running_ = true;

#if defined(USE_ESP32) && !defined(CONFIG_FREERTOS_UNICORE)
  xTaskCreatePinnedToCore(&NoiseComponent::noise_task_, "noise", 4096, this, 2,
                          reinterpret_cast<TaskHandle_t *>(&this->task_handle_), 0);
#else
  xTaskCreate(&NoiseComponent::noise_task_, "noise", 4096, this, 2,
              reinterpret_cast<TaskHandle_t *>(&this->task_handle_));
#endif

  ESP_LOGI(TAG, "Playing %s noise (%u Hz, %u ch)", this->last_variant_.c_str(), (unsigned) this->rate_, this->channels_);

#ifdef USE_SELECT
  if (this->select_ != nullptr)
    this->select_->publish_state(this->last_variant_);
#endif
#ifdef USE_MEDIA_PLAYER
  if (this->media_player_ != nullptr) {
    this->media_player_->state = media_player::MEDIA_PLAYER_STATE_PLAYING;
    this->media_player_->publish_state();
  }
#endif
  this->play_callback_.call();
  this->variant_changed_callback_.call(this->last_variant_);
}

void NoiseComponent::stop() {
  if (!this->running_ && !this->stop_req_)
    return;
  this->stop_req_ = true;
  this->paused_ = false;
  this->max_samples_ = 0;
#ifdef USE_NUMBER
  if (this->sleep_timer_minutes_ > 0.0f) {
    this->sleep_timer_minutes_ = 0.0f;
    if (this->sleep_timer_number_ != nullptr)
      this->sleep_timer_number_->publish_state(0.0f);
  }
#endif
#ifdef USE_SELECT
  if (this->select_ != nullptr)
    this->select_->publish_state("Off");
#endif
#ifdef USE_MEDIA_PLAYER
  if (this->media_player_ != nullptr) {
    this->media_player_->state = media_player::MEDIA_PLAYER_STATE_IDLE;
    this->media_player_->publish_state();
  }
#endif
#ifdef USE_SENSOR
  this->last_time_left_published_ = 0.0f;
  if (this->time_left_sensor_ != nullptr)
    this->time_left_sensor_->publish_state(0.0f);
#endif
  this->stop_callback_.call();
  ESP_LOGI(TAG, "Noise stopping (fade-out initiated)");
}

void NoiseComponent::finish_() {
  this->defer([this]() {
#ifdef USE_SELECT
    if (this->select_ != nullptr)
      this->select_->publish_state("Off");
#endif
#ifdef USE_MEDIA_PLAYER
    if (this->media_player_ != nullptr) {
      this->media_player_->state = media_player::MEDIA_PLAYER_STATE_IDLE;
      this->media_player_->publish_state();
    }
#endif
#ifdef USE_NUMBER
    this->sleep_timer_minutes_ = 0.0f;
    if (this->sleep_timer_number_ != nullptr)
      this->sleep_timer_number_->publish_state(0.0f);
#endif
#ifdef USE_SENSOR
    this->last_time_left_published_ = 0.0f;
    if (this->time_left_sensor_ != nullptr)
      this->time_left_sensor_->publish_state(0.0f);
#endif
    this->stop_callback_.call();
  });
  ESP_LOGI(TAG, "Playback completed");
}

void NoiseComponent::noise_task_(void *param) {
  auto *self = static_cast<NoiseComponent *>(param);
  self->task_loop_();
  self->task_handle_ = nullptr;
  vTaskDelete(nullptr);
}

void NoiseComponent::task_loop_() {
  while (true) {
    this->generate_chunk_(this->pcm_.data(), FRAMES_PER_CHUNK);

#ifdef USE_NOISE_AIRPLAY
    if (this->airplay_receiver_ != nullptr) {
      if (!this->external_paused_) {
        esphome::airplay_receiver::audio_output_write(
            this->pcm_.data(), this->pcm_.size() * sizeof(int16_t), pdMS_TO_TICKS(20));
      }
    } else
#endif
    if (this->speaker_ != nullptr) {
      size_t to_write = this->pcm_.size() * sizeof(int16_t);
      const uint8_t *ptr = reinterpret_cast<const uint8_t *>(this->pcm_.data());
      while (to_write > 0 && !this->stop_req_) {
        size_t written = this->speaker_->play(ptr, to_write, pdMS_TO_TICKS(20));
        if (written > 0) {
          ptr += written;
          to_write -= written;
        } else {
          vTaskDelay(pdMS_TO_TICKS(2));
        }
      }
    }

    // If stop requested and faded down to silence: exit task loop cleanly
    if (this->stop_req_ && this->current_gain_ <= 0.0001f) {
      break;
    }

    // Check one-shot beep finish
    if (this->variant_ == NoiseVariant::BEEP && this->time_smp_ >= this->beep_len_) {
      this->finish_();
      break;
    }

    // Check sleep timer duration finish
    if (this->max_samples_ > 0 && this->time_smp_ >= this->max_samples_) {
      this->finish_();
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (this->speaker_ != nullptr) {
    this->speaker_->stop();
  }
  this->running_ = false;
  this->stop_req_ = false;
}

void NoiseComponent::generate_chunk_(int16_t *samples, size_t frames) {
  uint32_t rng_l = this->rng_l_;
  uint32_t rng_r = this->rng_r_;
  const uint8_t channels = this->channels_;
  const float rate = static_cast<float>(this->rate_);
  const float tau2pi = 6.28318530718f;

  const float gain_step_up = 1.0f / (float) std::max<uint32_t>(1, this->fade_in_samples_);
  const float gain_step_down = 1.0f / (float) std::max<uint32_t>(1, this->fade_out_samples_);
  const float target_gain = (this->paused_ || this->stop_req_ || this->muted_)
                                ? 0.0f
                                : (this->ducked_ ? (this->volume_ * this->duck_level_) : this->volume_);

  for (size_t i = 0; i < frames; i++) {
    const uint64_t t = this->time_smp_++;
    const float ft = static_cast<float>(t);

    // Smooth gain ramping per sample
    if (this->current_gain_ < target_gain) {
      this->current_gain_ += gain_step_up;
      if (this->current_gain_ > target_gain)
        this->current_gain_ = target_gain;
    } else if (this->current_gain_ > target_gain) {
      this->current_gain_ -= gain_step_down;
      if (this->current_gain_ < target_gain)
        this->current_gain_ = target_gain;
    }

    // PRNG Left
    rng_l ^= rng_l << 13;
    rng_l ^= rng_l >> 17;
    rng_l ^= rng_l << 5;
    const float w_l = (float) ((int16_t) (rng_l >> 16)) / 32768.0f;
    const float rndf_l = (float) (rng_l & 0xFFFF) / 65536.0f;

    // PRNG Right (decorrelated if stereo)
    float w_r = w_l;
    float rndf_r = rndf_l;
    if (channels > 1) {
      rng_r ^= rng_r << 13;
      rng_r ^= rng_r >> 17;
      rng_r ^= rng_r << 5;
      w_r = (float) ((int16_t) (rng_r >> 16)) / 32768.0f;
      rndf_r = (float) (rng_r & 0xFFFF) / 65536.0f;
    }

    auto brown_core_l = [&]() {
      this->brown_l_ = (this->brown_l_ + 0.02f * w_l) / 1.02f;
      return this->brown_l_;
    };
    auto brown_core_r = [&]() {
      this->brown_r_ = (this->brown_r_ + 0.02f * w_r) / 1.02f;
      return this->brown_r_;
    };

    auto pink_core_l = [&]() {
      this->y1_l_ += 0.05f * (w_l - this->y1_l_);
      this->y2_l_ += 0.22f * (w_l - this->y2_l_);
      this->y3_l_ += 0.50f * (w_l - this->y3_l_);
      return (this->y1_l_ + this->y2_l_ + this->y3_l_) * 1.4f;
    };
    auto pink_core_r = [&]() {
      this->y1_r_ += 0.05f * (w_r - this->y1_r_);
      this->y2_r_ += 0.22f * (w_r - this->y2_r_);
      this->y3_r_ += 0.50f * (w_r - this->y3_r_);
      return (this->y1_r_ + this->y2_r_ + this->y3_r_) * 1.4f;
    };

    float out_l = 0.0f;
    float out_r = 0.0f;

    switch (this->variant_) {
      case NoiseVariant::WHITE:
        out_l = w_l * 0.25f;
        out_r = (channels > 1) ? (w_r * 0.25f) : out_l;
        break;
      case NoiseVariant::PINK:
        out_l = pink_core_l() * 0.18f;
        out_r = (channels > 1) ? (pink_core_r() * 0.18f) : out_l;
        break;
      case NoiseVariant::BROWN:
        out_l = brown_core_l() * 1.75f;
        out_r = (channels > 1) ? (brown_core_r() * 1.75f) : out_l;
        break;
      case NoiseVariant::GRAY: {
        rng_l ^= rng_l << 13; rng_l ^= rng_l >> 17; rng_l ^= rng_l << 5;
        const float w2_l = (float) ((int16_t) (rng_l >> 16)) / 32768.0f;
        out_l = 0.7f * (w_l + w2_l) / 1.4142f * 0.36f;
        if (channels > 1) {
          rng_r ^= rng_r << 13; rng_r ^= rng_r >> 17; rng_r ^= rng_r << 5;
          const float w2_r = (float) ((int16_t) (rng_r >> 16)) / 32768.0f;
          out_r = 0.7f * (w_r + w2_r) / 1.4142f * 0.36f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::WAVES: {
        brown_core_l();
        const float swell_l = 0.55f + 0.35f * std::sin(tau2pi * 0.11f * ft / rate) +
                             0.10f * std::sin(tau2pi * 0.29f * ft / rate + 1.7f);
        out_l = (this->brown_l_ * swell_l * 3.0f + w_l * 0.10f * swell_l) * 0.90f;
        if (channels > 1) {
          brown_core_r();
          const float swell_r = 0.55f + 0.35f * std::sin(tau2pi * 0.11f * ft / rate + 0.8f) +
                               0.10f * std::sin(tau2pi * 0.29f * ft / rate + 2.4f);
          out_r = (this->brown_r_ * swell_r * 3.0f + w_r * 0.10f * swell_r) * 0.90f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::WIND: {
        this->lp_l_ += (w_l - this->lp_l_) * 0.035f;
        if ((t & 0x1FF) == 0)
          this->wind_target_ = 0.4f + 0.6f * rndf_l;
        this->wind_amp_l_ += (this->wind_target_ - this->wind_amp_l_) * 0.002f;
        const float wob_l = 1.0f + 0.12f * std::sin(tau2pi * 0.33f * ft / rate);
        out_l = this->lp_l_ * this->wind_amp_l_ * wob_l * 1.9f;
        if (channels > 1) {
          this->lp_r_ += (w_r - this->lp_r_) * 0.035f;
          this->wind_amp_r_ += (this->wind_target_ - this->wind_amp_r_) * 0.002f;
          const float wob_r = 1.0f + 0.12f * std::sin(tau2pi * 0.33f * ft / rate + 1.2f);
          out_r = this->lp_r_ * this->wind_amp_r_ * wob_r * 1.9f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::STREAM: {
        const float pink_l = pink_core_l();
        const float m_l = 0.75f + 0.25f * std::sin(tau2pi * 0.13f * ft / rate) +
                          0.12f * std::sin(tau2pi * 0.41f * ft / rate + 2.1f);
        out_l = pink_l * m_l * 0.90f * 0.24f;
        if (channels > 1) {
          const float pink_r = pink_core_r();
          const float m_r = 0.75f + 0.25f * std::sin(tau2pi * 0.13f * ft / rate + 1.1f) +
                            0.12f * std::sin(tau2pi * 0.41f * ft / rate + 3.2f);
          out_r = pink_r * m_r * 0.90f * 0.24f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::FAN: {
        this->lp_l_ += (w_l - this->lp_l_) * 0.06f;
        const float wob = 0.85f + 0.15f * std::sin(tau2pi * 30.0f * ft / rate);
        out_l = this->lp_l_ * wob * 1.3f;
        if (channels > 1) {
          this->lp_r_ += (w_r - this->lp_r_) * 0.06f;
          out_r = this->lp_r_ * wob * 1.3f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::RAIN: {
        const float base_l = pink_core_l() * 0.15f;
        if (rndf_l < 0.0006f)
          this->rain_drop_l_ = 0.4f + 0.3f * rndf_l * 1000.0f;
        const float drop_l = this->rain_drop_l_ * (w_l > 0.0f ? 1.0f : -1.0f);
        this->rain_drop_l_ *= 0.97f;
        out_l = (base_l * 0.8f + drop_l * 0.4f) * 0.35f;

        if (channels > 1) {
          const float base_r = pink_core_r() * 0.15f;
          if (rndf_r < 0.0006f)
            this->rain_drop_r_ = 0.4f + 0.3f * rndf_r * 1000.0f;
          const float drop_r = this->rain_drop_r_ * (w_r > 0.0f ? 1.0f : -1.0f);
          this->rain_drop_r_ *= 0.97f;
          out_r = (base_r * 0.8f + drop_r * 0.4f) * 0.35f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::CAMPFIRE: {
        const float base_l = brown_core_l() * 1.3f;
        if (rndf_l < 0.00025f) {
          this->campfire_crackle_l_ = 0.7f + 0.3f * rndf_l * 2000.0f;
          this->campfire_freq_l_ = 800.0f + 1400.0f * (float)(rng_l & 0xFF) / 255.0f;
        }
        float pop_l = 0.0f;
        if (this->campfire_crackle_l_ > 0.005f) {
          pop_l = this->campfire_crackle_l_ * std::sin(this->campfire_phase_l_);
          this->campfire_phase_l_ += tau2pi * this->campfire_freq_l_ / rate;
          this->campfire_crackle_l_ *= 0.94f;
        }
        out_l = (base_l + pop_l * 0.6f) * 0.45f;

        if (channels > 1) {
          const float base_r = brown_core_r() * 1.3f;
          if (rndf_r < 0.00025f) {
            this->campfire_crackle_r_ = 0.7f + 0.3f * rndf_r * 2000.0f;
            this->campfire_freq_r_ = 800.0f + 1400.0f * (float)(rng_r & 0xFF) / 255.0f;
          }
          float pop_r = 0.0f;
          if (this->campfire_crackle_r_ > 0.005f) {
            pop_r = this->campfire_crackle_r_ * std::sin(this->campfire_phase_r_);
            this->campfire_phase_r_ += tau2pi * this->campfire_freq_r_ / rate;
            this->campfire_crackle_r_ *= 0.94f;
          }
          out_r = (base_r + pop_r * 0.6f) * 0.45f;
        } else {
          out_r = out_l;
        }
        break;
      }
      case NoiseVariant::HEARTBEAT: {
        const uint32_t beat_period = (uint32_t)(rate * 0.923f); // ~65 bpm
        const uint32_t pos = (uint32_t)(t % beat_period);
        const float fpos = (float)pos;
        const float dub_offset = 0.28f * rate;
        const float fdub = fpos - dub_offset;
        float s = 0.0f;
        const float lub_len = 0.14f * rate;
        const float dub_len = 0.10f * rate;
        if (fpos < lub_len) {
          const float env = std::sin(3.14159265f * fpos / lub_len);
          s = std::sin(tau2pi * 60.0f * fpos / rate) * env * 0.65f;
        } else if (fdub >= 0.0f && fdub < dub_len) {
          const float env = std::sin(3.14159265f * fdub / dub_len);
          s = std::sin(tau2pi * 85.0f * fdub / rate) * env * 0.45f;
        }
        out_l = out_r = s;
        break;
      }
      case NoiseVariant::BEEP: {
        if (t < this->beep_len_) {
          const float dur = (float) this->beep_len_;
          float env = 1.0f;
          const float fade = 0.004f * rate;
          if (ft < fade)
            env = ft / fade;
          else if (ft > dur - fade)
            env = (dur - ft) / fade;
          out_l = (0.42f * std::sin(tau2pi * 880.0f * ft / rate) +
                   0.12f * std::sin(tau2pi * 1760.0f * ft / rate)) *
                  env;
          out_r = out_l;
        }
        break;
      }
    }

    // Tone filter (one-pole low-pass)
    if (this->tone_ < 0.999f) {
      const float alpha = 0.05f + 0.90f * this->tone_ * this->tone_;
      this->tone_lp_l_ += alpha * (out_l - this->tone_lp_l_);
      out_l = this->tone_lp_l_;
      if (channels > 1) {
        this->tone_lp_r_ += alpha * (out_r - this->tone_lp_r_);
        out_r = this->tone_lp_r_;
      }
    }

    // Apply envelope gain
    out_l *= this->current_gain_;
    out_r *= this->current_gain_;

    // Clamp to prevent digital clipping
    if (out_l > 0.95f) out_l = 0.95f; else if (out_l < -0.95f) out_l = -0.95f;
    if (out_r > 0.95f) out_r = 0.95f; else if (out_r < -0.95f) out_r = -0.95f;

    samples[i * channels + 0] = (int16_t) (out_l * 32767.0f);
    if (channels > 1) {
      samples[i * channels + 1] = (int16_t) (out_r * 32767.0f);
    }
  }

  this->rng_l_ = rng_l;
  this->rng_r_ = rng_r;
}

}  // namespace esphome::noise
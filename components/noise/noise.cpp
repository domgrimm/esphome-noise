#include "noise.h"

#include "esphome/core/log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace esphome::noise {

static const char *const TAG = "noise";

void NoiseComponent::setup() {
  if (this->speaker_ == nullptr) {
    ESP_LOGE(TAG, "No speaker configured");
    this->mark_failed();
    return;
  }
  // Do not force a format here: we read the speaker's own stream info on play(),
  // so the component adapts to whatever the target device's speaker uses.
}

void NoiseComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Noise generator:\n  Sample Rate: %s\n  Channels: %u",
                this->sample_rate_ > 0 ? "override" : "speaker default", this->channels_);
}

void NoiseComponent::play(const std::string &variant) {
  this->y1_ = this->y2_ = this->y3_ = this->brown_ = 0.f;
  if (variant == "pink") {
    this->variant_ = NoiseVariant::PINK;
  } else if (variant == "brown") {
    this->variant_ = NoiseVariant::BROWN;
  } else if (variant == "gray") {
    this->variant_ = NoiseVariant::GRAY;
  } else {
    this->variant_ = NoiseVariant::WHITE;
  }

  // Follow the target speaker's format (rate + channel count).
  auto info = this->speaker_->get_audio_stream_info();
  uint32_t rate = info.get_sample_rate() ? info.get_sample_rate() : 16000;
  this->channels_ = info.get_channels() ? info.get_channels() : 1;
  if (info.get_bits_per_sample() != 0 && info.get_bits_per_sample() != 16) {
    ESP_LOGW(TAG, "Speaker uses %u-bit samples; generating 16-bit", info.get_bits_per_sample());
  }
  if (this->sample_rate_ > 0) {
    rate = this->sample_rate_;
    this->speaker_->set_audio_stream_info(audio::AudioStreamInfo(16, this->channels_, rate));
  }
  this->rate_ = rate;
  this->pcm_.resize(FRAMES_PER_CHUNK * this->channels_);

  if (!this->running_) {
    this->speaker_->start();
    this->stop_req_ = false;
    this->running_ = true;
    xTaskCreate(&NoiseComponent::noise_task_, "noise", 4096, this, 3, nullptr);
    ESP_LOGI(TAG, "Playing %s noise (%u Hz, %u ch)", variant.c_str(), (unsigned) this->rate_,
             this->channels_);
  } else {
    ESP_LOGI(TAG, "Switched noise to %s", variant.c_str());
  }
}

void NoiseComponent::stop() {
  if (!this->running_)
    return;
  this->running_ = false;
  this->stop_req_ = true;
  vTaskDelay(pdMS_TO_TICKS(30));
  this->speaker_->stop();
  ESP_LOGI(TAG, "Noise stopped");
}

void NoiseSelect::control(const std::string &value) {
  if (value == "Off") {
    this->parent_->stop();
    return;
  }
  std::string v;
  v.reserve(value.size());
  for (char c : value)
    v += (c >= 'A' && c <= 'Z') ? char(c + 32) : c;
  this->parent_->play(v);
}

void NoiseComponent::noise_task_(void *param) {
  auto *self = static_cast<NoiseComponent *>(param);
  self->task_loop_();
  vTaskDelete(nullptr);
}

void NoiseComponent::task_loop_() {
  while (!this->stop_req_) {
    this->generate_chunk_(this->pcm_.data(), FRAMES_PER_CHUNK);
    this->speaker_->play(reinterpret_cast<uint8_t *>(this->pcm_.data()),
                         this->pcm_.size() * sizeof(int16_t), pdMS_TO_TICKS(20));
  }
}

void NoiseComponent::generate_chunk_(int16_t *samples, size_t frames) {
  uint32_t rng = this->rng_;
  const uint8_t channels = this->channels_;
  for (size_t i = 0; i < frames; i++) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    const float w = (float) ((int16_t) (rng >> 16)) / 32768.0f;

    float out = 0.0f;
    switch (this->variant_) {
      case NoiseVariant::WHITE:
        out = w * 0.25f;
        break;
      case NoiseVariant::PINK: {
        this->y1_ += 0.05f * (w - this->y1_);
        this->y2_ += 0.22f * (w - this->y2_);
        this->y3_ += 0.50f * (w - this->y3_);
        out = (this->y1_ + this->y2_ + this->y3_) * 1.4f * 0.30f;
        break;
      }
      case NoiseVariant::BROWN:
        this->brown_ = (this->brown_ + 0.02f * w) / 1.02f;
        out = this->brown_ * 3.5f * 0.30f;
        break;
      case NoiseVariant::GRAY: {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        const float w2 = (float) ((int16_t) (rng >> 16)) / 32768.0f;
        out = 0.7f * (w + w2) / 1.4142f * 0.30f;
        break;
      }
    }

    if (out > 0.5f)
      out = 0.5f;
    else if (out < -0.5f)
      out = -0.5f;
    const int16_t s = (int16_t) (out * 32767.0f);
    for (uint8_t c = 0; c < channels; c++)
      samples[i * channels + c] = s;
  }
  this->rng_ = rng;
}

}  // namespace esphome::noise
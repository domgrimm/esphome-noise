# esphome-noise

Procedural noise generator for ESPHome — white / pink / brown / gray.

Hardware-agnostic: binds to any `speaker` platform (I2S DAC, amp chip, whatever)
and automatically follows that speaker's configured sample rate and channel
count, so it runs unchanged on almost any ESP32 with an audio output. It has no
pins, no network, no files — the noise is generated on the chip itself, so it
works fully offline and can play indefinitely (ideal for sleepers / 10-hour
white-noise flows).

## Install

```yaml
external_components:
  - source: github://domgrimm/esphome-noise
    components: [noise]
```

## Usage

```yaml
noise:
  speaker: i2s_audio_speaker
  # optional: override the speaker's native sample rate, e.g. 22050
  # sample_rate: 22050

# Trigger actions from buttons / automations / selects:
script:
  - id: noise_on
    sequence:
      - noise.start:
          id: my_noise
          variant: brown   # white | pink | brown | gray (templatable)
  - id: noise_off
    sequence:
      - noise.stop: my_noise
```

`variant` is templatable, so an `output.select` → `select.option` automation
can switch noise color live. See `example.yaml`.

## Actions

- `noise.start` — `id` (the noise component), optional `variant` (default `white`).
- `noise.stop` — `id` only.

## Notes

- Requires an ESP32-family chip (FreeRTOS task) and the `speaker` component.
- Generates 16-bit PCM. If the target speaker uses another bit depth, the
  component warns and still outputs 16-bit.
- Very light: a 1024-frame chunk is rendered per task loop against the
  speaker's own streaming buffer; add `buffer_duration` to the speaker if you
  hear glitches.
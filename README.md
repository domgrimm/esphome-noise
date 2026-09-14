# esphome-noise

Procedural sleep-sound generator for ESPHome — white / pink / brown / gray
noise plus on-device waves, wind, stream, a box-fan hum and an on-device alarm beep.

Hardware-agnostic: binds to any `speaker` platform (I2S DAC, amp chip, whatever)
and automatically follows that speaker's configured sample rate and channel
count, so it runs unchanged on almost any ESP32 with an audio output. It has no
pins, no network, no files — every sound is synthesized on the chip itself, so
it works fully offline and can play indefinitely (ideal for sleepers / 10-hour
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
  # optional: auto-create the "Noise" select entity (name gives it a HA entity)
  select:
    name: Noise
```

### Auto select entity

Add the optional `select:` block and the component creates the select for you —
options are auto-filled from the variant list (Off + all variants, exactly in
sync), and changing it starts/stops playback directly. No device-side
automation needed:

```yaml
noise:
  speaker: i2s_audio_speaker
  select:
    name: Noise
```

### Manual actions

Triggers can still call the actions from buttons / automations / scripts —
useful when you don't want a select entity:

```yaml
script:
  - id: noise_on
    then:
      - noise.start:
          id: my_noise
          variant: brown   # see Variants below (templatable)
  - id: noise_off
    then:
      - noise.stop:
          id: my_noise
```

`variant` is templatable, so an `output.select` → `select.option` automation
can switch sounds live. See `example.yaml`.

## Variants

| variant | sound |
|---|---|
| `white` | flat broadband hiss |
| `pink` | gentle 1/f hiss |
| `brown` | deep rumble, classic sleeping noise |
| `gray` | psychoacoustically flat hiss |
| `waves` | ocean surf: brown noise under slow swell LFOs |
| `wind` | low-pass filtered hiss with slowly drifting gusts |
| `stream` | bubbling water: pink noise, slow irregular modulation |
| `fan` | box-fan rumbly whoosh with blade wobble |
| `beep` | single loud 880 Hz alert beep (~250 ms), one-shot — ideal for on-device alerts |

All variants are synthesized in the same task loop against the speaker's own
streaming buffer (a few filters / LFOs / sine states per sample — negligible
at 16 kHz). Switch variants mid-playback with `noise.start`.

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
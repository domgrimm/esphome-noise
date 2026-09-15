# esphome-noise

Procedural sleep-sound generator for ESPHome — 12 offline synthetic soundscapes including classic noise colors, natural environments, sleeping aids, and an on-device alert beep.

Hardware-agnostic: binds to any `speaker` platform (I2S DAC, amp chip, internal DAC) and automatically adapts to that speaker's native sample rate and channels. No audio files, no network streaming, no flash storage needed — all sounds are synthesized directly on the ESP32 in real time.

---

## Features

- **12 Sound Profiles**: White, Pink, Brown, Gray, Ocean Waves, Gusting Wind, Babbling Stream, Box Fan, Rain Shower, Campfire, Rhythmic Heartbeat, and Alert Beep.
- **Pop-Free Playback**: Smooth sinusoidal/linear fade-in and fade-out envelopes eliminate clicks, pops, and sudden discontinuities on start, stop, and variant switching.
- **True Spatial Stereo**: When running on stereo speakers (`channels: 2`), Left and Right channels use independent PRNG generators and decorrelated phase LFOs for a wide, immersive spatial soundstage.
- **Rich Home Assistant Entities**:
  - `select`: Sound selector (`Off`, `White`, `Pink`, ..., `Rain`, `Campfire`, `Heartbeat`).
  - `media_player`: Native Home Assistant media player card integration with Play, Pause, Stop, Volume slider, and Power controls.
  - `volume` (`number`): Independent volume control (0–100%) without altering speaker hardware master gain.
  - `tone` (`number`): Acoustic low-pass filter (0–100%) to dial in deep mellow warmth vs. crisp presence.
  - `sleep_timer` (`number`): Configurable auto-off timer (0–180 minutes).
- **Voice Assistant Integration**: Built-in ducking (`noise.duck` / `noise.unduck`) and auto-pause (`noise.pause` / `noise.resume`) to temporarily lower noise level when wake words or TTS announcements occur.
- **ESP32 Multicore Optimized**: Audio task is automatically pinned to Core 0 on dual-core chips, leaving Core 1 free for WiFi, ESPHome main loop, and microWakeWord detection.

---

## Installation

```yaml
external_components:
  - source: github://domgrimm/esphome-noise
    components: [noise]
```

---

## Configuration

```yaml
noise:
  id: my_noise
  speaker: i2s_audio_speaker

  # Optional hardware overrides (default 0 = follow speaker settings):
  # sample_rate: 16000
  # channels: 2           # 1 = mono, 2 = true spatial stereo
  # fade_in_time: 150ms   # smooth start ramp
  # fade_out_time: 150ms  # smooth stop ramp

  # Optional Home Assistant Entities (any or all):
  select:
    name: "Noise Sound"
  media_player:
    name: "Noise Player"
  volume:
    name: "Noise Volume"
  tone:
    name: "Noise Tone"
  sleep_timer:
    name: "Noise Sleep Timer"

  # Optional Triggers:
  on_play:
    - logger.log: "Noise playback started"
  on_stop:
    - logger.log: "Noise playback stopped"
  on_variant_changed:
    - logger.log:
        format: "Sound changed to %s"
        args: [ 'variant.c_str()' ]
```

---

## Sound Profiles

| Variant | Description |
|---|---|
| `white` | Flat broadband hiss with equal energy across all frequencies |
| `pink` | Gentle $1/f$ balanced sound, soothing background hiss |
| `brown` | Deep $1/f^2$ low-frequency rumble, the classic sleep sound |
| `gray` | Psychoacoustically flat noise weighted for human hearing curves |
| `waves` | Ocean surf: brown noise modulated by slow rhythmic swell LFOs |
| `wind` | Low-pass filtered breeze with slowly drifting gusts and wobbles |
| `stream` | Babbling water brook: pink noise with irregular sinusoidal ripples |
| `fan` | Box fan: deep motor hum with 30 Hz blade rotation modulation |
| `rain` | Rain shower: steady diffuse rainfall base with randomized droplet impacts |
| `campfire` | Cozy hearth: warm low-frequency bed with randomized wooden crackles and pops |
| `heartbeat` | Soothing rhythmic 65 BPM "lub-dub" double pulse (ideal for nurseries and pets) |
| `beep` | Single loud 880 Hz alert beep (~250 ms) one-shot for on-device notifications |

---

## Actions

### `noise.start`
Starts noise playback. Accepts optional parameters:
```yaml
- noise.start:
    id: my_noise
    variant: campfire   # white, pink, brown, gray, waves, wind, stream, fan, rain, campfire, heartbeat, beep (templatable)
    duration: 30min     # auto-off duration (optional, templatable)
    volume: 75%         # playback volume (optional, templatable)
```

### `noise.stop`
Smoothly fades out and stops playback:
```yaml
- noise.stop:
    id: my_noise
```

### `noise.pause` & `noise.resume`
Pause playback with smooth fade-out and resume where you left off:
```yaml
- noise.pause:
    id: my_noise
- noise.resume:
    id: my_noise
```

### `noise.duck` & `noise.unduck`
Temporarily drops volume during voice assistant listening or announcements:
```yaml
# When wake word detected:
- noise.duck:
    id: my_noise
    level: 20%  # default 20%

# When assistant finishes speaking:
- noise.unduck:
    id: my_noise
```

### `noise.set_volume`
Adjusts noise gain without touching speaker DAC/master amplifier volume:
```yaml
- noise.set_volume:
    id: my_noise
    volume: 60%  # templatable
```

### `noise.set_tone`
Adjusts the acoustic low-pass filter (0% = warm & deep, 100% = crisp full spectrum):
```yaml
- noise.set_tone:
    id: my_noise
    tone: 40%  # templatable
```

---

## Voice Assistant Automation Example

Integrate ducking seamlessly with the ESPHome Voice Assistant pipeline:

```yaml
voice_assistant:
  on_listening:
    - noise.duck:
        id: my_noise
        level: 15%
  on_end:
    - noise.unduck:
        id: my_noise
  on_error:
    - noise.unduck:
        id: my_noise
```

---

## License

MIT License. See [LICENSE](LICENSE) for details.
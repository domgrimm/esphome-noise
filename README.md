# esphome-noise

Procedural sleep-sound generator for ESPHome — 12 offline synthetic soundscapes including classic noise colors, natural environments, sleeping aids, and an on-device alert beep.

Hardware-agnostic: binds to any standard ESPHome `speaker` platform (I2S DAC, internal DAC, resampler, mixer) **or directly to an `esphome-airplay2` receiver**. No audio files, no network streaming, no flash storage needed — all sounds are synthesized directly on the ESP32 in real time.

---

## Features

- **12 Sound Profiles**: White, Pink, Brown, Gray, Ocean Waves, Gusting Wind, Babbling Stream, Box Fan, Rain Shower, Campfire, Rhythmic Heartbeat, and Alert Beep.
- **Pop-Free Playback**: Smooth sinusoidal/linear fade-in and fade-out envelopes eliminate clicks, pops, and sudden discontinuities on start, stop, pause, resume, and variant switching.
- **True Spatial Stereo**: When running on stereo speakers (`channels: 2` or AirPlay), Left and Right channels use independent PRNG generators and decorrelated phase LFOs for a wide, immersive spatial soundstage.
- **AirPlay 2 Compatibility**: Direct output to [`henriklied/esphome-airplay2`](https://github.com/henriklied/esphome-airplay2) receivers without I2S pin conflicts. Automatically yields and pauses during AirPlay music streams, then smoothly resumes when music stops.
- **Multi-Source Audio Coordination**: Automatically duck or pause background noise whenever other media players, announcements, or voice assistant pipelines on the device become active.
- **Mobile-First Interactive Web Dashboard**: Optional route-hijacking rewrite of ESPHome's web server at `/`. Delivers a sleek dark-mode, touch-optimized dashboard dedicated to the sound machine with 12 tactile sound profile cards, smooth volume & tone sliders, sleep timer with live countdown, and one-tap quick presets.
- **Full `webserver-listcomponents` Compatibility**: Seamlessly compatible with [`esphome-webserver-listcomponents`](https://github.com/domgrimm/esphome-webserver-listcomponents). All device entities (sensors, switches, lights, numbers, etc.) remain fully surfaced in an interactive, collapsible components drawer with live SSE synchronization, while `/components` API requests remain unintercepted.
- **Rich Home Assistant Entities**:
  - `select`: Sound selector (`Off`, `White`, `Pink`, ..., `Rain`, `Campfire`, `Heartbeat`).
  - `media_player`: Native Home Assistant media player card integration with Play, Pause, Stop, Volume slider, and Power controls.
  - `volume` (`number`): Independent volume control (0–100%) without altering speaker hardware master gain.
  - `tone` (`number`): Acoustic low-pass filter (0–100%) to dial in deep mellow warmth vs. crisp presence.
  - `sleep_timer` (`number`): Configurable auto-off timer (0–1440 minutes).
  - `time_left` (`sensor`): Real-time countdown timer showing remaining minutes before auto-shutoff.
- **ESP32 Multicore Optimized**: Audio synthesis task is automatically pinned to Core 0 on dual-core chips, leaving Core 1 free for WiFi, ESPHome main loop, and microWakeWord detection.

---

## Installation

```yaml
external_components:
  - source: github://domgrimm/esphome-noise
    components: [noise]
```

---

## Quick Start

### Standard Speaker Setup
```yaml
noise:
  id: my_noise
  speaker: i2s_audio_speaker

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
```

### AirPlay 2 Receiver Setup
Run both AirPlay 2 streaming and offline white noise on the same ESP32 board without hardware pin conflicts:

```yaml
external_components:
  - source: github://henriklied/esphome-airplay2
    components: [airplay_receiver]
  - source: github://domgrimm/esphome-noise
    components: [noise]

# AirPlay receiver controls the hardware I2S pins:
airplay_receiver:
  id: airplay
  name: "Living Room Speaker"
  i2s_bclk_pin: 14
  i2s_lrclk_pin: 15
  i2s_dout_pin: 16

# Noise component outputs directly through the AirPlay audio backend:
noise:
  id: my_noise
  airplay_receiver: airplay
  select:
    name: "Noise Sound"
  media_player:
    name: "Noise Machine"
```

---

## Multi-Source Audio Coordination

When running background noise alongside announcements, TTS alerts, or other media players, `esphome-noise` can automatically coordinate volume levels without complex manual automations:

```yaml
noise:
  speaker: hardware_speaker

  # Automatically duck noise volume to 15% whenever any of these players are active:
  duck_on_media_players:
    - alert_player
    - tts_player
  duck_level: 15%

  # Or automatically pause noise completely while a high-priority player streams:
  pause_on_media_players:
    - airplay
```

When an external player starts playing, `esphome-noise` smoothly ramps down. When all external sources return to idle/stop, noise smoothly ramps back up to its previous volume.

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

## Configuration Variables

| Option | Type | Default | Description |
|---|---|---|---|
| `speaker` | Optional, ID | | The ID of the `speaker` component to output audio to. (Required unless `airplay_receiver` is set). |
| `airplay_receiver` | Optional, ID | | The ID of an `airplay_receiver` component to output audio directly into the AirPlay 2 backend. |
| `duck_on_media_players` | Optional, list of IDs | | Media players that trigger automatic noise ducking when active. |
| `pause_on_media_players` | Optional, list of IDs | | Media players that trigger automatic noise pausing when active. |
| `duck_level` | Optional, percentage | `20%` | Volume multiplier when ducked. |
| `sample_rate` | Optional, int | `0` | Sample rate in Hz. `0` follows speaker/airplay rate (e.g. 16000, 44100). |
| `channels` | Optional, int | `0` | Audio channels. `0` follows backend, `1` forces mono, `2` enables true spatial stereo decorrelation. |
| `initial_volume` | Optional, percentage | `100%` | Initial internal playback gain (0%–100%). |
| `fade_in_time` | Optional, time | `100ms` | Ramp-up duration when playback starts or resumes. |
| `fade_out_time` | Optional, time | `100ms` | Ramp-down duration when playback stops or pauses. |
| `select` | Optional, Schema | | Auto-creates a `select` entity listing `Off` and all sound profiles. |
| `media_player` | Optional, Schema | | Auto-creates a `media_player` entity with Play/Pause/Stop/Volume/Power controls. |
| `volume` | Optional, Schema | | Auto-creates a `number` entity (0%–100%) for internal volume gain. |
| `tone` | Optional, Schema | | Auto-creates a `number` entity (0%–100%) for the acoustic low-pass filter. |
| `sleep_timer` | Optional, Schema | | Auto-creates a `number` entity (0–1440 min) for automatic off timers. |
| `time_left` | Optional, Schema | | Auto-creates a `sensor` entity (duration in min) for remaining sleep timer countdown. |
| `web_server` | Optional, boolean or Schema | | Optional mobile-first interactive dashboard rewrite at `/`. |
| `on_play` | Optional, Automation | | Triggered when noise starts playing. |
| `on_stop` | Optional, Automation | | Triggered when noise stops. |
| `on_variant_changed` | Optional, Automation | | Triggered when sound profile changes (passes `std::string variant`). |

---

## Actions

### `noise.start`
Starts noise playback:
```yaml
- noise.start:
    id: my_noise
    variant: campfire   # Any variant name (templatable)
    duration: 30min     # Auto-off duration (optional, templatable)
    volume: 75%         # Playback volume (optional, templatable)
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
- noise.duck:
    id: my_noise
    level: 20%  # Target duck volume (default 20%, templatable)

- noise.unduck:
    id: my_noise
```

### `noise.set_volume`
Adjusts internal generator volume without touching hardware master amplifier gain:
```yaml
- noise.set_volume:
    id: my_noise
    volume: 60%  # 0% to 100% (templatable)
```

### `noise.set_tone`
Adjusts the acoustic low-pass filter (0% = warm & deep rumble, 100% = crisp full spectrum):
```yaml
- noise.set_tone:
    id: my_noise
    tone: 40%  # 0% to 100% (templatable)
```

### `noise.set_sleep_timer`
Sets the auto-off sleep timer duration in minutes (`0` cancels the active timer):
```yaml
- noise.set_sleep_timer:
    id: my_noise
    sleep_timer: 45  # Duration in minutes (templatable)
```

---

## Mobile-First Web Dashboard (Route Hijacking)

`esphome-noise` includes an optional, mobile-first rewrite of the device's web server dashboard. When enabled, visiting `http://<device-ip>/` presents a dark-mode, tactile dashboard focused completely on the noise generator:

- **12 Interactive Sound Cards**: Quick selection across all 12 sound profiles with active state indicators and volume metering.
- **Precision Sliders**: Master volume slider with mute toggle and step buttons, tone/low-pass acoustic shaping slider.
- **Sleep Timer & Live Countdown**: Sleep timer presets, expandable custom duration drawer, and synchronized live countdown display.
- **One-Tap Quick Presets**: Deep Sleep, Rainy Night, Deep Focus, and Ocean Calm.
- **Embedded & 100% Offline**: Minified and compressed (<8.5 KB gzip) directly in ESP32 flash (`NOISE_INDEX_HTML_GZ` PROGMEM array). Zero external CDNs or internet access required.
- **Direct REST API**: Provides `/api/noise` (GET status, POST control) for instant JSON control.

### Route Hijacking & `webserver-listcomponents` Compatibility

The custom web server uses route hijacking via `web_server_base::add_handler` registered with high setup priority (`setup_priority::WIFI`). It exclusively intercepts `GET /` and `GET /index.html`, leaving all other paths completely untouched:

- `/components` continues directly to [`esphome-webserver-listcomponents`](https://github.com/domgrimm/esphome-webserver-listcomponents).
- `/events` continues directly to ESPHome's Server-Sent Events stream.
- `/switch/*`, `/select/*`, `/number/*`, `/light/*`, and `/button/*` continue to standard ESPHome REST endpoints.

**All components remain surfaced**: The mobile dashboard queries `/components` (provided by `webserver_listcomponents`) to populate an expandable **"Device Components"** drawer. You can toggle switches, adjust numbers, trigger buttons, and read sensors directly from the dashboard, with real-time state synchronization via `/events`.

### YAML Configuration

```yaml
external_components:
  - source: github://domgrimm/esphome-noise
    components: [noise]
  - source: github://domgrimm/esphome-webserver-listcomponents
    components: [webserver_listcomponents]

web_server:
  port: 80

webserver_listcomponents:

noise:
  speaker: hardware_speaker
  web_server:
    title: "Bedside Noise Machine" # Optional: custom page title
    show_components: true          # Optional: surfaces all device entities (default: true)
```

Or simply:
```yaml
noise:
  speaker: hardware_speaker
  web_server: true
```

---

## Examples

Check the [`examples/`](examples/) directory for complete, verified configurations:

- [**`example.yaml`**](example.yaml): General ESP32 + I2S speaker setup with all entities and a physical button.
- [**`examples/web_dashboard_sound_machine.yaml`**](examples/web_dashboard_sound_machine.yaml): **Mobile-First Web Dashboard with `webserver-listcomponents`**: Dedicated interactive sound machine web UI at `/` with all device components surfaced.
- [**`examples/airplay_with_noise.yaml`**](examples/airplay_with_noise.yaml): **AirPlay 2 Receiver with Integrated Noise Generator** on the same ESP32 board without I2S pin conflicts and with automatic pause/resume coordination.
- [**`examples/multi_source_audio_mixer.yaml`**](examples/multi_source_audio_mixer.yaml): **Multi-Source Audio** configuration with multi-instance noise, background ambient sound, and automatic ducking during alerts.
- [**`examples/bedside_sound_machine.yaml`**](examples/bedside_sound_machine.yaml): Sleep-aid machine with rotary encoder volume knob, sound button, and synchronized auto-dimming nightlight.
- [**`examples/voice_assistant_ducking.yaml`**](examples/voice_assistant_ducking.yaml): Voice assistant pipeline integration automatically ducking noise during wake word detection and speech.

---

## License

MIT License. See [LICENSE](LICENSE) for details.
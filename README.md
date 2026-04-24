# Quartz 🎵

A lightweight, header-only C++ audio engine for real-time playback, mixing, and DSP effects. Quartz provides a node-based signal graph where audio sources, mixers, and effects can be chained together and routed to your output device.

---

## Features

- **Multi-format playback** — WAV, MP3, OGG (Vorbis), and FLAC
- **Two playback modes** — `Sound` (fully pre-loaded into memory) and `Music` (streamed from disk)
- **Node-based signal graph** — chain mixers, effects, and sources in any topology
- **Real-time DSP effects** — biquad filters, Linkwitz-Riley filters, distortion/waveshaping
- **Playback control** — play, pause, stop, loop, and variable playback speed with linear interpolation resampling
- **Header-only** — easy to integrate via CMake

---

## Requirements

- C++17 or later
- CMake 3.31+
- [miniaudio](https://miniaud.io/) (included in `vendor/`)
- [dr_libs](https://github.com/mackron/dr_libs) — dr_wav, dr_mp3, dr_flac (included in `vendor/`)
- [stb_vorbis](https://github.com/nothings/stb) (included in `vendor/`)
- [base-layer](https://github.com/nicomoraes/base-layer) (git submodule in `vendor/base-layer/`)

---

## Installation

### Clone with submodules

```bash
git clone --recurse-submodules https://github.com/your-username/quartz.git
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
# or use the helper script:
./update_submodules.sh
```

### Add to your CMake project

```cmake
add_subdirectory(quartz)
target_link_libraries(your_target PRIVATE Quartz)
```

---

## Quick Start

```cpp
#include <quartz.h>

#define SAMPLE_RATE 48000
#define CHANNELS    2

int main() {
    AudioManager quartz;
    quartz.init(SAMPLE_RATE, CHANNELS);

    // Sound: fully loaded into memory — best for short clips
    Quartz::Sound sfx("assets/click.wav", Quartz::WAV);

    // Music: streamed from disk — best for long tracks
    Quartz::Music bgm;
    bgm.load("assets/theme.ogg", Quartz::OGG);
    bgm.loop = true;

    // Route both into the master bus
    quartz.master().addInput(&sfx);
    quartz.master().addInput(&bgm);

    sfx.play();
    bgm.play();

    // Keep alive
    while (true) { /* your app loop */ }
}
```

---

## Core Concepts

### AudioManager

The top-level engine. Initializes the audio device via miniaudio and owns the **master bus** — the final mix that goes to your speakers.

```cpp
AudioManager quartz;
quartz.init(48000, 2); // sample rate, channels

Quartz::Mixer& master = quartz.master();
```

### Signal Graph

Quartz uses a pull-based node graph. Each `AudioNode` implements `getSamples()`, which is called recursively from the master bus down through the graph on every audio callback.

```
Master Bus (Mixer)
  └── Insert Effect (e.g. LR4 filter)
        └── Sub-bus (Mixer)
              ├── Music (streamed)
              └── Sound (buffered)
```

Build graphs by connecting nodes:

```cpp
Quartz::Mixer subBus;
Quartz::LR4 lowpass(quartz.getSampleRate(), quartz.getNumChannels());
lowpass.setFreq(8000.f);

quartz.master().addInput(&lowpass); // master pulls from filter
lowpass.setInput(&subBus);          // filter pulls from sub-bus
subBus.addInput(&mySound);          // sub-bus mixes sources
```

Print the current graph for debugging:

```cpp
quartz.master().print();
```

---

## Audio Sources

### `Quartz::Sound`

Decodes the entire file into memory on load. Best for short, frequently-triggered sounds (UI clicks, SFX).

```cpp
Quartz::Sound sfx("assets/shoot.wav", Quartz::WAV);

sfx.play();
sfx.pause();
sfx.stop();       // stops and resets playhead
sfx.playStop();   // toggles between play and stop
sfx.playPause();  // toggles between play and pause

sfx.loop = true;
sfx.setPlaybackSpeed(1.5f); // 1.0 = normal, 2.0 = double speed

uint64_t frames = sfx.getLength(); // total frames in file
```

**Supported formats:** `Quartz::WAV`, `Quartz::MP3`, `Quartz::OGG`, `Quartz::FLAC`

### `Quartz::Music`

Streams audio from disk frame-by-frame. Best for long background music tracks. Uses linear interpolation for resampling and playback speed changes.

```cpp
Quartz::Music bgm;
bgm.load("assets/theme.flac", Quartz::FLAC);

bgm.loop = true;
bgm.setPlaybackSpeed(0.8f);
bgm.play();

float speed = bgm.getPlaybackSpeed();
```

---

## DSP Effects

All effects extend `InsertEffect` and implement `processSample()`. They plug into the graph via `setInput()`.

### Biquad Filters (`RBJ`)

A standard RBJ biquad with selectable filter type:

```cpp
Quartz::RBJ filter(sampleRate, channels);
filter.setFilterType(Quartz::BiquadType::LOWPASS); // default
filter.setFreq(2000.f); // Hz
filter.setQ(0.707f);    // Q factor

// Available types:
// BiquadType::LOWPASS, HIGHPASS, BANDPASS, NOTCH,
// PEAKING, HIGH_SHELF, LOW_SHELF, ALL_PASS
```

### Linkwitz-Riley 4th Order Filter (`LR4`)

Two cascaded RBJ stages for a steeper, phase-aligned response:

```cpp
Quartz::LR4 crossover(sampleRate, channels);
crossover.setFreq(10000.f);
crossover.setFilterType(Quartz::BiquadType::HIGHPASS);
```

### Distortion / Waveshapers

All distortion types share `setDrive()` and `setBias()`:

| Class | Description |
|---|---|
| `RectifierHalf` | Half-wave rectifier (clips negative signal to 0) |
| `RectifierFull` | Full-wave rectifier (absolute value) |
| `HardClip` | Hard clip at a configurable threshold |
| `TanhShaper` | Smooth saturation via `tanh` |
| `SoftClipper` | Soft knee clipper with configurable threshold and knee |

```cpp
Quartz::TanhShaper sat(sampleRate, channels);
sat.setDrive(2.0f); // pre-gain before waveshaper

Quartz::HardClip clip(sampleRate, channels);
clip.setThreshold_linear(0.7f);

Quartz::SoftClipper soft(sampleRate, channels);
soft.setThreshold_linear(0.8);
soft.setKnee_dB(6.0);
```

---

## Full Example

```cpp
#include <quartz.h>

int main() {
    AudioManager quartz;
    quartz.init(48000, 2);

    Quartz::Music music;
    music.load("assets/song.ogg", Quartz::OGG);
    music.loop = true;

    Quartz::Sound hit("assets/hit.wav", Quartz::WAV);

    // Build signal chain: master -> clipper -> filter -> sub-bus -> sources
    Quartz::Mixer subBus;
    Quartz::LR4 filter(quartz.getSampleRate(), quartz.getNumChannels());
    filter.setFreq(15000.f);
    Quartz::RectifierHalf clipper(quartz.getSampleRate(), quartz.getNumChannels());

    quartz.master().addInput(&clipper);
    clipper.setInput(&filter);
    filter.setInput(&subBus);
    subBus.addInput(&music);
    subBus.addInput(&hit);

    music.play();

    while (true) {
        char c = getchar();
        if (c == 'q') break;
        if (c == 's') hit.playStop();
        if (c == 'm') music.playPause();
        if (c == '+') music.setPlaybackSpeed(music.getPlaybackSpeed() + 0.1f);
        if (c == '-') music.setPlaybackSpeed(music.getPlaybackSpeed() - 0.1f);
    }
}
```

---

## Project Structure

```
quartz/
├── quartz.h              # Main entry point — include this
├── include/
│   ├── AudioNode.h       # Base class for all graph nodes
│   ├── AudioNodeCollection.h  # Includes all nodes/effects
│   ├── Mixer.h           # Multi-input mixer node
│   ├── Sound.h           # Buffered audio source
│   ├── Music.h           # Streamed audio source
│   ├── InsertEffect.h    # Base class for insert effects
│   ├── Biquad.h          # RBJ biquad and LR4 filters
│   └── Distortion.h      # Waveshaping/distortion effects
├── vendor/
│   ├── miniaudio.h       # Audio device I/O
│   ├── dr_wav.h          # WAV decoding
│   ├── dr_mp3.h          # MP3 decoding
│   ├── dr_flac.h         # FLAC decoding
│   ├── stb_vorbis.c      # OGG/Vorbis decoding
│   └── base-layer/       # Logging and utilities (submodule)
├── testing/
│   └── main.cpp          # Interactive test app
└── CMakeLists.txt
```

---

## License

See [LICENSE](LICENSE) for details.
# QUARTZ

A lightweight, header-only C++ audio engine built on [miniaudio](https://miniaud.io).

Quartz gives you a node-based audio graph — mix sounds, stream music, and chain effects — with a single `init()` call to open the output device.

---

## Features

- **Node graph architecture** — composable `AudioNode` tree: mixers, sounds, music streams, and effects chain together freely
- **Sound playback** — load and play `.ogg` files fully decoded into memory; ideal for short clips and SFX
- **Music streaming** — stream `.ogg` files from disk frame-by-frame; suitable for long tracks and background music
- **Per-input gain** — each connection into a `Mixer` carries its own gain value, plus a master gain on the mixer itself
- **Insert effects** — chainable `InsertEffect` base class; `HardClip` included out of the box
- **Looping** — loop flag on both `Sound` and `Music` nodes
- **Graph inspection** — `print()` on any node dumps the full subtree to stdout for debugging
- **miniaudio backend** — cross-platform audio output (Windows, macOS, Linux, iOS, Android) with no extra dependencies

---

## Installation

Don't even try at this point. I'm working on making it more user-friendly, but it's not there yet.

---

## Quick Start

```cpp
#include <quartz.h>

int main() {
    Quartz audio;
    audio.init(44100, 2); // 44.1 kHz stereo

    // Load a sound effect into memory
    Sound* sfx = new Sound();
    sfx->load("assets/explosion.ogg");

    // Stream background music from disk
    Music* bgm = new Music();
    bgm->load("assets/theme.ogg");

    // Add both to the master bus
    audio.master().addInput(sfx, 0.8f); // 80% gain
    audio.master().addInput(bgm, 0.5f); // 50% gain

    // Keep the process alive while audio plays
    std::this_thread::sleep_for(std::chrono::seconds(10));

    delete sfx;
    delete bgm;
    return 0;
}
```

---

## Node Graph

Quartz's audio pipeline is a tree of `AudioNode` objects. The `Quartz` device owns a master `Mixer` that is called every audio callback. You build a graph by connecting nodes into it:

```
Quartz device
└── Mixer (master bus)
    ├── [gain=1.0] Music  ← streaming ogg
    ├── [gain=0.8] Sound  ← in-memory ogg
    └── [gain=1.0] HardClip
                   └── Sound  ← clipped SFX
```

Print any subtree for debugging:

```cpp
audio.master().print();
// Mixer (inputs=3)
//   gain=1
//     Music
//   gain=0.8
//     Sound
//   gain=1
//     HardClip (t=0.9)
//       Sound
```

---

## Chaining Effects

`InsertEffect` wraps another node and processes its output. Chain multiple effects by nesting them:

```cpp
Sound* sfx = new Sound();
sfx->load("assets/crunch.ogg");

HardClip* clip = new HardClip();
clip->threshold = 0.9f;
clip->setInput(sfx);

audio.master().addInput(clip, 1.0f);
```

Subclass `InsertEffect` to write your own:

```cpp
class Gain : public InsertEffect {
public:
    float amount = 1.0f;

    const char* getName() const override { return "Gain"; }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Gain (x" << amount << ")\n";
        if (input) input->print(depth + 1);
    }

    void process(float* buffer, int frames, int channels) override {
        for (int i = 0; i < frames * channels; ++i)
            buffer[i] *= amount;
    }
};
```

---

## API Reference

### `Quartz`

| Method | Description |
|---|---|
| `bool init(sampleRate, numChannels)` | Open the default playback device and start the audio thread |
| `Mixer& master()` | Returns a reference to the master bus mixer |

### `Mixer : AudioNode`

| Method | Description |
|---|---|
| `addInput(node, gain)` | Connect a node to this mixer with an optional per-input gain (default `1.0`) |
| `setGain(g)` | Set the master output gain of this mixer |

### `Sound : AudioNode`

Decodes an entire `.ogg` file into memory. Best for short clips.

| Method | Description |
|---|---|
| `bool load(filepath)` | Decode and load an `.ogg` file |
| `loop` | Set to `true` to loop playback (public field) |

### `Music : AudioNode`

Streams an `.ogg` file from disk. Best for long tracks.

| Method | Description |
|---|---|
| `bool load(filepath)` | Open an `.ogg` file for streaming |
| `loop` | Set to `true` to loop when the stream ends (default `true`, public field) |

### `InsertEffect : AudioNode` (base class)

| Method | Description |
|---|---|
| `setInput(node)` | Set the upstream node whose output this effect processes |
| `process(buffer, frames, channels)` | Override in subclasses to implement the effect |

### `HardClip : InsertEffect`

| Field | Description |
|---|---|
| `threshold` | Clips samples to `[-threshold, +threshold]` (default `1.0`) |

---

## Notes

- All nodes output **interleaved float** samples in the range `[-1.0, 1.0]`.
- `getSamples()` is called from miniaudio's **audio thread**. Node graph mutations (adding/removing inputs, loading files) from the main thread are not thread-safe — synchronize access if you need to modify the graph at runtime.
- `Sound` and `Music` start playing as soon as they are added to a mixer. There is no explicit `play()` call; control playback by adding/removing nodes from the graph.
- `Music` loops by default (`loop = true`). Set it to `false` before loading if you want one-shot streaming playback.
- Nodes are **not** owned by the mixer — you are responsible for their lifetime. Do not delete a node while it is still connected.

---

## License

MIT — see `LICENSE` for details.
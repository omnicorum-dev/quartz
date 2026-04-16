//
// Created by Nico Russo on 4/15/26.
//

#ifndef QUARTZ_H
#define QUARTZ_H

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <base.h>
#define STB_VORBIS_IMPLEMENTATION
#include <stb_vorbis.c>

using namespace omni;

inline void pcm16_to_float(const short* input, float* output, const int count) {
    for (int i = 0; i < count; ++i) {
        output[i] = input[i] / 32768.0f;
    }
}

inline void indent(int depth) {
    for (int i = 0; i < depth; ++i)
        std::cout << "  ";
}

class AudioNode {
public:
    virtual ~AudioNode() = default;

    virtual void getSamples(float* output, int numFrames, int numChannels) = 0;

    // for tree/graph visualization
    virtual void print(int depth = 0) const = 0;

    virtual const char* getName() const = 0;
};

struct Connection {
    AudioNode* node = nullptr;
    float gain = 1.0f;
};

class Mixer : public AudioNode {
private:
    std::vector<Connection> inputs;
    float masterGain = 1.0f;

public:
    void addInput(AudioNode* node, float gain = 1.0f) {
        inputs.push_back({ node, gain });
    }

    void setGain(float g) {
        masterGain = g;
    }

    const char* getName() const override {
        return "Mixer";
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Mixer (inputs=" << inputs.size() << ")\n";

        for (const auto& in : inputs) {
            if (!in.node) continue;

            for (int i = 0; i < depth + 1; ++i) std::cout << "  ";
            std::cout << "gain=" << in.gain << "\n";

            in.node->print(depth + 2);
        }
    }

    void getSamples(float* output, int numFrames, int numChannels) override {
        const int total = numFrames * numChannels;

        for (int i = 0; i < total; ++i)
            output[i] = 0.0f;

        std::vector<float> temp(total);

        for (const auto& in : inputs) {
            if (!in.node) continue;

            std::fill(temp.begin(), temp.end(), 0.0f);

            in.node->getSamples(temp.data(), numFrames, numChannels);

            for (int i = 0; i < total; ++i)
                output[i] += temp[i] * in.gain;
        }

        for (int i = 0; i < total; ++i)
            output[i] *= masterGain;
    }
};

class Sound : public AudioNode {
private:
    int channels = 0;
    int sampleRate = 0;

    int length = 0;
    float* audio = nullptr;

    int pos = 0;
    bool playing = true;
    bool loop = false;

public:
    ~Sound() {
        delete[] audio;
    }

    const char* getName() const override {
        return "Sound";
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Sound\n";
    }

    bool load(const std::string& file) {
        short* pcm = nullptr;

        length = stb_vorbis_decode_filename(
            file.c_str(),
            &channels,
            &sampleRate,
            &pcm
        );

        if (length <= 0 || !pcm) return false;

        const int total = length * channels;

        audio = new float[total];
        pcm16_to_float(pcm, audio, total);

        free(pcm);

        pos = 0;
        return true;
    }

    void getSamples(float* output, int numFrames, int numChannels) override {
        if (!playing || !audio) return;

        const int total = numFrames * numChannels;
        const int srcTotal = length * channels;

        for (int i = 0; i < total; ++i) {
            if (pos >= srcTotal) {
                if (loop) pos = 0;
                else { playing = false; break; }
            }

            output[i] += audio[pos++];
        }
    }
};

class Music : public AudioNode {
private:
    stb_vorbis* vorbis = nullptr;
    int channels = 0;
    bool loop = true;
    bool playing = true;

    static constexpr int MAX_FRAMES = 1024;
    static constexpr int MAX_CH = 2;

public:
    ~Music() {
        if (vorbis) stb_vorbis_close(vorbis);
    }

    const char* getName() const override {
        return "Music";
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Music\n";
    }

    bool load(const std::string& file) {
        int err = 0;
        vorbis = stb_vorbis_open_filename(file.c_str(), &err, nullptr);
        if (!vorbis || err) return false;

        channels = stb_vorbis_get_info(vorbis).channels;
        return true;
    }

    void getSamples(float* output, int numFrames, int numChannels) override {
        if (!playing || !vorbis) return;

        short pcm[MAX_FRAMES * MAX_CH];
        float temp[MAX_FRAMES * MAX_CH];

        int framesLeft = numFrames;

        while (framesLeft > 0) {
            int block = std::min(framesLeft, MAX_FRAMES);

            int framesRead = stb_vorbis_get_samples_short_interleaved(
                vorbis,
                channels,
                pcm,
                block * channels
            );

            if (framesRead == 0) {
                if (loop) {
                    stb_vorbis_seek_start(vorbis);
                    continue;
                } else {
                    playing = false;
                    break;
                }
            }

            int samples = framesRead * channels;

            pcm16_to_float(pcm, temp, samples);

            for (int i = 0; i < samples; ++i)
                output[i] += temp[i];

            framesLeft -= framesRead;
            output += samples;
        }
    }
};

class InsertEffect : public AudioNode {
protected:
    AudioNode* input = nullptr;

public:
    void setInput(AudioNode* in) {
        input = in;
    }

    const char* getName() const override {
        return "Effect";
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "Effect\n";

        if (input)
            input->print(depth + 1);
    }

    virtual void process(float* buffer, int frames, int channels) = 0;

    void getSamples(float* output, int frames, int channels) override {
        const int total = frames * channels;

        for (int i = 0; i < total; ++i)
            output[i] = 0.0f;

        if (input)
            input->getSamples(output, frames, channels);

        process(output, frames, channels);
    }
};

class HardClip : public InsertEffect {
public:
    float threshold = 1.0f;

    const char* getName() const override {
        return "HardClip";
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "HardClip (t=" << threshold << ")\n";

        if (input)
            input->print(depth + 1);
    }

    void process(float* buffer, int frames, int channels) override {
        const int total = frames * channels;

        for (int i = 0; i < total; ++i)
            buffer[i] = std::clamp(buffer[i], -threshold, threshold);
    }
};

class Quartz {
private:
    ma_result result{};
    ma_device_config deviceConfig{};
    ma_device device{};

    Mixer masterBuss;

    int numChannels = 0;
    int sampleRate = 0;

public:
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
        Quartz* self = static_cast<Quartz*>(pDevice->pUserData);

        self->masterBuss.getSamples((float*)pOutput, frameCount, self->numChannels);
    }

    Mixer& master() { return masterBuss; }

    bool init(int _sampleRate, int _numChannels) {
        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format =  ma_format_f32;
        deviceConfig.playback.channels = _numChannels;
        deviceConfig.sampleRate = _sampleRate;
        deviceConfig.dataCallback = data_callback;
        deviceConfig.pUserData = this;

        numChannels = _numChannels;
        sampleRate = _sampleRate;

        if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
            LOG_ERROR("Failed to open playback device");
            return false;
        }

        if (ma_device_start(&device) != MA_SUCCESS) {
            LOG_ERROR("Failed to start playback device.");
            ma_device_uninit(&device);
            return false;
        }

        return true;
    }
};

#endif //QUARTZ_H

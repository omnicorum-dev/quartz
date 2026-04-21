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

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

using namespace omni;

enum AudioFileFormat {
    WAV,
    MP3,
    OGG,
    FLAC
};

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

    virtual void getSamples(float* output, int numFrames, int numChannels, int sampleRate) = 0;

    // for tree/graph visualization
    virtual void print(int depth) const = 0;

    [[nodiscard]] virtual const char* getName() const = 0;
};

struct Connection {
    AudioNode* node = nullptr;
    float gain = 1.0f;
};

class Mixer : public AudioNode {
private:
    std::vector<Connection> inputs;
    float masterGain = 1.0f;

    std::string name;
    mutable std::string cachedName;

public:
    void addInput(AudioNode* node, float gain = 1.0f) {
        inputs.push_back({ node, gain });
    }

    void setGain(float g) {
        masterGain = g;
    }

    const char* getName() const override {
        cachedName = "Mixer: " + name;
        return cachedName.c_str();
    }

    void setName(const std::string& n) {
        name = n;
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << getName() << " (" << inputs.size() << " inputs)\n";

        for (const auto& in : inputs) {
            if (!in.node) continue;

            for (int i = 0; i < depth + 1; ++i) std::cout << "  ";
            std::cout << "gain=" << in.gain << "\n";

            in.node->print(depth + 2);
        }
    }

    void getSamples(float* output, int numFrames, int numChannels, int sampleRate) override {
        const int total = numFrames * numChannels;

        for (int i = 0; i < total; ++i)
            output[i] = 0.0f;

        std::vector<float> temp(total);

        for (const auto& in : inputs) {
            if (!in.node) continue;

            std::fill(temp.begin(), temp.end(), 0.0f);

            in.node->getSamples(temp.data(), numFrames, numChannels, sampleRate);

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
    int fileSampleRate = 0;
    AudioFileFormat format = OGG;

    uint64_t length = 0;
    float* audio = nullptr;

    std::string name;
    mutable std::string cachedName;

    int pos = 0;
    bool playing = false;

public:
    bool loop = false;

    uint64_t getLength() const { return length; }

    ~Sound() {
        delete[] audio;
    }

    const char* getName() const override {
        cachedName = "Sound: " + name;
        return cachedName.c_str();
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << getName() << std::endl;
    }

    bool loadOGG(const std::string& file) {
        short* pcm = nullptr;
        name = file;
        format = OGG;

        length = stb_vorbis_decode_filename(
            file.c_str(),
            &channels,
            &fileSampleRate,
            &pcm
        );

        omni::LOG_DEBUG("length: {}", length);

        if (length <= 0 || !pcm) return false;

        const int total = length * channels;

        audio = new float[total];
        pcm16_to_float(pcm, audio, total);

        free(pcm);

        pos = 0;
        return true;
    }

    bool loadWAV(const std::string& file) {
        name = file;
        format = WAV;

        uint64_t frameCount;
        uint32_t sr;
        uint32_t chan;

        audio = drwav_open_file_and_read_pcm_frames_f32(file.c_str(),
            &chan, &sr, &frameCount,
            nullptr);

        length = static_cast<int>(frameCount);
        channels = static_cast<int>(chan);
        fileSampleRate = static_cast<int>(sr);

        pos = 0;
        return true;
    }

    bool loadMP3(const std::string& file) {
        name = file;
        format = MP3;

        uint64_t frameCount;
        drmp3_config config;

        audio = drmp3_open_file_and_read_pcm_frames_f32(file.c_str(),
            &config, &frameCount,
            nullptr);

        length = static_cast<int>(frameCount);
        channels = static_cast<int>(config.channels);
        fileSampleRate = static_cast<int>(config.sampleRate);

        pos = 0;
        return true;
    }

    bool loadFLAC(const std::string& file) {
        name = file;
        format = FLAC;

        uint64_t frameCount;
        uint32_t sr;
        uint32_t chan;

        audio = drflac_open_file_and_read_pcm_frames_f32(file.c_str(),
            &chan, &sr, &frameCount,
            nullptr);

        length = static_cast<int>(frameCount);
        channels = static_cast<int>(chan);
        fileSampleRate = static_cast<int>(sr);

        pos = 0;
        return true;
    }

    bool load(const std::string& file, const AudioFileFormat fileFormat) {
        switch (fileFormat) {
            case WAV:
                return loadWAV(file);
            case MP3:
                return loadMP3(file);
            case FLAC:
                return loadFLAC(file);
            case OGG:
                return loadOGG(file);
        }
        return false;
    }

    void play()    { playing = true;  }
    void pause()   { playing = false; }
    void stop()    { playing = false; pos = 0; }
    void restart() { pos = 0; }

    void playPause() {
        if (playing) { pause(); }
        else         { play();  }
    }
    void playStop() {
        if (playing) { stop(); }
        else         { play();  }
    }

    void getSamples(float* output, const int numFrames, const int numChannels, int sampleRate) override {
        if (!playing || !audio) return;

        const int total = numFrames * numChannels;
        const uint64_t srcTotal = length * channels;

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
    drmp3* mp3 = nullptr;
    drwav* wav = nullptr;
    drflac* flac = nullptr;
    AudioFileFormat format = OGG;

    int channels = 0;
    int fileSampleRate = 0;
    bool playing = false;

    std::string name;
    mutable std::string cachedName;

    static constexpr int MAX_FRAMES = 1024;
    static constexpr int MAX_CH = 2;

public:
    bool loop = false;

    ~Music() override {
        if (vorbis) stb_vorbis_close(vorbis);
        if (flac) drflac_close(flac);
        if (mp3) free(mp3);
        if (wav) free(wav);
    }

    const char* getName() const override {
        cachedName = "Music: " + name;
        return cachedName.c_str();
    }

    void print(const int depth) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << getName() << std::endl;
    }

    bool loadOGG(const std::string& file) {
        name = file;
        format = OGG;

        int err = 0;
        vorbis = stb_vorbis_open_filename(file.c_str(), &err, nullptr);
        if (!vorbis || err) return false;

        channels = stb_vorbis_get_info(vorbis).channels;
        fileSampleRate = stb_vorbis_get_info(vorbis).sample_rate;
        return true;
    }

    bool loadWAV(const std::string& file) {
        name = file;
        format = WAV;

        wav = new drwav();

        if (!drwav_init_file(wav, file.c_str(), nullptr)) {
            return false;
        }

        channels = static_cast<int>(wav->channels);
        fileSampleRate = static_cast<int>(wav->sampleRate);

        return true;
    }

    bool loadMP3(const std::string& file) {
        name = file;
        format = MP3;

        mp3 = new drmp3();

        if (!drmp3_init_file(mp3, file.c_str(), nullptr)) {
            return false;
        }

        channels = static_cast<int>(mp3->channels);
        fileSampleRate = static_cast<int>(mp3->sampleRate);

        return true;
    }

    bool loadFLAC(const std::string& file) {
        name = file;
        format = FLAC;

        flac = new drflac();

        flac = drflac_open_file(file.c_str(), nullptr);
        if (!flac) {
            return false;
        }

        channels = flac->channels;
        fileSampleRate = flac->sampleRate;

        return true;
    }

    bool load(const std::string& file, const AudioFileFormat fileFormat) {
        switch (fileFormat) {
            case WAV:
                return loadWAV(file);
            case MP3:
                return loadMP3(file);
            case FLAC:
                return loadFLAC(file);
            case OGG:
                return loadOGG(file);
        }
        return false;
    }

    void play()  { playing = true; }
    void pause() { playing = false; }

    void stop() {
        if (vorbis) stb_vorbis_seek_start(vorbis);
        playing = false;
    }
    void restart() const {
        if (vorbis) stb_vorbis_seek_start(vorbis);
    }

    void playPause() {
        if (playing) { pause(); }
        else         { play();  }
    }
    void playStop() {
        if (playing) { stop(); }
        else         { play();  }
    }

    // Reads up to `frames` source frames into `out`, returns frames actually read.
    int readFrames(float* out, int frames) {
        switch (format) {
            case OGG: {
                short pcm[MAX_FRAMES * MAX_CH];
                int got = stb_vorbis_get_samples_short_interleaved(vorbis, channels, pcm, frames * channels);
                if (got > 0) pcm16_to_float(pcm, out, got * channels);
                return got;
            }
            case WAV:  return static_cast<int>(drwav_read_pcm_frames_f32 (wav,  frames, out));
            case MP3:  return static_cast<int>(drmp3_read_pcm_frames_f32  (mp3,  frames, out));
            case FLAC: return static_cast<int>(drflac_read_pcm_frames_f32 (flac, frames, out));
            default:   return 0;
        }
    }

    void seekToStart() {
        switch (format) {
            case OGG:  stb_vorbis_seek_start(vorbis);          break;
            case WAV:  drwav_seek_to_pcm_frame(wav,   0);      break;
            case MP3:  drmp3_seek_to_pcm_frame(mp3,   0);      break;
            case FLAC: drflac_seek_to_pcm_frame(flac, 0);      break;
        }
    }

    void getSamples(float* output, const int numFrames, const int numChannels, const int sampleRate) override {
        if (!playing ||
            (format == OGG  && !vorbis) ||
            (format == WAV  && !wav)    ||
            (format == MP3  && !mp3)    ||
            (format == FLAC && !flac)) return;

        // Ratio > 1 means the file has more samples per second than we need,
        // so we can read fewer source frames to fill each output frame.
        const double resampleRatio = static_cast<double>(fileSampleRate) / sampleRate;
        const int    srcFramesNeeded = std::max(1, static_cast<int>(std::ceil(numFrames * resampleRatio)));

        // Temp buffer sized for the source frames we'll actually decode.
        std::vector<float> srcBuf(srcFramesNeeded * channels);

        // ── 1.  Decode all source frames in one unified loop ─────────────────────
        int srcFramesRead = 0;
        int framesLeft    = srcFramesNeeded;
        float* dst        = srcBuf.data();

        while (framesLeft > 0) {
            const int block = std::min(framesLeft, MAX_FRAMES);
            int       got   = readFrames(dst, block);   // see helper below

            if (got == 0) {
                if (loop) { seekToStart(); continue; }
                playing = false;
                break;
            }

            srcFramesRead += got;
            framesLeft    -= got;
            dst           += got * channels;
        }

        if (srcFramesRead == 0) return;

        // ── 2.  Resample (linear interpolation) and mix into output ──────────────
        for (int outFrame = 0; outFrame < numFrames; ++outFrame) {
            // Where in the source buffer does this output frame map to?
            const double srcPos   = outFrame * resampleRatio;
            const int    srcIdx   = static_cast<int>(srcPos);
            const double frac     = srcPos - srcIdx;
            const int    srcIdxB  = std::min(srcIdx + 1, srcFramesRead - 1);

            for (int ch = 0; ch < numChannels; ++ch) {
                // Mix channels: if the file has fewer channels than requested,
                // wrap around (e.g. mono -> duplicate to all output channels).
                const int fileCh = ch % channels;

                const float a = srcBuf[srcIdx  * channels + fileCh];
                const float b = srcBuf[srcIdxB * channels + fileCh];

                output[outFrame * numChannels + ch] += static_cast<float>(a + frac * (b - a));
            }
        }
    }

    void getSamplesOld(float* output, const int numFrames, const int numChannels, int sampleRate) {
        if (!playing ||
            (format == OGG && !vorbis) ||
            (format == WAV && !wav) ||
            (format == MP3 && !mp3) ||
            (format == FLAC && !flac)) return;

        short pcm[MAX_FRAMES * MAX_CH];
        float temp[MAX_FRAMES * MAX_CH];

        int framesLeft = numFrames;

        while (framesLeft > 0) {
            int block = std::min(framesLeft, MAX_FRAMES);

            int framesRead = 0;

            switch (format) {
                case OGG: {
                    framesRead = stb_vorbis_get_samples_short_interleaved(
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
                    break;
                }
                case WAV: {
                    framesRead = static_cast<int>(drwav_read_pcm_frames_f32(wav, block, temp));

                    if (framesRead == 0) {
                        if (loop) {
                            drwav_seek_to_pcm_frame(wav, 0);
                            continue;
                        } else {
                            playing = false;
                            break;
                        }
                    }

                    const int samples = framesRead * channels;

                    for (int i = 0; i < samples; ++i)
                        output[i] += temp[i];

                    framesLeft -= framesRead;
                    output += samples;
                    break;
                }
                case MP3: {
                    framesRead = static_cast<int>(drmp3_read_pcm_frames_f32(mp3, block, temp));

                    if (framesRead == 0) {
                        if (loop) {
                            drmp3_seek_to_pcm_frame(mp3, 0);
                            continue;
                        } else {
                            playing = false;
                            break;
                        }
                    }

                    const int samples = framesRead * channels;

                    for (int i = 0; i < samples; ++i)
                        output[i] += temp[i];

                    framesLeft -= framesRead;
                    output += samples;
                    break;
                }
                case FLAC: {
                    framesRead = static_cast<int>(drflac_read_pcm_frames_f32(flac, block, temp));

                    if (framesRead == 0) {
                        if (loop) {
                            drflac_seek_to_pcm_frame(flac, 0);
                            continue;
                        } else {
                            playing = false;
                            break;
                        }
                    }

                    const int samples = framesRead * channels;

                    for (int i = 0; i < samples; ++i)
                        output[i] += temp[i];

                    framesLeft -= framesRead;
                    output += samples;
                    break;
                }
            }
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
        return "InsertEffect";
    }

    void print(int depth = 0) const override {
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "InsertEffect\n";

        if (input)
            input->print(depth + 1);
    }

    virtual void process(float* buffer, int frames, int channels) = 0;

    void getSamples(float* output, int frames, int channels, int sampleRate) override {
        const int total = frames * channels;

        for (int i = 0; i < total; ++i)
            output[i] = 0.0f;

        if (input)
            input->getSamples(output, frames, channels, sampleRate);

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

        self->masterBuss.getSamples((float*)pOutput, frameCount, self->numChannels, self->sampleRate);
    }

    Mixer& master() { return masterBuss; }

    bool init(const int _sampleRate, const int _numChannels) {
        deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format =  ma_format_f32;
        deviceConfig.playback.channels = _numChannels;
        deviceConfig.sampleRate = _sampleRate;
        deviceConfig.dataCallback = data_callback;
        deviceConfig.pUserData = this;

        numChannels = _numChannels;
        sampleRate = _sampleRate;

        masterBuss.setName("Master Bus");

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

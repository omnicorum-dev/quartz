//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_SOUND_H
#define QUARTZ_SOUND_H

#include <AudioNode.h>

namespace Quartz {
    class Sound : public AudioNode {
    private:
        int channels = 0;
        int fileSampleRate = 0;
        AudioFileFormat format = OGG;

        float playbackSpeed = 1.0f;
        int effectiveSampleRate = 0;

        uint64_t length = 0;
        float* audio = nullptr;

        std::string name;
        mutable std::string cachedName;

        double playhead = 0;
        bool playing = false;

    public:
        bool loop = false;

        uint64_t getLength() const { return length; }

        void setSampleRate(const int sampleRate) {
            fileSampleRate = sampleRate;
            effectiveSampleRate = fileSampleRate * playbackSpeed;
        }

        void setPlaybackSpeed(const float speed) {
            playbackSpeed = speed;
            effectiveSampleRate = fileSampleRate * speed;
        }

        Sound() = default;
        Sound(const std::string& file, const AudioFileFormat fileFormat) {
            load(file, fileFormat);
        }
        ~Sound() override {
            //delete[] audio;
            free(audio);
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

            setSampleRate(fileSampleRate);

            omni::LOG_DEBUG("length: {}", length);

            if (length <= 0 || !pcm) return false;

            const int total = length * channels;

            audio = new float[total];
            pcm16_to_float(pcm, audio, total);

            free(pcm);

            playhead = 0;
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
            const int sampleRate = static_cast<int>(sr);

            setSampleRate(sampleRate);

            playhead = 0;
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
            const int sampleRate = static_cast<int>(config.sampleRate);

            setSampleRate(sampleRate);

            playhead = 0;
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
            const int sampleRate = static_cast<int>(sr);

            setSampleRate(sampleRate);

            playhead = 0;
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
        void stop()    { playing = false; playhead = 0; }
        void restart() { playhead = 0; }

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

            const uint64_t srcTotal = length * channels;
            const double step = static_cast<double>(effectiveSampleRate) / sampleRate;

            for (int i = 0; i < numFrames; ++i) {
                // Compute the base source frame index (fractional)
                uint64_t srcFrame = static_cast<uint64_t>(playhead);

                if (srcFrame * channels >= srcTotal) {
                    if (loop) { playhead = 0.0; srcFrame = 0; }
                    else { playing = false; break; }
                }

                for (int ch = 0; ch < numChannels; ++ch) {
                    uint64_t srcCh = ch % channels; // handle mono -> stereo upmix, etc.
                    output[i * numChannels + ch] += audio[srcFrame * channels + srcCh];
                }

                playhead += step;
            }
        }
    };
}

#endif //QUARTZ_SOUND_H

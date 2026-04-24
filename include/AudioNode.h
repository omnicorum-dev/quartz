//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_AUDIONODE_H
#define QUARTZ_AUDIONODE_H

#define STB_VORBIS_IMPLEMENTATION
#include <stb_vorbis.c>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

#define MAX_CHANNELS 8

namespace Quartz {
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
}


#endif //QUARTZ_AUDIONODE_H

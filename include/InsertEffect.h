//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_MONOINSERT_H
#define QUARTZ_MONOINSERT_H

#include <AudioNode.h>
#include <iostream>

namespace Quartz {
    class InsertEffect : public AudioNode {
    protected:
        AudioNode* input = nullptr;

        float fs{};
        int numChannels{};
        int blockSize{};

        bool prepped = false;
    public:

        InsertEffect(const float _sampleRate, const int _channels) {
            fs = _sampleRate;
            numChannels = _channels;
        }

        void setInput(AudioNode* in) {
            input = in;
        }

        const char* getName() const override {
            return "Mono Insert";
        }

        void print(const int depth = 0) const override {
            for (int i = 0; i < depth; ++i) std::cout << "  ";
            std::cout << "Mono Insert\n";

            if (input)
                input->print(depth + 1);
        }

        virtual float processSample (float xn, int channel = 0) = 0;

        virtual void reset () {}

        virtual void processBuffer(float* buffer, const int frames) {
            const int total = frames * numChannels;

            for (int i = 0; i < total; ++i) {
                buffer[i] = processSample(buffer[i], i % numChannels);
            }
        }

        void getSamples(float* output, const int frames, const int channels, const int sampleRate) override {
            if (channels != numChannels || fs != sampleRate) {
                numChannels = channels;
                fs = sampleRate;
                reset();
            }

            const int total = frames * channels;
            memset(output, 0, total * sizeof(float));

            if (input)
                input->getSamples(output, frames, channels, sampleRate);

            processBuffer(output, frames);
        }
    };
}

#endif //QUARTZ_MONOINSERT_H

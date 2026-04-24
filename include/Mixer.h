//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_MIXER_H
#define QUARTZ_MIXER_H

#include <AudioNode.h>

namespace Quartz {

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
}

#endif //QUARTZ_MIXER_H

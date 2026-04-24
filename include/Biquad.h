//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_BIQUAD_H
#define QUARTZ_BIQUAD_H

#include <InsertEffect.h>

namespace Quartz {
    enum class BiquadType {
        LOWPASS = 0,
        HIGHPASS,
        BANDPASS,
        NOTCH,
        PEAKING,
        HIGH_SHELF,
        LOW_SHELF,
        ALL_PASS
    };

    class Biquad : public InsertEffect {
    public:
        virtual void updateCoeffs() = 0;

        Biquad(const float fs, const int channels) : InsertEffect(fs, channels) {
            f0 = 1000.f;
            Q  = 0.7071f;
            A  = 1.0f;
            w0 = 0; a = 0; a0 = 0; a1 = 0; a2 = 0;
            b0 = 1; b1 = 0; b2 = 0;
            for (auto&[xnm1, xnm2, ynm1, ynm2] : state) {
                xnm1 = 0;
                xnm2 = 0;
                ynm1 = 0;
                ynm2 = 0;
            }
        }

        void reset() override {
            for (auto&[xnm1, xnm2, ynm1, ynm2] : state) {
                xnm1 = 0;
                xnm2 = 0;
                ynm1 = 0;
                ynm2 = 0;
            }
            updateCoeffs();
        }

        void setFreq(const float _frequency) {
            f0 = std::clamp(_frequency, 0.f, fs/2 - 100);
            updateCoeffs();
        }
        void setQ(const float _q) {
            Q = _q;
            updateCoeffs();
        }
        void setA(const float _a) {
            A = _a;
            updateCoeffs();
        }
        void setAll(const float _frequency, const float _q, const float _a) {
            f0 = _frequency;
            Q = _q;
            A = _a;
            updateCoeffs();
        }

        float getFreq() { return f0; }
        float getQ() { return Q; }
        float getA() { return A; }

        float processSample(const float xn, const int channel) override {
            if (channel >= MAX_CHANNELS) { return 0.f; }

            const float yn = (a0/b0)*xn + (a1/b0)*state.at(channel).xnm1 + (a2/b0)*state.at(channel).xnm2
                                        - (b1/b0)*state.at(channel).ynm1 - (b2/b0)*state.at(channel).ynm2;

            state.at(channel).xnm2 = state.at(channel).xnm1;
            state.at(channel).xnm1 = xn;
            state.at(channel).ynm2 = state.at(channel).ynm1;
            state.at(channel).ynm1 = yn;
            return yn;
        }


    protected:
        struct State {
            float xnm1{}, xnm2{}, ynm1{}, ynm2{};
        };

        float f0{}, Q{}, A{};
        float w0{}, a{}, a0{}, a1{}, a2{}, b0{}, b1{}, b2{};
        std::array<State, MAX_CHANNELS> state;
    };



    class RBJ : public Biquad {
    public:

        RBJ(const float fs, const int channels) : Biquad(fs, channels) {
            Biquad::reset();
        }

        void setFilterType(BiquadType _filterType) {
            filterType = _filterType;
            updateCoeffs();
        }

        void updateCoeffs() override {
            w0 = (2*M_PI*f0)/fs;
            a  = sin(w0)/(2*Q);

            float cosw0 = cos(w0);

            switch (filterType) {
                case BiquadType::LOWPASS:
                    a0 = (1-cosw0)/2;
                    a1 = 1-cosw0;
                    a2 = a0;
                    b0 = 1+a;
                    b1 = -2*cosw0;
                    b2 = 1-a;
                    break;
                case BiquadType::HIGHPASS:
                    a0 = (1+cosw0)/2;
                    a1 = -1 * (1+cosw0);
                    a2 = a0;
                    b0 = 1+a;
                    b1 = -2*cosw0;
                    b2 = 1-a;
                    break;
                case BiquadType::BANDPASS:
                    a0 = A * a;
                    a1 = 0;
                    a2 = -A * a;
                    b0 = 1 + a;
                    b1 = -2 * cosw0;
                    b2 = 1 - a;
                    break;
                case BiquadType::NOTCH:
                    a0 = 1;
                    a1 = -2 * cosw0;
                    a2 = 1;
                    b0 = 1 + a;
                    b1 = a1;
                    b2 = 1 - a;
                    break;
                case BiquadType::PEAKING:
                    a0 = 1 + A * a;
                    a1 = -2 * cosw0;
                    a2 = 1 - A * a;
                    b0 = 1 + (a/A);
                    b1 = a1;
                    b2 = 1 - (a/A);
                    break;
                case BiquadType::HIGH_SHELF:
                    a0 = A * ((A+1)+(A-1) * cosw0 + 2*sqrt(A)*a);
                    a1 = -2*A*((A-1)+(A+1)*cosw0);
                    a2 = A * ((A+1)+(A-1) * cosw0 - 2*sqrt(A)*a);
                    b0 = (A+1) - (A-1) * cosw0 + 2*sqrt(A)*a;
                    b1 = 2 * ((A-1)-(A+1)*cosw0);
                    b2 = (A+1) - (A-1) * cosw0 - 2*sqrt(A)*a;
                    break;
                case BiquadType::LOW_SHELF:
                    a0 = A * ((A+1)-(A-1) * cosw0 + 2*sqrt(A)*a);
                    a1 = 2*A*((A-1)-(A+1)*cosw0);
                    a2 = A * ((A+1)-(A-1) * cosw0 - 2*sqrt(A)*a);
                    b0 = (A+1) + (A-1) * cosw0 + 2*sqrt(A)*a;
                    b1 = -2 * ((A-1)+(A+1)*cosw0);
                    b2 = (A+1) + (A-1) * cosw0 - 2*sqrt(A)*a;
                    break;
                case BiquadType::ALL_PASS:
                    a0 = 1-a;
                    a1 = -2*cosw0;
                    a2 = 1+a;
                    b0 = a2;
                    b1 = a1;
                    b2 = a0;
                    break;
                default:
                    a0 = (1-cosw0)/2;
                    a1 = 1-cosw0;
                    a2 = a0;
                    b0 = 1+a;
                    b1 = -2*cosw0;
                    b2 = 1-a;
                    break;
            }
        }
    protected:
        BiquadType filterType = BiquadType::LOWPASS;
    };




    class LR4 : public InsertEffect {
    public:
        LR4(const float fs, const int channels) : InsertEffect(fs, channels), stage1(fs, channels), stage2(fs, channels) {}

        void setFilterType(BiquadType _filterType) {
            stage1.setFilterType(_filterType);
            stage2.setFilterType(_filterType);
        }

        void setFreq(const float _frequency) {
            stage1.setFreq(_frequency);
            stage2.setFreq(_frequency);
        }

        float getFreq() {
            return stage1.getFreq();
        }

        float processSample(float xn, int channel) override {
            float wn = stage1.processSample(xn, channel);
            return stage2.processSample(xn, channel);
        }

    protected:
        RBJ stage1, stage2;
    };

}

#endif //QUARTZ_BIQUAD_H

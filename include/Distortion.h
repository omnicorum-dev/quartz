//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_DISTORTION_H
#define QUARTZ_DISTORTION_H

#include <InsertEffect.h>

namespace Quartz {
    class Distortion : public InsertEffect {
    public:
        Distortion(const float _fs, const int numChannels) : InsertEffect(_fs, numChannels) {};

        virtual float distortion (float xn) = 0;

        float processSample(float xn, int channel) override {
            return distortion(applyDriveBias(xn));
        }

        void setDrive (float _driveMag) { driveMag = _driveMag; }
        void setBias  (float _bias) { bias = _bias; }

        float getDriveMag () const { return driveMag; }
        float getBias () const     { return bias; }
    protected:
        float applyDriveBias(const float xn) const {
            return xn * driveMag + bias;
        }

        float driveMag = 1.f;
        float bias     = 0.f;
    };



    class RectifierFull : public Distortion {
    public:
        RectifierFull(const float fs, const int numChannels) : Distortion(fs, numChannels) {}

        float distortion (float xn) override {
            if (xn < 0.f) return -xn;
            return xn;
        }
    };

    class RectifierHalf : public Distortion {
    public:
        RectifierHalf(const float fs, const int numChannels) : Distortion(fs, numChannels) {}
        float distortion (float xn) override {
            if (xn < 0.f) return 0.f;
            return xn;
        }
    };

    class HardClip : public Distortion {
    public:
        HardClip(const float fs, const int numChannels) : Distortion(fs, numChannels) {}
        float distortion (float xn) override {
            if (xn > m_Threshold_linear) return m_Threshold_linear;
            if (xn < -m_Threshold_linear) return -m_Threshold_linear;
            return xn;
        }

        void setThreshold_linear(float threshold) {
            m_Threshold_linear = threshold;
        }

        float getThreshold_linear() const { return m_Threshold_linear; }

    private:
        float m_Threshold_linear = 1.0f;
    };

    class TanhShaper : public Distortion {
    public:
        TanhShaper(const float fs, const int numChannels) : Distortion(fs, numChannels) {}
        float distortion (float xn) override {
            return std::tanh(xn);
        }
    };

    class SoftClipper : public Distortion {
    public:
        SoftClipper(const float fs, const int numChannels) : Distortion(fs, numChannels) {
            setThreshold_linear(1.0);
            setKnee_dB(3.0);
        }

        void setThreshold_linear(double threshold) {
            threshold_linear = threshold;
        }

        void setKnee_dB(double _knee_dB) {
            knee_dB = _knee_dB;
            knee_linear = std::min((2*threshold_linear), (1.0 / std::pow(10.0f, _knee_dB / 20.0f)) - 1.0);
            //knee_linear = 2.0 * threshold_linear * std::pow(10.0, knee_dB / 20.0) - 2.0 * threshold_linear;
            halfKnee = 0.5 * knee_linear;
        }

        double getThreshold_linear() const { return threshold_linear; }
        double getKnee_dB() const { return knee_dB; }

        float distortion(float xn) override {
            const float abs = std::fabs(xn);
            const int sign = xn >= 0 ? 1 : -1;

            if (abs <= threshold_linear - halfKnee) {
                return xn;
            } else if (abs <= threshold_linear + halfKnee) {
                const float excess = abs - (threshold_linear - halfKnee);
                return sign * (abs - (excess * excess) / (2 * knee_linear));
            } else {
                return sign * threshold_linear;
            }
        }
    private:
        double threshold_linear{};
        double knee_linear{};
        double knee_dB{};
        double halfKnee{};
    };

}

#endif //QUARTZ_DISTORTION_H

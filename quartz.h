//
// Created by Nico Russo on 4/15/26.
//

#ifndef QUARTZ_H
#define QUARTZ_H

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <base.h>

#include <AudioNodeCollection.h>

using namespace omni;

class AudioManager {
private:
    ma_result result{};
    ma_device_config deviceConfig{};
    ma_device device{};

    Quartz::Mixer masterBuss;

    int numChannels = 0;
    int sampleRate = 0;

public:
    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
    {
        AudioManager* self = static_cast<AudioManager*>(pDevice->pUserData);

        self->masterBuss.getSamples((float*)pOutput, frameCount, self->numChannels, self->sampleRate);
    }

    Quartz::Mixer& master() { return masterBuss; }

    float getSampleRate() { return sampleRate; }
    int getNumChannels() { return numChannels; }

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

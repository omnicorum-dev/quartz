//
// Created by Nico Russo on 4/23/26.
//

#ifndef QUARTZ_MUSIC_H
#define QUARTZ_MUSIC_H

#include <AudioNode.h>

namespace Quartz {

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

        float playbackSpeed = 1.0f;
        int effectiveSampleRate = 0;

        std::string name;
        mutable std::string cachedName;

        static constexpr int MAX_FRAMES = 1024;
        static constexpr int MAX_CH = 2;

    public:
        bool loop = false;

        Music() = default;
        Music(const std::string& file, const AudioFileFormat fileFormat) {
            load(file, fileFormat);
        }
        ~Music() override {
            if (vorbis) stb_vorbis_close(vorbis);
            if (flac) drflac_close(flac);
            if (mp3) free(mp3);
            if (wav) free(wav);
        }

        void setSampleRate(const int sampleRate) {
            fileSampleRate = sampleRate;
            effectiveSampleRate = sampleRate * playbackSpeed;
        }

        void setPlaybackSpeed(const float speed) {
            playbackSpeed = speed;
            effectiveSampleRate = fileSampleRate * playbackSpeed;
        }

        float getPlaybackSpeed() const { return playbackSpeed; }

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

            setSampleRate(fileSampleRate);

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

            setSampleRate(fileSampleRate);

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

            setSampleRate(fileSampleRate);

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

            setSampleRate(fileSampleRate);

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
        int readFrames(float* out, const int frames) const {
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

        void seekToStart() const {
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
            //const double resampleRatio = static_cast<double>(fileSampleRate) / sampleRate;
            const double resampleRatio = static_cast<double>(effectiveSampleRate) / sampleRate;
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
}

#endif //QUARTZ_MUSIC_H

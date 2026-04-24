#include <iostream>
#include <thread>

#include <quartz.h>
#include <AudioNodeCollection.h>

#define SAMPLE_RATE 48000
#define CHANNELS 2

int main() {
    AudioManager quartz;

    quartz.init(SAMPLE_RATE, CHANNELS);

    Quartz::Sound ctt("../assets/archery.flac", Quartz::FLAC);

    omni::LOG_DEBUG("ctt length: {}", ctt.getLength());

    Quartz::Music htf;
    if (!htf.load("../assets/archery.flac", Quartz::FLAC)) {
        omni::LOG_ERROR("Could not load file");
    }

    htf.loop = true;
    htf.setPlaybackSpeed(1.5f);
    ctt.setPlaybackSpeed(2.5f);

    Quartz::Mixer bus;

    Quartz::RectifierHalf clipper(quartz.getSampleRate(), quartz.getNumChannels());

    Quartz::RBJ filter(quartz.getSampleRate(), quartz.getNumChannels());
    filter.setQ(5.f);

    Quartz::LR4 filter2(quartz.getSampleRate(), quartz.getNumChannels());
    filter2.setFreq(15000);

    //quartz.master().addInput(&clipper);
    quartz.master().addInput(&clipper);
    clipper.setInput(&filter2);
    filter2.setInput(&bus);
    bus.addInput(&htf);
    bus.addInput(&ctt);

    quartz.master().print();

    while (true) {
        const int c = getchar();
        if (c == 'q') {
            break;
        } else if (c == 's') {
            ctt.playStop();
        } else if (c == 'm') {
            htf.playPause();
        } else if (c == '+') {
            htf.setPlaybackSpeed(htf.getPlaybackSpeed() + 0.1f);
        } else if (c == '-') {
            htf.setPlaybackSpeed(htf.getPlaybackSpeed() - 0.1f);
        } else if (c == 'k') {
            //filter.setFreq(filter.getFreq() * 0.9f);
            filter2.setFreq(filter2.getFreq() * 0.9f);
        } else if (c == 'l') {
            //filter.setFreq(filter.getFreq() * 1.1f);
            filter2.setFreq(filter2.getFreq() * 1.1f);
        } else if (c == 'e') {
            clipper.setDrive(clipper.getDriveMag() + 0.05f);
        } else if (c == 'w') {
            clipper.setDrive(clipper.getDriveMag() - 0.05f);
        }
    }

    return 0;
}
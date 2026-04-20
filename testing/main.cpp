#include <iostream>
#include <thread>

#include <quartz.h>

int main() {
    Quartz quartz;

    quartz.init(48000, 2);

    Sound ctt;
    ctt.load("../assets/ctt.ogg", AudioFileFormat::OGG);

    omni::LOG_DEBUG("ctt length: {}", ctt.getLength());

    Music htf;
    if (!htf.load("/Users/nicorusso/Development/Quartz/assets/cegl.mp3", MP3)) {
        omni::LOG_ERROR("Could not load wav file");
    }

    Mixer bus;

    HardClip clipper;
    clipper.threshold = 0.75f;

    quartz.master().addInput(&clipper);
    clipper.setInput(&bus);
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
        }
    }

    return 0;
}
#include <iostream>
#include <thread>

#include <quartz.h>

int main() {
    Quartz quartz;

    quartz.init(48000, 2);

    Sound ctt;
    ctt.load("../assets/ctt.ogg");

    Music htf;
    htf.load("../assets/htf.ogg");

    Mixer bus;

    HardClip clipper;
    clipper.threshold = 0.75f;

    quartz.master().addInput(&clipper);
    clipper.setInput(&bus);
    bus.addInput(&htf);
    bus.addInput(&ctt);

    quartz.master().print();

    while (true) {
        char c = getchar();
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
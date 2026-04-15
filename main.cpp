#include <iostream>
#include <thread>

#include "quartz.h"

int main() {
    Quartz quartz;

    quartz.init(48000, 2);

    Sound ctt;
    ctt.load("../ctt.ogg");

    Music htf;
    htf.load("../htf.ogg");

    Mixer bus;

    HardClip clipper;
    clipper.threshold = 0.75f;

    quartz.master().addInput(&clipper);
    clipper.setInput(&bus);
    bus.addInput(&htf);
    bus.addInput(&ctt);

    quartz.master().print();

    printf("Press Enter to quit...");
    getchar();

    return 0;
}
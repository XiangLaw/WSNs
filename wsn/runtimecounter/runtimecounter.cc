#include <scheduler.h>
#include "runtimecounter.h"

RunTimeCounter::RunTimeCounter() {
    FILE *fp = fopen("Runtime.tr", "w");
    fclose(fp);
}

void RunTimeCounter::printTime(const char* prefix) {
    timeval t;
    gettimeofday(&t, NULL);
    double currentTime = (t.tv_sec * 1000000.0) + t.tv_usec;
    currentTime = Scheduler::instance().clock();
    FILE *fp = fopen("Runtime.tr", "a+");
    fprintf(fp, "%s\t%f\n", prefix, currentTime);
    fclose(fp);
}

void RunTimeCounter::start() {
    printTime("started_time");
}

void RunTimeCounter::finish() {
    printTime("finished_time");
}

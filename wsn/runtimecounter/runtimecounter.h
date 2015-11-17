#ifndef RUNTIME_COUNTER_H
#define RUNTIME_COUNTER_H
#include <stdio.h>
#include <sys/time.h>

class RunTimeCounter {
public:
    RunTimeCounter();

    void start();
    void finish();

private:
    void printTime(const char*);
};

#endif
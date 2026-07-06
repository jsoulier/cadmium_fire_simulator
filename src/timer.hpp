#pragma once

#include <SDL3/SDL.h>

#include <string>

#define TimerBlock(label) Timer timer(label)

class Timer
{
public:
    Timer(std::string label);
    ~Timer();
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

private:
    std::string Label;
    Uint64 Start;
};

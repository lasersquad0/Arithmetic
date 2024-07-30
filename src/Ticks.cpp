#include "Ticks.h" 
#include <chrono>

std::map<string_t, Ticks::timepoint> Ticks::s;
std::map<string_t, Ticks::timepoint> Ticks::f;


std::string millisecToStr(long long ms)
{
    auto milliseconds = ms % 1000;
    auto seconds = (ms / 1000) % 60;
    auto minutes = (ms / 60000) % 60;
    auto hours = (ms / 3600000) % 24;

    char buf[100];
    if (hours > 0)
        sprintf_s(buf, 100, "%llu hours %llu minutes %llu seconds %llu ms", hours, minutes, seconds, milliseconds);
    else if (minutes > 0)
        sprintf_s(buf, 100, "%llu minutes %llu seconds %llu ms", minutes, seconds, milliseconds);
    else
        sprintf_s(buf, 100, "%llu seconds %llu ms", seconds, milliseconds);

    return buf;
}




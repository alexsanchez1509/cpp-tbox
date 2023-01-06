#include <gtest/gtest.h>
#include <thread>

#include "time_counter.h"
#include <tbox/base/log_output.h>

TEST(TimeCounter, basic)
{
    LogOutput_Initialize();

    SetTimeCounter();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    SetTimeCounter();
}

TEST(TimeCounter, stop)
{
    LogOutput_Initialize();

    SetNamedTimeCounter(a);
    SetNamedTimeCounter(b);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    StopNamedTimeCounter(a);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

#include "time_counter.h"
#include <iostream>

namespace tbox {
namespace util {

using namespace std;
using namespace std::chrono;

namespace {
const char* Basename(const char *full_path)
{
    const char *p_last = full_path;
    if (p_last != nullptr) {
        for (const char *p = full_path; *p; ++p) {
            if (*p == '/')
                p_last = p + 1;
        }
    }
    return p_last;
}
}

TimeCounter::TimeCounter(const char *file_name, const char *func_name, int line) :
    file_name_(file_name),
    func_name_(func_name),
    line_(line)
{
    start_time_point_ = steady_clock::now();
}

TimeCounter::~TimeCounter()
{
    stop();
}

void TimeCounter::stop()
{
    if (stoped_)
        return;

    auto cost = steady_clock::now() - start_time_point_;
    cout << "Info: " << func_name_ << "() costs " << cost.count()
         <<  " ns -- " << Basename(file_name_)
         << ":" << line_ << endl;

    stoped_ = true;
}

}
}

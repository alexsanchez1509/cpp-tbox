#ifndef TBOX_UTIL_TIME_COUNTER_H_20220225
#define TBOX_UTIL_TIME_COUNTER_H_20220225

#include <chrono>

namespace tbox {
namespace util {

class TimeCounter {
  public:
    TimeCounter(const char *file_name, const char *func_name, int line);
    ~TimeCounter();

    void stop();

  private:
    const char *file_name_;
    const char *func_name_;
    int line_;

    std::chrono::steady_clock::time_point start_time_point_;
    bool stoped_ = false;
};

}
}

//! 无名计时器，用行号命名
#define _TimeCounter_1(file,func,line) tbox::util::TimeCounter _timer_counter_at_##line(file,func,line)
#define _TimeCounter_0(file,func,line) _TimeCounter_1(file,func,line)
#define SetTimeCounter() _TimeCounter_0(__FILE__, __func__, __LINE__)

//! 有名计时器
#define _NamedTimeCounter_0(file,func,line,name) tbox::util::TimeCounter _timer_counter_##name(file,func,line)
#define SetNamedTimeCounter(name) _NamedTimeCounter_0(__FILE__, __func__, __LINE__, name)
#define StopNamedTimeCounter(name) _timer_counter_##name.stop()

#endif //TBOX_UTIL_TIME_COUNTER_H_20220225

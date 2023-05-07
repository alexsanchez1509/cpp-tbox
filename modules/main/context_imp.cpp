#include "context_imp.h"

#include <sstream>
#include <iomanip>
#include <sys/time.h>

#include <tbox/base/json.hpp>
#include <tbox/base/log.h>
#include <tbox/base/assert.h>
#include <tbox/base/version.h>
#include <tbox/base/catch_throw.h>
#include <tbox/util/string.h>
#include <tbox/util/json.h>
#include <tbox/terminal/session.h>

#include "main.h"

namespace tbox {
namespace main {

namespace {
std::string ToString(const std::chrono::milliseconds msec)
{
    uint64_t ms = msec.count();

    constexpr auto ms_per_sec   = 1000;
    constexpr auto ms_per_min   = 60 * ms_per_sec;
    constexpr auto ms_per_hour  = 60 * ms_per_min;
    constexpr auto ms_per_day   = 24 * ms_per_hour;

    auto days = ms / ms_per_day;
    auto ms_in_day = ms % ms_per_day;
    auto hours = ms_in_day / ms_per_hour;
    auto ms_in_hour = ms_in_day % ms_per_hour;
    auto mins = ms_in_hour / ms_per_min;
    auto ms_in_min = ms_in_hour % ms_per_min;
    auto secs = ms_in_min / ms_per_sec;
    auto msecs = ms_in_min % ms_per_sec;

    using namespace std;
    ostringstream oss;

    oss << days << ' ';
    oss << setfill('0');
    oss << setw(2) << hours << ':'
        << setw(2) << mins << ':'
        << setw(2) << secs << '.'
        << setw(3) << msecs;

    return oss.str();
}
}

ContextImp::ContextImp() :
    sp_loop_(event::Loop::New()),
    sp_thread_pool_(new eventx::ThreadPool(sp_loop_)),
    sp_timer_pool_(new eventx::TimerPool(sp_loop_)),
    sp_terminal_(new terminal::Terminal),
    sp_telnetd_(new terminal::Telnetd(sp_loop_, sp_terminal_)),
    sp_tcp_rpc_(new terminal::TcpRpc(sp_loop_, sp_terminal_)),
    start_time_point_(std::chrono::steady_clock::now())
{
    TBOX_ASSERT(sp_loop_ != nullptr);
    TBOX_ASSERT(sp_thread_pool_ != nullptr);
    TBOX_ASSERT(sp_timer_pool_ != nullptr);
    TBOX_ASSERT(sp_terminal_ != nullptr);
    TBOX_ASSERT(sp_telnetd_ != nullptr);
    TBOX_ASSERT(sp_tcp_rpc_ != nullptr);
}

ContextImp::~ContextImp()
{
    delete sp_tcp_rpc_;
    delete sp_telnetd_;
    delete sp_terminal_;
    delete sp_timer_pool_;
    delete sp_thread_pool_;
    delete sp_loop_;
}

void ContextImp::fillDefaultConfig(Json &cfg) const
{
    cfg["thread_pool"] = R"({"min":0, "max":5})"_json;
}

bool ContextImp::initialize(const Json &cfg)
{
    if (util::json::HasObjectField(cfg, "loop")) {
        auto &js_loop = cfg["loop"];
        initLoop(js_loop);
    }

    if (!util::json::HasObjectField(cfg, "thread_pool")) {
        LogWarn("cfg.thread_pool not found");
        return false;
    }

    auto &js_thread_pool = cfg["thread_pool"];
    if (!initThreadPool(js_thread_pool))
        return false;

    if (util::json::HasObjectField(cfg, "telnetd")) {
        auto &js_telnetd = cfg["telnetd"];
        initTelnetd(js_telnetd);
    }

    if (util::json::HasObjectField(cfg, "tcp_rpc")) {
        auto &js_tcp_rpc = cfg["tcp_rpc"];
        initRpc(js_tcp_rpc);
    }

    initShell();

    return true;
}

bool ContextImp::initLoop(const Json &js)
{
    int run_in_loop_queue_size_water_line = 0;
    if (util::json::GetField(js, "run_in_loop_queue_size_water_line", run_in_loop_queue_size_water_line))
        sp_loop_->setRunInLoopQueueSizeWaterLine(run_in_loop_queue_size_water_line);

    int run_next_queue_size_water_line = 0;
    if (util::json::GetField(js, "run_next_queue_size_water_line", run_next_queue_size_water_line))
        sp_loop_->setRunNextQueueSizeWaterLine(run_next_queue_size_water_line);

    int run_in_loop_delay_water_line_us = 0;
    if (util::json::GetField(js, "run_in_loop_delay_water_line", run_in_loop_delay_water_line_us))
        sp_loop_->setRunInLoopDelayWaterLine(std::chrono::microseconds(run_in_loop_delay_water_line_us));

    int run_next_delay_water_line_us = 0;
    if (util::json::GetField(js, "run_next_delay_water_line", run_next_delay_water_line_us))
        sp_loop_->setRunInLoopDelayWaterLine(std::chrono::microseconds(run_next_delay_water_line_us));

    int loop_time_cost_water_line_us = 0;
    if (util::json::GetField(js, "loop_time_cost_water_line", loop_time_cost_water_line_us))
        sp_loop_->setLoopTimeCostWaterLine(std::chrono::microseconds(loop_time_cost_water_line_us));

    int cb_time_cost_water_line_us = 0;
    if (util::json::GetField(js, "cb_time_cost_water_line", cb_time_cost_water_line_us))
        sp_loop_->setCbTimeCostWaterLine(std::chrono::microseconds(cb_time_cost_water_line_us));

    int run_request_delay_water_line_us = 0;
    if (util::json::GetField(js, "run_request_delay_water_line", run_request_delay_water_line_us))
        sp_loop_->setRunRequestDelayWaterLine(std::chrono::microseconds(run_request_delay_water_line_us));

    return true;
}

bool ContextImp::initThreadPool(const Json &js)
{
    int thread_pool_min = 0, thread_pool_max = 0;
    if (!util::json::GetField(js, "min", thread_pool_min) ||
        !util::json::GetField(js, "max", thread_pool_max)) {
        LogWarn("in cfg.thread_pool, min or max is not number");
        return false;
    }

    if (!sp_thread_pool_->initialize(thread_pool_min, thread_pool_max))
        return false;

    return true;
}

bool ContextImp::initTelnetd(const Json &js)
{
    std::string telnetd_bind;
    if (util::json::GetField(js, "bind", telnetd_bind)) {
      if (sp_telnetd_->initialize(telnetd_bind))
        telnetd_init_ok = true;
    }
    return true;
}

bool ContextImp::initRpc(const Json &js)
{
    std::string tcp_rpc_bind;
    if (util::json::GetField(js, "bind", tcp_rpc_bind)) {
        if (sp_tcp_rpc_->initialize(tcp_rpc_bind))
            tcp_rpc_init_ok = true;
    }
    return true;
}

bool ContextImp::start()
{
    if (telnetd_init_ok)
        sp_telnetd_->start();

    if (tcp_rpc_init_ok)
        sp_tcp_rpc_->start();

    return true;
}

void ContextImp::stop()
{
    if (tcp_rpc_init_ok)
        sp_tcp_rpc_->stop();

    if (telnetd_init_ok)
        sp_telnetd_->stop();
}

void ContextImp::cleanup()
{
    sp_tcp_rpc_->cleanup();
    sp_telnetd_->cleanup();
    sp_timer_pool_->cleanup();
    sp_thread_pool_->cleanup();

    LogInfo("running_time: %s", ToString(running_time()).c_str());
}

std::chrono::milliseconds ContextImp::running_time() const
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_point_);
}

std::chrono::system_clock::time_point ContextImp::start_time_point() const
{
    auto steady_clock_now = std::chrono::steady_clock::now();
    auto system_clock_now = std::chrono::system_clock::now();
    return system_clock_now - (steady_clock_now - start_time_point_);
}

void ContextImp::initShell()
{
    using namespace terminal;
    TerminalNodes *wp_nodes = sp_terminal_;

    auto ctx_node = wp_nodes->createDirNode("This is Context directory");
    wp_nodes->mountNode(wp_nodes->rootNode(), ctx_node, "ctx");

    {
        auto loop_node = wp_nodes->createDirNode("This is Loop directory");
        wp_nodes->mountNode(ctx_node, loop_node, "loop");

        {
            auto water_line_node = wp_nodes->createDirNode("This is water line directory");
            wp_nodes->mountNode(loop_node, water_line_node, "wl");
            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setRunInLoopQueueSizeWaterLine(value);
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getRunInLoopQueueSizeWaterLine() << "\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setRunInLoopQueueSizeWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "run_in_loop_queue_size");
            }

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setRunNextQueueSizeWaterLine(value);
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getRunNextQueueSizeWaterLine() << "\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setRunNextQueueSizeWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "run_next_queue_size");
            }

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setRunInLoopDelayWaterLine(std::chrono::microseconds(value));
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getRunInLoopDelayWaterLine().count()/1000 << " us\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setRunInLoopDelayWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "run_in_loop_delay");
            }

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setRunNextDelayWaterLine(std::chrono::microseconds(value));
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getRunNextDelayWaterLine().count()/1000 << " us\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setRunNextDelayWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "run_next_delay");
            }

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setLoopTimeCostWaterLine(std::chrono::microseconds(value));
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getLoopTimeCostWaterLine().count()/1000 << " us\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setLoopTimeCostWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "loop_time_cost");
            }

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setCbTimeCostWaterLine(std::chrono::microseconds(value));
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getCbTimeCostWaterLine().count()/1000 << " us\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setCbTimeCostWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "cb_time_cost");
            }

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        bool print_usage = false;
                        std::ostringstream oss;
                        if (args.size() >= 2) {
                            int value = 0;
                            auto may_throw_func = [&] { value = std::stoi(args[1]); };
                            if (CatchThrowQuietly(may_throw_func)) {
                                oss << "must be number\r\n";
                                print_usage = true;
                            } else {
                                sp_loop_->setRunRequestDelayWaterLine(std::chrono::microseconds(value));
                                oss << "done\r\n";
                            }
                        } else {
                            oss << sp_loop_->getRunRequestDelayWaterLine().count()/1000 << " us\r\n";
                        }

                        if (print_usage)
                            oss << "Usage: " << args[0] << " <num>\r\n";

                        s.send(oss.str());
                    },
                    "Invoke Loop::setRunRequestDelayWaterLine()"
                );
                wp_nodes->mountNode(water_line_node, func_node, "run_request_delay");
            }
        }

        {
            auto loop_stat_node = wp_nodes->createDirNode();
            wp_nodes->mountNode(loop_node, loop_stat_node, "stat");

            auto loop_stat_print_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    ss << sp_loop_->getStat();
                    std::string txt = ss.str();
                    util::string::Replace(txt, "\n", "\r\n");
                    s.send(txt);
                }
            , "print Loop's stat data");
            wp_nodes->mountNode(loop_stat_node, loop_stat_print_node, "print");

            auto loop_stat_reset_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    sp_loop_->resetStat();
                    ss << "done\r\n";
                    s.send(ss.str());
                }
            , "reset Loop's stat data");
            wp_nodes->mountNode(loop_stat_node, loop_stat_reset_node, "reset");

            {
                auto func_node = wp_nodes->createFuncNode(
                    [this] (const Session &s, const Args &args) {
                        s.send(ToString(running_time()));
                        s.send("\r\n");
                    }
                , "Print running times");
                wp_nodes->mountNode(ctx_node, func_node, "running_time");
            }
        }

        {
            auto func_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(start_time_point().time_since_epoch()).count();
                    {
                        time_t sec = ts / 1000;
                        struct tm tm;
                        localtime_r(&sec, &tm);
                        char tmp[20];
                        strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", &tm);
                        ss << tmp << '.' << ts % 1000 << "\r\n";
                    }

                    s.send(ss.str());
                }
            , "Print start time point");
            wp_nodes->mountNode(ctx_node, func_node, "start_time");
        }

    }

    {
        auto threadpool_node = wp_nodes->createDirNode("This is ThreadPool directory");
        wp_nodes->mountNode(ctx_node, threadpool_node, "thread_pool");

        {
            //! 打印线程池快照
            auto func_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::ostringstream oss;
                    auto snapshot = sp_thread_pool_->snapshot();
                    oss << "thread_num: " << snapshot.thread_num << "\r\n";
                    oss << "idle_thread_num: " << snapshot.idle_thread_num << "\r\n";
                    oss << "undo_task_num:";
                    for (size_t i = 0; i < THREAD_POOL_PRIO_SIZE; ++i)
                        oss << " " << snapshot.undo_task_num[i];
                    oss << "\r\n";
                    oss << "doing_task_num: " << snapshot.doing_task_num << "\r\n";
                    oss << "undo_task_peak_num: " << snapshot.undo_task_peak_num << "\r\n";
                    s.send(oss.str());
                }
            , "Print thread pool's snapshot");
            wp_nodes->mountNode(threadpool_node, func_node, "snapshot");
        }
    }

    {
        auto info_node = wp_nodes->createDirNode("Information directory");
        wp_nodes->mountNode(wp_nodes->rootNode(), info_node, "info");

        {
            auto func_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    int major = 0, minor = 0, rev = 0, build = 0;
                    GetAppVersion(major, minor, rev, build);
                    ss << 'v' << major << '.' << minor << '.' << rev << '_' << build << "\r\n";
                    s.send(ss.str());
                }
            , "Print app version");
            wp_nodes->mountNode(info_node, func_node, "app_ver");
        }
        {
            auto func_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    int major = 0, minor = 0, rev = 0;
                    GetTboxVersion(major, minor, rev);
                    ss << 'v' << major << '.' << minor << '.' << rev << "\r\n";
                    s.send(ss.str());
                }
            , "Print tbox version");
            wp_nodes->mountNode(info_node, func_node, "tbox_ver");
        }
        {
            auto func_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    ss << GetAppBuildTime() << "\r\n";
                    s.send(ss.str());
                }
            , "Print buildtime");
            wp_nodes->mountNode(info_node, func_node, "build_time");
        }
        {
            auto func_node = wp_nodes->createFuncNode(
                [this] (const Session &s, const Args &args) {
                    std::stringstream ss;
                    ss << GetAppDescribe() << "\r\n";
                    s.send(ss.str());
                }
            , "Print app describe");
            wp_nodes->mountNode(info_node, func_node, "what");
        }
    }
}

}
}

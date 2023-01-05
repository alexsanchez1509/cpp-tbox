#include <gtest/gtest.h>
#include <signal.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <tbox/base/log.h>
#include <tbox/base/log_output.h>

#include "loop.h"
#include "signal_event.h"
#include "timer_event.h"

using namespace std;
using namespace tbox::event;

const int kAcceptableError = 10;

TEST(SignalEvent, Oneshot)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);
        auto signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(signal_event->initialize(SIGUSR1, Event::Mode::kOneshot));
        EXPECT_TRUE(signal_event->enable());

        int run_time = 0;
        signal_event->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR1);
                ++run_time;
            }
        );

        sp_loop->run([] { raise(SIGUSR1); });

        sp_loop->exitLoop(std::chrono::milliseconds(100));
        sp_loop->runLoop();

        EXPECT_EQ(run_time, 1);

        delete signal_event;
        delete sp_loop;
    }
}

TEST(SignalEvent, PersistWithTimerEvent)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);

        auto signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(signal_event->initialize(SIGUSR1, Event::Mode::kPersist));
        EXPECT_TRUE(signal_event->enable());

        auto timer_event = sp_loop->newTimerEvent();
        EXPECT_TRUE(timer_event->initialize(chrono::milliseconds(10), Event::Mode::kPersist));
        EXPECT_TRUE(timer_event->enable());
        int count = 0;
        timer_event->setCallback([&]
            {
                ++count;
                if (count <= 5) {
                    raise(SIGUSR1);
                }
            }
        );

        int run_time = 0;
        signal_event->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR1);
                ++run_time;
            }
        );

        sp_loop->exitLoop(std::chrono::milliseconds(100));
        sp_loop->runLoop();

        EXPECT_EQ(run_time, 5);

        delete timer_event;
        delete signal_event;
        delete sp_loop;
    }
}

TEST(SignalEvent, MultiSignalMultiEvents)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);
        auto user1_signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(user1_signal_event->initialize(SIGUSR1, Event::Mode::kOneshot));
        EXPECT_TRUE(user1_signal_event->enable());

        auto user2_signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(user2_signal_event->initialize(SIGUSR2, Event::Mode::kOneshot));
        EXPECT_TRUE(user2_signal_event->enable());

        int user1_run_time = 0;
        user1_signal_event->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR1);
                ++user1_run_time;
            }
        );
        int user2_run_time = 0;
        user2_signal_event->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR2);
                ++user2_run_time;
            }
        );

        sp_loop->run([]
            {
                raise(SIGUSR1);
                raise(SIGUSR2);
            }
        );
        sp_loop->exitLoop(std::chrono::milliseconds(100));
        sp_loop->runLoop();

        EXPECT_EQ(user1_run_time, 1);
        EXPECT_EQ(user2_run_time, 1);


        delete user1_signal_event;
        delete user2_signal_event;
        delete sp_loop;
    }
}

TEST(SignalEvent, MultiThread)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);
        auto user1_signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(user1_signal_event->initialize(SIGUSR1, Event::Mode::kOneshot));
        EXPECT_TRUE(user1_signal_event->enable());

        auto user2_signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(user2_signal_event->initialize(SIGUSR2, Event::Mode::kOneshot));
        EXPECT_TRUE(user2_signal_event->enable());

        int user1_run_time = 0;
        user1_signal_event->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR1);
                ++user1_run_time;
            }
        );
        int user2_run_time = 0;
        user2_signal_event->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR2);
                ++user2_run_time;
            }
        );

        sp_loop->run([]
            {
                raise(SIGUSR1);
                raise(SIGUSR2);
            }
        );

        bool t1_run = false;
        //! t1 线程sleep 200ms
        auto t1 = std::thread(
            [&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                t1_run = true;
            }
        );

        //! t2 线程等待一个信号
        bool exit_thread = false;
        std::mutex lock;
        std::condition_variable cond_var;
        bool t2_run = false;
        auto t2 = std::thread(
            [&] {
                std::unique_lock<std::mutex> lk(lock);
                cond_var.wait(lk, [&]{ return exit_thread; });
                t2_run = true;
            }
        );


        sp_loop->exitLoop(std::chrono::milliseconds(100));
        sp_loop->runLoop();

        t1.join();
        {
            std::unique_lock<std::mutex> lk(lock);
            exit_thread = true;
            cond_var.notify_one();
        }
        t2.join();

        EXPECT_TRUE(t1_run);
        EXPECT_TRUE(t2_run);
        EXPECT_EQ(user1_run_time, 1);
        EXPECT_EQ(user2_run_time, 1);

        delete user1_signal_event;
        delete user2_signal_event;
        delete sp_loop;
    }
}

//! 同一种信号被多个事件监听
TEST(SignalEvent, OneSignalMultiEvents)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);

        auto signal_event_1 = sp_loop->newSignalEvent();
        auto signal_event_2 = sp_loop->newSignalEvent();

        EXPECT_TRUE(signal_event_1->initialize(SIGUSR1, Event::Mode::kOneshot));
        EXPECT_TRUE(signal_event_2->initialize(SIGUSR1, Event::Mode::kOneshot));

        EXPECT_TRUE(signal_event_1->enable());
        EXPECT_TRUE(signal_event_2->enable());

        int run_time_1 = 0;
        int run_time_2 = 0;

        signal_event_1->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR1);
                ++run_time_1;
            }
        );
        signal_event_2->setCallback(
            [&](int signo) {
                EXPECT_EQ(signo, SIGUSR1);
                ++run_time_2;
            }
        );

        sp_loop->run([] { raise(SIGUSR1); });
        sp_loop->exitLoop(std::chrono::milliseconds(100));
        sp_loop->runLoop();

        EXPECT_EQ(run_time_1, 1);
        EXPECT_EQ(run_time_2, 1);


        delete signal_event_1;
        delete signal_event_2;
        delete sp_loop;
    }
}

//! 多线程下多个Loop的事件监听同一个信号
TEST(SignalEvent, OneSignalMultiLoopInMultiThread)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        auto thread_func = [e] (int &run_time) {
            auto sp_loop = Loop::New(e);
            auto signal_event = sp_loop->newSignalEvent();
            signal_event->initialize(SIGUSR1, Event::Mode::kPersist);
            signal_event->enable();
            signal_event->setCallback([&](int) { ++run_time; });

            sp_loop->exitLoop(std::chrono::milliseconds(200));
            sp_loop->runLoop();

            delete signal_event;
            delete sp_loop;
        };

        int run_time_1 = 0;
        int run_time_2 = 0;

        auto t1 = std::thread(std::bind(thread_func, std::ref(run_time_1)));
        auto t2 = std::thread(std::bind(thread_func, std::ref(run_time_2)));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        raise(SIGUSR1);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        raise(SIGUSR1);

        t1.join();
        t2.join();

        EXPECT_EQ(run_time_1, 2);
        EXPECT_EQ(run_time_2, 2);
    }
}

//! 多线程下多个Loop的事件监听同多个信号
TEST(SignalEvent, MultiSignalMultiLoopInMultiThread)
{
    LogOutput_Initialize();

    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        auto thread_func = [e] (int &user1_run_time, int &user2_run_time) {
            auto sp_loop = Loop::New(e);

            auto user1_signal_event = sp_loop->newSignalEvent();
            user1_signal_event->initialize(SIGUSR1, Event::Mode::kPersist);
            user1_signal_event->enable();
            user1_signal_event->setCallback([&](int) { ++user1_run_time; });

            auto user2_signal_event = sp_loop->newSignalEvent();
            user2_signal_event->initialize(SIGUSR2, Event::Mode::kPersist);
            user2_signal_event->enable();
            user2_signal_event->setCallback([&](int) { ++user2_run_time; });

            sp_loop->exitLoop(std::chrono::milliseconds(200));
            sp_loop->runLoop();

            delete user2_signal_event;
            delete user1_signal_event;
            delete sp_loop;
        };

        int user1_run_time_1 = 0, user2_run_time_1 = 0;
        int user1_run_time_2 = 0, user2_run_time_2 = 0;

        auto t1 = std::thread(std::bind(thread_func, std::ref(user1_run_time_1), std::ref(user2_run_time_1)));
        auto t2 = std::thread(std::bind(thread_func, std::ref(user1_run_time_2), std::ref(user2_run_time_2)));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        raise(SIGUSR1);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        raise(SIGUSR2);

        t1.join();
        t2.join();

        EXPECT_EQ(user1_run_time_1, 1);
        EXPECT_EQ(user2_run_time_1, 1);
        EXPECT_EQ(user1_run_time_2, 1);
        EXPECT_EQ(user2_run_time_2, 1);
    }
}

//! 同一个事件，监听多个事件
TEST(SignalEvent, OneEventMultiSignal)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);
        auto signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(signal_event->initialize({ SIGUSR1, SIGUSR2 }, Event::Mode::kPersist));
        EXPECT_TRUE(signal_event->enable());

        int user1_run_time = 0;
        int user2_run_time = 0;
        signal_event->setCallback(
            [&](int signo) {
                //LogTrace("signo:%d", signo);
                if (signo == SIGUSR1)
                    ++user1_run_time;
                else if (signo == SIGUSR2)
                    ++user2_run_time;
                else {}
            }
        );

        sp_loop->run(
            [] {
                raise(SIGUSR1);
                raise(SIGUSR2);
            }
        );

        sp_loop->exitLoop(std::chrono::milliseconds(100));
        sp_loop->runLoop();

        EXPECT_EQ(user1_run_time, 1);
        EXPECT_EQ(user2_run_time, 1);

        delete signal_event;
        delete sp_loop;
    }
}

//! 短时间内触发非常多的信号
TEST(SignalEvent, LargeNumberOfSignals)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);
        auto signal_event = sp_loop->newSignalEvent();
        EXPECT_TRUE(signal_event->initialize(SIGUSR1, Event::Mode::kPersist));
        EXPECT_TRUE(signal_event->enable());

        int user1_run_time = 0;
        signal_event->setCallback(
            [&](int signo) {
                if (signo == SIGUSR1)
                    ++user1_run_time;
            }
        );

        int remain = 1000;
        std::function<void()> func = \
        [&] {
            --remain;
            if (remain >= 0) {
                raise(SIGUSR1);
                sp_loop->runInLoop(func);
            } else {
                sp_loop->exitLoop(std::chrono::milliseconds(10));
            }
        };
        sp_loop->runInLoop(func);

        sp_loop->runLoop();

        EXPECT_EQ(remain, -1);
        EXPECT_EQ(user1_run_time, 1000);

        delete signal_event;
        delete sp_loop;
    }
}



#include <gtest/gtest.h>
#include <thread>

#include "loop.h"
#include "timer_event.h"

#include <tbox/base/log.h>
#include <tbox/base/log_output.h>
#include <tbox/base/scope_exit.hpp>

namespace tbox {
namespace event {

using namespace std;
using namespace std::chrono;

TEST(CommonLoop, isRunning)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        sp_timer->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_run = false;
        sp_timer->setCallback(
            [sp_loop, &is_run] {
                is_run = true;
                EXPECT_TRUE(sp_loop->isRunning());
            }
        );
        sp_timer->enable();

        EXPECT_FALSE(sp_loop->isRunning());

        sp_loop->exitLoop(chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_TRUE(is_run);
        EXPECT_FALSE(sp_loop->isRunning());
    }
}

TEST(CommonLoop, isInLoopThread)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        sp_timer->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer->setCallback(
            [sp_loop, &is_timer_run] {
                is_timer_run = true;
                EXPECT_TRUE(sp_loop->isInLoopThread());
            }
        );
        sp_timer->enable();

        bool is_thread_run = false;
        auto t = thread(
            [sp_loop, &is_thread_run] {
                is_thread_run = true;
                this_thread::sleep_for(chrono::milliseconds(10));
                EXPECT_FALSE(sp_loop->isInLoopThread());
            }
        );

        sp_loop->exitLoop(chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
        EXPECT_TRUE(is_thread_run);

        t.join();
    }
}

TEST(CommonLoop, runNextInsideLoop)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer1 = sp_loop->newTimerEvent();
        TimerEvent *sp_timer2 = sp_loop->newTimerEvent();
        SetScopeExitAction(
            [sp_loop, sp_timer1, sp_timer2]{
                delete sp_timer2;
                delete sp_timer1;
                delete sp_loop;
            }
        );

        sp_timer1->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_run = false;
        sp_timer1->setCallback(
            [&] {
                sp_loop->runNext([&] { is_run = true; });
            }
        );
        sp_timer1->enable();

        sp_timer2->initialize(chrono::milliseconds(20), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer2->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
                sp_loop->exitLoop();
            }
        );
        sp_timer2->enable();

        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
    }
}

//! runNext() 支持在Loop之前操作
TEST(CommonLoop, runNextBeforeLoop)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        bool is_run_next_run = false;
        sp_loop->runNext([&] { is_run_next_run = true; });

        bool is_timer_run = false;
        sp_timer->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        sp_timer->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run_next_run);
                sp_loop->exitLoop();
            }
        );
        sp_timer->enable();

        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
    }
}

TEST(CommonLoop, runNextForever)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);

        int count = 0;
        std::function<void()> func;
        func = [&] {
            ++count;
            sp_loop->runNext(func);
        };

        sp_loop->runNext(func);

        sp_loop->exitLoop(chrono::milliseconds(1));
        sp_loop->runLoop();

        EXPECT_GT(count, 1);
        cout << "count:" << count << endl;

        delete sp_loop;
    }
}

TEST(CommonLoop, runInLoopInsideLoop)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer1 = sp_loop->newTimerEvent();
        TimerEvent *sp_timer2 = sp_loop->newTimerEvent();
        SetScopeExitAction(
            [sp_loop, sp_timer1, sp_timer2]{
                delete sp_timer2;
                delete sp_timer1;
                delete sp_loop;
            }
        );

        sp_timer1->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_run = false;
        sp_timer1->setCallback(
            [&] {
                sp_loop->runInLoop([&] { is_run = true; });
            }
        );
        sp_timer1->enable();

        sp_timer2->initialize(chrono::milliseconds(20), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer2->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
                sp_loop->exitLoop();
            }
        );
        sp_timer2->enable();

        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
    }
}

TEST(CommonLoop, runInLoopBeforeLoop)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        bool is_run = false;
        sp_loop->runInLoop([&] { is_run = true; });

        sp_timer->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
            }
        );
        sp_timer->enable();

        sp_loop->exitLoop(chrono::milliseconds(20));
        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
    }
}

TEST(CommonLoop, runInLoopCrossThread)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        bool is_thread_run = false;
        bool is_run = false;
        auto t = thread(
            [&] {
                is_thread_run = true;
                this_thread::sleep_for(chrono::milliseconds(10));
                sp_loop->runInLoop([&]{ is_run = true; });
            }
        );

        sp_timer->initialize(chrono::milliseconds(20), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
            }
        );
        sp_timer->enable();

        sp_loop->exitLoop(chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
        EXPECT_TRUE(is_thread_run);

        t.join();
    }
}

TEST(CommonLoop, runInsideLoop)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer1 = sp_loop->newTimerEvent();
        TimerEvent *sp_timer2 = sp_loop->newTimerEvent();
        SetScopeExitAction(
            [sp_loop, sp_timer1, sp_timer2]{
                delete sp_timer2;
                delete sp_timer1;
                delete sp_loop;
            }
        );

        sp_timer1->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_run = false;
        sp_timer1->setCallback(
            [&] {
                sp_loop->run([&] { is_run = true; });
            }
        );
        sp_timer1->enable();

        sp_timer2->initialize(chrono::milliseconds(20), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer2->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
                sp_loop->exitLoop();
            }
        );
        sp_timer2->enable();

        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
    }
}

TEST(CommonLoop, runBeforeLoop)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        bool is_run = false;
        sp_loop->run([&] { is_run = true; });

        sp_timer->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
            }
        );
        sp_timer->enable();

        sp_loop->exitLoop(chrono::milliseconds(20));
        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
    }
}

TEST(CommonLoop, runCrossThread)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer = sp_loop->newTimerEvent();
        SetScopeExitAction([sp_loop, sp_timer]{ delete sp_timer; delete sp_loop; });

        bool is_thread_run = false;
        bool is_run = false;
        auto t = thread(
            [&] {
                is_thread_run = true;
                this_thread::sleep_for(chrono::milliseconds(10));
                sp_loop->run([&]{ is_run = true; });
            }
        );

        sp_timer->initialize(chrono::milliseconds(40), Event::Mode::kOneshot);
        bool is_timer_run = false;
        sp_timer->setCallback(
            [&] {
                is_timer_run = true;
                EXPECT_TRUE(is_run);
            }
        );
        sp_timer->enable();

        sp_loop->exitLoop(chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_TRUE(is_timer_run);
        EXPECT_TRUE(is_thread_run);

        t.join();
    }
}

TEST(CommonLoop, runForever)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);

        int count = 0;
        std::function<void()> func;
        func = [&] {
            ++count;
            sp_loop->run(func);
        };

        sp_loop->run(func);

        sp_loop->exitLoop(chrono::milliseconds(1));
        sp_loop->runLoop();

        EXPECT_GT(count, 1);
        cout << "count:" << count << endl;
        delete sp_loop;
    }
}

TEST(CommonLoop, cleanupDeferedTask)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        TimerEvent *sp_timer1 = sp_loop->newTimerEvent();
        SetScopeExitAction(
            [&]{
                delete sp_timer1;
                delete sp_loop;
            }
        );

        sp_timer1->initialize(chrono::milliseconds(10), Event::Mode::kOneshot);
        bool is_run_1 = false;
        bool is_run_2 = false;
        bool is_run_3 = false;
        sp_timer1->setCallback(
            [&] {
                sp_loop->runInLoop([&] { is_run_1 = true; });
                sp_loop->runNext([&] { is_run_2 = true; });
                sp_loop->run([&] { is_run_3 = true; });
                sp_loop->exitLoop();
            }
        );
        sp_timer1->enable();

        sp_loop->runLoop();

        EXPECT_TRUE(is_run_1);
        EXPECT_TRUE(is_run_2);
        EXPECT_TRUE(is_run_3);
    }
}

TEST(CommonLoop, cleanupDeferedTask1)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);
        SetScopeExitAction( [&]{ delete sp_loop; });

        std::function<void()> func;
        func = [&] {
            cout << '.' << flush;
            sp_loop->runInLoop(func);
        };
        sp_loop->run(func);

        sp_loop->exitLoop(chrono::milliseconds(10));
        sp_loop->runLoop();
    }
}

TEST(CommonLoop, RunInLoopBenchmark)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);

        int counter = 0;
        function<void()> func = [&] {
            sp_loop->runInLoop(func);
            ++counter;
        };
        sp_loop->runInLoop(func);

        sp_loop->exitLoop(chrono::seconds(10));
        sp_loop->runLoop();

        delete sp_loop;
        cout << "10s count: " << counter << endl;
    }
}

TEST(CommonLoop, RunNextBenchmark)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);

        int counter = 0;
        function<void()> func = [&] {
            sp_loop->runNext(func);
            ++counter;
        };
        sp_loop->runNext(func);

        sp_loop->exitLoop(chrono::seconds(10));
        sp_loop->runLoop();

        delete sp_loop;
        cout << "10s count: " << counter << endl;
    }
}

TEST(CommonLoop, RunBenchmark)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);

        int counter = 0;
        function<void()> func = [&] {
            sp_loop->run(func);
            ++counter;
        };
        sp_loop->run(func);

        sp_loop->exitLoop(chrono::seconds(10));
        sp_loop->runLoop();

        delete sp_loop;
        cout << "10s count: " << counter << endl;
    }
}

TEST(CommonLoop, ExitLoopMultiTimes)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        Loop *sp_loop = event::Loop::New(e);

        sp_loop->runInLoop([sp_loop] { sp_loop->exitLoop(); });
        sp_loop->exitLoop(chrono::seconds(10));
        sp_loop->runLoop();

        delete sp_loop;
    }
}

}
}

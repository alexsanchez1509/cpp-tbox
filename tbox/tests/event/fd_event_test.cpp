#include <gtest/gtest.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <cstring>

#include "loop.h"
#include "fd_event.h"

using namespace std;
using namespace tbox::event;

TEST(FdEvent, DisableSelfInReadCallback)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        int fds[2] = { 0 };
        ASSERT_EQ(pipe2(fds, O_CLOEXEC | O_NONBLOCK), 0);

        int read_fd(fds[0]);
        int write_fd(fds[1]);

        auto sp_loop = Loop::New(e);
        auto read_event = sp_loop->newFdEvent();
        EXPECT_TRUE(read_event->initialize(read_fd, FdEvent::kReadEvent, Event::Mode::kPersist));
        EXPECT_TRUE(read_event->enable());

        int run_time = 0;
        read_event->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kReadEvent);
                char tmp[11] = {0};
                auto rsize = read(read_fd, tmp, 10);
                EXPECT_EQ(rsize, 10);
                EXPECT_STREQ(tmp, "0123456789");
                ++run_time;

                write(write_fd, "0123456789", 10);
                read_event->disable();
            }
        );

        write(write_fd, "0123456789", 10);
        sp_loop->exitLoop(std::chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_EQ(run_time, 1); //! 应该只执行一次

        delete read_event;
        delete sp_loop;

        close(write_fd);
        close(read_fd);
    }
}

TEST(FdEvent, DisableSelfInWriteCallback)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        int fds[2] = { 0 };
        ASSERT_EQ(pipe2(fds, O_CLOEXEC | O_NONBLOCK), 0);

        int read_fd(fds[0]);
        int write_fd(fds[1]);

        auto sp_loop = Loop::New(e);
        auto write_event = sp_loop->newFdEvent();
        EXPECT_TRUE(write_event->initialize(write_fd, FdEvent::kWriteEvent, Event::Mode::kPersist));
        EXPECT_TRUE(write_event->enable());

        int run_time = 0;
        write_event->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kWriteEvent);
                write(write_fd, "0", 1);
                ++run_time;
                write_event->disable();
            }
        );

        sp_loop->exitLoop(std::chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_EQ(run_time, 1); //! 应该只执行一次

        delete write_event;
        delete sp_loop;

        close(write_fd);
        close(read_fd);
    }
}

TEST(FdEvent, OneWriteMultiRead)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        int fds[2] = { 0 };
        ASSERT_EQ(pipe2(fds, O_CLOEXEC | O_NONBLOCK), 0);

        int read_fd(fds[0]);
        int write_fd(fds[1]);

        auto sp_loop = Loop::New(e);

        auto read_event1 = sp_loop->newFdEvent();
        auto read_event2 = sp_loop->newFdEvent();

        EXPECT_TRUE(read_event1->initialize(read_fd, FdEvent::kReadEvent, Event::Mode::kOneshot));
        EXPECT_TRUE(read_event2->initialize(read_fd, FdEvent::kReadEvent, Event::Mode::kOneshot));

        EXPECT_TRUE(read_event1->enable());
        EXPECT_TRUE(read_event2->enable());

        bool event1_run = false;
        bool event2_run = false;

        read_event1->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kReadEvent);
                char tmp[11] = {0};
                auto rsize = read(read_fd, tmp, 10);
                if (rsize == -1)
                    EXPECT_EQ(errno, EAGAIN);
                else
                    EXPECT_STREQ(tmp, "0123456789");
                event1_run = true;
            }
        );
        read_event2->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kReadEvent);
                char tmp[11] = {0};
                auto rsize = read(read_fd, tmp, 10);
                if (rsize == -1)
                    EXPECT_EQ(errno, EAGAIN);
                else
                    EXPECT_STREQ(tmp, "0123456789");
                event2_run = true;
            }
        );

        write(write_fd, "0123456789", 10);
        sp_loop->exitLoop(std::chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_TRUE(event1_run);
        EXPECT_TRUE(event2_run);

        delete read_event2;
        delete read_event1;
        delete sp_loop;

        close(write_fd);
        close(read_fd);
    }
}

TEST(FdEvent, MultiWriteOneRead)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        int fds[2] = { 0 };
        ASSERT_EQ(pipe2(fds, O_CLOEXEC | O_NONBLOCK), 0);

        int read_fd(fds[0]);
        int write_fd(fds[1]);

        auto sp_loop = Loop::New(e);

        auto read_event = sp_loop->newFdEvent();
        auto write_event1 = sp_loop->newFdEvent();
        auto write_event2 = sp_loop->newFdEvent();

        EXPECT_TRUE(read_event->initialize(read_fd, FdEvent::kReadEvent, Event::Mode::kPersist));
        EXPECT_TRUE(write_event1->initialize(write_fd, FdEvent::kWriteEvent, Event::Mode::kOneshot));
        EXPECT_TRUE(write_event2->initialize(write_fd, FdEvent::kWriteEvent, Event::Mode::kOneshot));

        EXPECT_TRUE(read_event->enable());
        EXPECT_TRUE(write_event1->enable());
        EXPECT_TRUE(write_event2->enable());

        bool event_run  = false;
        bool event1_run = false;
        bool event2_run = false;

        read_event->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kReadEvent);
                char recv[21] = {0};
                auto rsize = read(read_fd, recv, 20);
                EXPECT_EQ(rsize, 20);
                event_run = true;
            }
        );
        write_event1->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kWriteEvent);
                auto wsize = write(write_fd, "abcdefghij", 10);
                EXPECT_EQ(wsize, 10);
                event1_run = true;
            }
        );
        write_event2->setCallback(
            [&] (short events) {
                EXPECT_EQ(events, FdEvent::kWriteEvent);
                auto wsize = write(write_fd, "0123456789", 10);
                EXPECT_EQ(wsize, 10);
                event2_run = true;
            }
        );

        sp_loop->exitLoop(std::chrono::milliseconds(50));
        sp_loop->runLoop();

        EXPECT_TRUE(event_run);
        EXPECT_TRUE(event1_run);
        EXPECT_TRUE(event2_run);

        delete write_event2;
        delete write_event1;
        delete read_event;
        delete sp_loop;

        close(write_fd);
        close(read_fd);
    }
}


TEST(FdEvent, DeleteLater)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;
        auto sp_loop = Loop::New(e);
        auto sp_fd_event = sp_loop->newFdEvent();
        sp_fd_event->initialize(1, FdEvent::kReadEvent, Event::Mode::kPersist);
        sp_loop->run([=] { delete sp_fd_event; });
        delete sp_loop;
    }
}

TEST(FdEvent, Benchmark)
{
    auto engins = Loop::Engines();
    for (auto e : engins) {
        cout << "engin: " << e << endl;

        int fds[2] = { 0 };
        ASSERT_EQ(pipe2(fds, O_CLOEXEC | O_NONBLOCK), 0);

        int read_fd(fds[0]);
        int write_fd(fds[1]);

        auto sp_loop = Loop::New(e);
        auto read_event = sp_loop->newFdEvent();
        read_event->initialize(read_fd, FdEvent::kReadEvent, Event::Mode::kPersist);
        read_event->enable();

        int run_time = 0;
        read_event->setCallback(
            [&] (short events) {
                char dummy = 0;
                write(write_fd, &dummy, 1);
                ++run_time;
            }
        );

        char dummy = 0;
        write(write_fd, &dummy, 1);
        sp_loop->exitLoop(std::chrono::seconds(10));
        sp_loop->runLoop();

        cout << "count in 10sec: " << run_time << endl;

        delete read_event;
        delete sp_loop;

        close(write_fd);
        close(read_fd);
    }
}


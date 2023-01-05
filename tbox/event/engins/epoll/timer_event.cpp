#include <sys/epoll.h>
#include <tbox/base/assert.h>
#include "loop.h"
#include "timer_event.h"

namespace tbox {
namespace event {

EpollTimerEvent::EpollTimerEvent(EpollLoop *wp_loop)
    : wp_loop_(wp_loop)
{ }

EpollTimerEvent::~EpollTimerEvent()
{
    TBOX_ASSERT(cb_level_ == 0);
    disable();
}

bool EpollTimerEvent::initialize(const std::chrono::milliseconds &interval, Mode mode)
{
    disable();

    interval_ = interval;
    mode_ = mode;

    is_inited_ = true;
    return true;
}

void EpollTimerEvent::setCallback(const CallbackFunc &cb)
{
    cb_ = cb;
}

bool EpollTimerEvent::isEnabled() const
{
    if (!is_inited_)
        return false;

    return is_enabled_;
}

bool EpollTimerEvent::enable()
{
    if (!is_inited_)
        return false;

    if (isEnabled())
        return true;

    if (wp_loop_)
        token_ = wp_loop_->addTimer(interval_.count(), mode_ == Mode::kOneshot ? 1 : 0, [this]{ onEvent(); });

    is_enabled_ = true;

    return true;
}

bool EpollTimerEvent::disable()
{
    if (!is_inited_)
        return false;

    if (!isEnabled())
        return true;

    if (wp_loop_)
        wp_loop_->deleteTimer(token_);

    is_enabled_ = false;
    return true;
}

Loop* EpollTimerEvent::getLoop() const
{
    return wp_loop_;
}

void EpollTimerEvent::onEvent()
{
    wp_loop_->beginEventProcess();

    if (mode_ == Mode::kOneshot) {
        is_enabled_ = false;
        token_.reset();
    }

    if (cb_) {
        ++cb_level_;
        cb_();
        --cb_level_;
    }

    wp_loop_->endEventProcess();
}

}
}

#include "timer_event.h"

#include <event2/event.h>
#include <tbox/base/log.h>
#include <tbox/base/assert.h>

#include "loop.h"
#include "common.h"

namespace tbox {
namespace event {

LibeventTimerEvent::LibeventTimerEvent(LibeventLoop *wp_loop) :
    wp_loop_(wp_loop)
{
    event_assign(&event_, NULL, -1, 0, NULL, NULL);
}

LibeventTimerEvent::~LibeventTimerEvent()
{
    TBOX_ASSERT(cb_level_ == 0);
    disable();
}

bool LibeventTimerEvent::initialize(const std::chrono::milliseconds &interval, Mode mode)
{
    disable();

    interval_ = Chrono2Timeval(interval);

    short libevent_events = 0;
    if (mode == Mode::kPersist)
        libevent_events |= EV_PERSIST;

    int ret = event_assign(&event_, wp_loop_->getEventBasePtr(), -1, libevent_events, LibeventTimerEvent::OnEventCallback, this);
    if (ret == 0) {
        is_inited_ = true;
        return true;
    }

    LogErr("event_assign() fail");
    return false;
}

void LibeventTimerEvent::setCallback(const CallbackFunc &cb)
{
    cb_ = cb;
}

bool LibeventTimerEvent::isEnabled() const
{
    if (!is_inited_)
        return false;

    return event_pending(&event_, EV_TIMEOUT, NULL) != 0;
}

bool LibeventTimerEvent::enable()
{
    if (!is_inited_) {
        LogErr("can't enable() before initialize()");
        return false;
    }

    if (isEnabled())
        return true;

    int ret = event_add(&event_, &interval_);
    if (ret != 0) {
        LogErr("event_add() fail");
        return false;
    }

    return true;
}

bool LibeventTimerEvent::disable()
{
    if (!is_inited_)
        return false;

    if (!isEnabled())
        return true;

    int ret = event_del(&event_);
    if (ret != 0) {
        LogErr("event_del() fail");
        return false;
    }

    return true;
}

Loop* LibeventTimerEvent::getLoop() const
{
    return wp_loop_;
}

void LibeventTimerEvent::OnEventCallback(int, short, void *args)
{
    LibeventTimerEvent *pthis = static_cast<LibeventTimerEvent*>(args);
    pthis->onEvent();
}

void LibeventTimerEvent::onEvent()
{
    wp_loop_->beginEventProcess();

    if (cb_) {
        ++cb_level_;
        cb_();
        --cb_level_;
    } else {
        LogWarn("you should specify event callback by setCallback()");
    }

    wp_loop_->endEventProcess();
}

}
}

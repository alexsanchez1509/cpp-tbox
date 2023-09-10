/*
 *     .============.
 *    //  M A K E  / \
 *   //  C++ DEV  /   \
 *  //  E A S Y  /  \/ \
 * ++ ----------.  \/\  .
 *  \\     \     \ /\  /
 *   \\     \     \   /
 *    \\     \     \ /
 *     -============'
 *
 * Copyright (c) 2018 Hevake and contributors, all rights reserved.
 *
 * This file is part of cpp-tbox (https://github.com/cpp-main/cpp-tbox)
 * Use of this source code is governed by MIT license that can be found
 * in the LICENSE file in the root of the source tree. All contributing
 * project authors may be found in the CONTRIBUTORS.md file in the root
 * of the source tree.
 */
#include <tbox/base/log.h>
#include <tbox/event/signal_event.h>
#include <tbox/eventx/timer_pool.h>
#include <tbox/http/server/server.h>
#include <tbox/log/async_stdout_sink.h>

#include <tbox/base/scope_exit.hpp>

using namespace tbox;
using namespace tbox::event;
using namespace tbox::eventx;
using namespace tbox::http;
using namespace tbox::http::server;

int main(int argc, char **argv)
{
    std::string bind_addr = "0.0.0.0:12345";

    if (argc == 2) {
        bind_addr = argv[1];
    }

    log::AsyncStdoutSink log;
    log.enable();
    log.enableColor(true);
    log.setLevel(LOG_LEVEL_TRACE);

    LogInfo("enter");

    auto sp_loop = Loop::New();
    auto sp_sig_event = sp_loop->newSignalEvent();

    TimerPool timers(sp_loop);

    SetScopeExitAction([=] {
        delete sp_sig_event;
        delete sp_loop;
    });

    sp_sig_event->initialize(SIGINT, Event::Mode::kPersist);
    sp_sig_event->enable();

    Server srv(sp_loop);
    if (!srv.initialize(network::SockAddr::FromString(bind_addr), 1)) {
        LogErr("init srv fail");
        return 0;
    }

    srv.start();
    srv.setContextLogEnable(true);

    //! 添加请求处理
    srv.use([&](ContextSptr ctx, const NextFunc &next) {
        if (ctx->req().url.path == "/") {
            ctx->res().status_code = StatusCode::k200_OK;
            ctx->res().body =
                R"(
<head>
</head>
<body>
    <p> <a href="/1" target="_blank">delay</a> </p>
    <p> <a href="/2" target="_blank">now</a> </p>
</body>
)";
        } else if (ctx->req().url.path == "/1") {
            timers.doAfter(std::chrono::seconds(10), [ctx] {
                ctx->res().status_code = StatusCode::k200_OK;
                ctx->res().body = ctx->req().url.path;
            });
        } else if (ctx->req().url.path == "/2") {
            ctx->res().status_code = StatusCode::k200_OK;
            ctx->res().body = ctx->req().url.path;
        }
    });

    sp_sig_event->setCallback([&](int) {
        srv.stop();
        sp_loop->exitLoop();
    });

    LogInfo("start");
    sp_loop->runLoop();
    LogInfo("stop");
    srv.cleanup();

    LogInfo("exit");
    return 0;
}

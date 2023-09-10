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
#include <tbox/event/loop.h>
#include <tbox/network/udp_socket.h>
#include <time.h>

#include <iostream>
#include <tbox/base/scope_exit.hpp>

using namespace std;
using namespace tbox::network;
using namespace tbox::event;

int main()
{
    Loop *sp_loop = Loop::New();
    SetScopeExitAction([sp_loop] { delete sp_loop; });

    UdpSocket respond(sp_loop);
    respond.bind(SockAddr::FromString("0.0.0.0:6668"));
    respond.setRecvCallback(
        [&respond](const void *data_ptr, size_t data_size, const SockAddr &from) {
            const char *str = (const char *)data_ptr;
            if (string(str) == "time?") {
                time_t now = time(nullptr);
                respond.send(&now, sizeof(now), from);
            }
        });
    respond.enable();

    sp_loop->runLoop();
    return 0;
}

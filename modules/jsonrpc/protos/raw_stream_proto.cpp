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
 * Copyright (c) 2023 Hevake and contributors, all rights reserved.
 *
 * This file is part of cpp-tbox (https://github.com/cpp-main/cpp-tbox)
 * Use of this source code is governed by MIT license that can be found
 * in the LICENSE file in the root of the source tree. All contributing
 * project authors may be found in the CONTRIBUTORS.md file in the root
 * of the source tree.
 */
#include "raw_stream_proto.h"

#include <tbox/base/assert.h>
#include <tbox/base/catch_throw.h>
#include <tbox/util/json.h>

#include <tbox/base/json.hpp>

namespace tbox {
namespace jsonrpc {

void RawStreamProto::sendJson(const Json &js)
{
    if (send_data_cb_) {
        const auto &json_text = js.dump();
        send_data_cb_(json_text.data(), json_text.size());
    }
}

ssize_t RawStreamProto::onRecvData(const void *data_ptr, size_t data_size)
{
    TBOX_ASSERT(data_ptr != nullptr);

    if (data_size < 2) return 0;

    const char *str_ptr = static_cast<const char *>(data_ptr);
    auto str_len = util::json::FindEndPos(str_ptr, data_size);
    if (str_len > 0) {
        Json js;
        bool is_throw = tbox::CatchThrow([&] { js = Json::parse(str_ptr, str_ptr + str_len); });
        if (is_throw) {
            LogNotice("parse json fail");
            return -1;
        }

        onRecvJson(js);
        return str_len;
    }
    return 0;
}

}  // namespace jsonrpc
}  // namespace tbox

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
#ifndef TBOX_LOG_ASYNC_STDOUT_SINK_H_20220408
#define TBOX_LOG_ASYNC_STDOUT_SINK_H_20220408

#include "async_sink.h"

#include <vector>

namespace tbox {
namespace log {

class AsyncStdoutSink : public AsyncSink {
  public:
    AsyncStdoutSink();

  protected:
    virtual void endline() override;
    virtual void flush() override;

  private:
    std::vector<char> buffer_;
};

}
}

#endif //TBOX_LOG_ASYNC_STDOUT_SINK_H_20220408

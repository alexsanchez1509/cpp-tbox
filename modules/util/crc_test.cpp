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
#include "crc.h"

#include <gtest/gtest.h>

namespace tbox {
namespace util {

TEST(Crc, CalcCrc16)
{
    uint8_t tmp_data[] = {0x02, 0x0A, 0x00, 0x03, 0xE6, 0x02, 0x08, 0x03};
    uint16_t crc = CalcCrc16(tmp_data, sizeof(tmp_data));
    EXPECT_EQ(crc, 0xD7DD);
}

}  // namespace util
}  // namespace tbox

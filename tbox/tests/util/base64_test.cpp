#include <gtest/gtest.h>
#include "base64.h"

namespace tbox {
namespace util {
namespace base64 {

TEST(Base64, EncodeLength) {
    EXPECT_EQ(EncodeLength(0), 0);
    EXPECT_EQ(EncodeLength(1), 4);
    EXPECT_EQ(EncodeLength(2), 4);
    EXPECT_EQ(EncodeLength(3), 4);
    EXPECT_EQ(EncodeLength(4), 8);
    EXPECT_EQ(EncodeLength(5), 8);
}

TEST(Base64, DecodeLength) {
    EXPECT_EQ(DecodeLength("MQ"), 0);
    EXPECT_EQ(DecodeLength("MQ="), 0);
    EXPECT_EQ(DecodeLength("MQ=="), 1);
    EXPECT_EQ(DecodeLength("MTI="), 2);
    EXPECT_EQ(DecodeLength("MTIz"), 3);
    EXPECT_EQ(DecodeLength("MTIzNA=="), 4);
}

TEST(Base64, Encode) {
    const char* in = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char out[37] = {0};
    EXPECT_EQ(Encode(in, 26, out, 36), 36);
    EXPECT_STREQ(out, "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=");
}

TEST(Base64, Encode_ReturnString) {
    const char* in = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    EXPECT_EQ(Encode(in, 26), "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=");
}

TEST(Base64, Encode_LenthNotEnough) {
    const char* in = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char out[36] = {0};
    EXPECT_EQ(Encode(in, 26, out, 35), 0);
}

TEST(Base64, Decode) {
    const char* in = "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=";
    char out[27] = {0};
    EXPECT_EQ(Decode(in, 36, out, 26), 26);
    EXPECT_STREQ(out, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

TEST(Base64, Decode_LenthNotEnough) {
    const char* in = "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=";
    char out[27] = {0};
    EXPECT_EQ(Decode(in, 36, out, 25), 0);
}

}
}
}

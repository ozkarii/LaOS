#include <gtest/gtest.h>

extern "C" {
#include "libc-impl.h"
}

#include <stdarg.h>

static int call_vsnprintf_impl(char* buf, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf_impl(buf, size, format, args);
    va_end(args);
    return written;
}

TEST(VsnprintfTest, FormatsIntegerCorrectly) {
    char dest[50] = {};

    int written = call_vsnprintf_impl(dest, sizeof(dest), "Number: %d\n", 100);

    EXPECT_EQ(written, 12);
    EXPECT_STREQ(dest, "Number: 100\n");
}

TEST(VsnprintfTest, FormatsStringAndUnsignedInteger) {
    char dest[50] = {};

    int written = call_vsnprintf_impl(dest, sizeof(dest), "%s %u\n", "test", 42u);

    EXPECT_EQ(written, 8);
    EXPECT_STREQ(dest, "test 42\n");
}

TEST(VsnprintfTest, HandlesPercentLiteral) {
    char dest[50] = {};

    int written = call_vsnprintf_impl(dest, sizeof(dest), "100%% done\n");

    EXPECT_EQ(written, 10);
    EXPECT_STREQ(dest, "100% done\n");
}

TEST(VsnprintfTest, HandlesNullStringPointer) {
    char dest[50] = {};

    int written = call_vsnprintf_impl(dest, sizeof(dest), "%s", static_cast<const char*>(nullptr));

    EXPECT_EQ(written, 6);
    EXPECT_STREQ(dest, "(null)");
}

TEST(VsnprintfTest, TruncatesOutputToBufferSize) {
    char dest[8] = {};

    int written = call_vsnprintf_impl(dest, sizeof(dest), "%s", "hello world");

    EXPECT_EQ(written, 7);
    EXPECT_STREQ(dest, "hello w");
}

TEST(VsnprintfTest, HandlesMixedFormatSpecifiers) {
    char dest[64] = {};

    int written = call_vsnprintf_impl(dest, sizeof(dest), "Name: %s, Age: %d, Hex: %x\n", "Alice", 30, 0x2a);

    EXPECT_EQ(written, 30);
    EXPECT_STREQ(dest, "Name: Alice, Age: 30, Hex: 2a\n");
}

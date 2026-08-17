#include <gtest/gtest.h>

extern "C" {
#include "libc-impl.h"
}

/* BASIC FUNCTIONALITY */
TEST(StrcpyTest, CopiesStringCorrectly) {
    char src[] = "hello";
    char dest[6] = {0};

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, src);
}

TEST(StrcpyTest, CopiesEmptyString) {
    char src[] = "";
    char dest[1] = {'x'};

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, "");
}

TEST(StrcpyTest, CopiesLongerString) {
    char src[] = "abcdefghi";
    char dest[20] = "xyz";

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, src);
}

TEST(StrcpyTest, HandlesSpecialCharacters) {
    char src[] = "Line1\nLine2\tTabbed\rCarriage!@#$%^&*()";
    char dest[128] = {0};

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, src);
}

TEST(StrcpyTest, OverwritesExistingBufferContents) {
    char src[] = "abc";
    char dest[] = "uvwxyz";

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, "abc");
}

/* EDGE CASES */
TEST(StrcpyTest, HandlesSameSourceAndDestination) {
    char buffer[] = "abc";

    char* result = strcpy_impl(buffer, buffer);

    EXPECT_EQ(result, buffer);
    EXPECT_STREQ(buffer, "abc");
}

TEST(StrcpyTest, CopiesToBufferWithTrailingGarbage) {
    char src[] = "hello";
    char dest[10] = {'x', 'y', 'z', 'z', 'z', 'z', 'z', 'z', 'z', 'z'};

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, src);
}

TEST(StrcpyTest, CopiesStringWithEmbeddedNullTerminatorBoundary) {
    char src[] = {'h', 'i', '\0', 'x', 'y', 'z'};
    char dest[10] = {0};

    char* result = strcpy_impl(dest, src);

    EXPECT_EQ(result, dest);
    EXPECT_STREQ(dest, "hi");
}

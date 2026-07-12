#include <gtest/gtest.h>

extern "C" {
#include "libc-impl.h"
}

/* CORE FUNCTIONALITY TESTS */
TEST(StrlenTest, CorrectStringLength) {
    char str[] = "hello";

    EXPECT_EQ(strlen_impl(str), 5);
}

TEST(StrlenTest, EmptyStringReturnsZero) { 
    char str[] = "";

    EXPECT_EQ(strlen_impl(str), 0);
}

TEST(StrlenTest, NonEmptyStringLength) {
    char str[] = "a"; /* at least one character long */

    EXPECT_EQ(strlen_impl(str), 1);
}

TEST(StrlenTest, StringWithSpaces) { 
    char str[] = "hello world!";

    EXPECT_EQ(strlen_impl(str), 12);
}

TEST(StrlenTest, StringWithSpecialChars) { 
    char str[] = "Line1\nLine2\tTabbed\rCarriage!@#$%^&*()";

    EXPECT_EQ(strlen_impl(str), 37);
}

TEST(StrlenTest, StringWithEmbeddedNullCharacters) { 
    char str[] = {'h', 'e', 'l', '\0', 'x', 'y', 'z'};

    /* Only prints until null terminator */
    EXPECT_EQ(strlen_impl(str), 3);
}

TEST(StrlenTest, MultiByteString) { 
    char str[] = u8"café";

    /* 'é' takes 2 bytes */
    EXPECT_EQ(strlen_impl(str), 5);
}

/* HANDLING INVALID PARAMETERS */
TEST(StrlenTest, NullPointerInput) { 
    EXPECT_EQ(strlen_impl(NULL), -1);
}

/* ADDITIONAL TESTS */
// VeryLongStringPerformance // Tests performance with extremely long strings
TEST(StrlenTest, VeryLongStringPerformance) { 
    /* Checks performance with very long strings (over millions of chars). */ 
    const size_t len = 1000000;
    char* str = new char[len + 1]; /* allocate array */

    memset(str, 'a', len);

    EXPECT_EQ(strlen_impl(str), len);

    delete[] str;
}

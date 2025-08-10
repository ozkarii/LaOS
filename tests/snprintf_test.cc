#include <gtest/gtest.h>
#include "../src/libc/include/string.h"

/* CORE FUNCTIONALITY TESTS */
TEST(SnprintfTest, CopiesBytesCorrectly) {
    char src[] = "hello";
    char dest[6] = {0};

    snprintf(dest, sizeof(dest), "%s\n", src);

    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(dest[i], src[i]);
    }
}

TEST(SnprintfTest, FormatsIntegerCorrectly) {
    char dest[50] = {};
    int n = 100;

    int written = snprintf(dest, sizeof(dest), "Number: %d\n", n);

    const char* expected = "Number: 100\n";

    EXPECT_EQ(written, 12);
    EXPECT_STREQ(dest, expected);
}

TEST(SnprintfTest, FormatsStringWithNewline) {
    char dest[20] = {};

    int written = snprintf(dest, sizeof(dest), "%s\n", "test");

    EXPECT_EQ(written, 5);
    EXPECT_STREQ(dest, "test\n");
}

TEST(SnprintfTest, HandlesEmptyFormatString) {
    char dest[10] = {};

    int written = snprintf(dest, sizeof(dest), "");

    EXPECT_EQ(written, 0);
    EXPECT_EQ(dest[0], '\0');
}

/* EDGE CASES AND VALIDATIONS */
TEST(SnprintfTest, CopyEmptyString) {
    char src[] = "";
    char dest[10] = {};

    int written = snprintf(dest, sizeof(dest), "%s\n", src);

    EXPECT_EQ(written, 1); /* Just '\n' */
    EXPECT_STREQ(dest, "\n");
}

TEST(SnprintfTest, WritesNullTerminatorOnExactFit) {
    char src[] = "hello";
    char dest[7] = {}; /* "hello\n" + '\0' */

    int written = snprintf(dest, sizeof(dest), "%s\n", src);

    EXPECT_STREQ(dest, "hello\n");
    EXPECT_EQ(written, 6);
}

TEST(SnprintfTest, OverwritesDestination) {
    char src[] = "hello world!";
    char dest[10] = {};  /* Bufferr too small for full output */

    int written = snprintf(dest, sizeof(dest), "%s\n", src);

    EXPECT_EQ(written, 13);
}

TEST(SnprintfTest, DoesNotWriteBeyondBuffer) {
    char src[] = "hello world!";
    char dest[20];
    memset(dest, 'X', sizeof(dest));

    int written = snprintf(dest, sizeof(dest), "%s\n", src);

    EXPECT_EQ(written, 13);
    EXPECT_EQ(dest[13], '\0');

    for (int i = 14; i < 20; ++i) {
        EXPECT_EQ(dest[i], 'X') << "Byte " << i << " was unexpectedly modified";
    }
}

/* FORMAT STRING HANDLING */
TEST(SnprintfTest, HandlesMultipleFormatSpecifiers) {
    char dest[50] = {};
    const char* name = "Alice";
    int age = 30;

    int written = snprintf(dest, sizeof(dest), "Name: %s, Age: %d\n", name, age);

    const char* expected = "Name: Alice, Age: 30\n";

    EXPECT_EQ(written, 21);
    EXPECT_STREQ(dest, expected);
}

TEST(SnprintfTest, HandlesPercentLiteral) {
    char dest[50] = {};
    const char* src = "%%";

    int written = snprintf(dest, sizeof(dest), "%s\n", src);

    const char* expected = "%%\n";

    EXPECT_EQ(written, 3);
    EXPECT_STREQ(dest, expected);
}

TEST(SnprintfTest, HandlesEscapeCharactersInFormat) {
    char dest[50] = {};

    int written = snprintf(dest, sizeof(dest), "Line1\nLine2\tTabbed\rCarriage");

    const char* expected = "Line1\nLine2\tTabbed\rCarriage";

    EXPECT_EQ(written, 27);
    EXPECT_STREQ(dest, expected);
}

/* SPECIAL CONDITIONS */
TEST(SnprintfTest, HandlesLongInputString) {
    char src[1000];
    char dest[1050];

    memset(src, 'X', sizeof(src));
    src[999] = '\0';

    int written = snprintf(dest, sizeof(dest), "%s\n", src);

    EXPECT_EQ(written, 1000); /* 999 'X' + '\n' */
    EXPECT_EQ(dest[0], 'X');
    EXPECT_EQ(dest[500], 'X');
    EXPECT_EQ(dest[999], '\n');
    EXPECT_EQ(dest[1000], '\0');
}

TEST(SnprintfTest, HandlesNullFormatString) {
    char dest[10] = {};

    int written = snprintf(dest, sizeof(dest), NULL);

    EXPECT_EQ(written, -1);
}

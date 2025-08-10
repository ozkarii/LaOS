#include <gtest/gtest.h>
#include "../src/libc/include/string.h"


/* CORE FUNCTIONALITY TESTS */
TEST(StrcmpTest, IdenticalStringsEqual) {
    char str1[] = "abc";
    char str2[] = "abc";

    int res = strcmp(str1, str2);

    EXPECT_EQ(res, 0);
}

TEST(StrcmpTest, LexicographicallySmallerStringReturnsNegative) {
    char str1[] = "abc";
    char str2[] = "abd";

    int res = strcmp(str1, str2);

    EXPECT_LT(res, 0);
}

TEST(StrcmpTest, LexicographicallyGreaterStringReturnsPositive) {
    char str1[] = "abd";
    char str2[] = "abc";

    int res = strcmp(str1, str2);

    EXPECT_GT(res, 0);
}

TEST(StrcmpTest, DifferenceAtLastCharacter) {
    char str1[] = "hellO";
    char str2[] = "hello";

    int res = strcmp(str1, str2);

    EXPECT_LT(res, 0);
}

TEST(StrcmpTest, PrefixStringsComparison) {
    char str1[] = "hello";
    char str2[] = "hello world!";

    int res = strcmp(str1, str2);

    EXPECT_LT(res, 0);
}

/* EDGE CASES */
TEST(StrcmpTest, EmptyStringsAreEqual) {
    char str1[] = "";
    char str2[] = "";

    int res = strcmp(str1, str2);

    EXPECT_EQ(res, 0);
}

TEST(StrcmpTest, EmptyStringVsNonEmptyReturnsNegative) {
    char str1[] = "";
    char str2[] = "hello";

    int res = strcmp(str1, str2);

    EXPECT_LT(res, 0);
}

TEST(StrcmpTest, NonEmptyStringVsEmptyReturnsPositive) {
    char str1[] = "hello";
    char str2[] = "";

    int res = strcmp(str1, str2);

    EXPECT_GT(res, 0);
}

TEST(StrcmpTest, StringsWithEmbeddedNullCharacters) {
    char str1[] = {'h', 'e', 'l', '\0', 'x', 'y', 'z'};
    char str2[] = {'h', 'e', 'l', '\0', 'a', 'b', 'c'};

    int res = strcmp(str1, str2);

    EXPECT_EQ(res, 0);
}

TEST(StrcmpTest, StringsWithSpecialCharacters) {
    char str1[] = "Line1\nLine2\tTabbed\rCarriage!@#$%^&*()";
    char str2[] = "Line1\nLine2\tTabbed\rCarriage!@#$%^&*()";

    int res = strcmp(str1, str2);

    EXPECT_EQ(res, 0);
}

/* HANDLING INVALID PARAMETERS */
TEST(StrcmpTest, NullPointerAsFirstArgument) {
    char str2[] = "hello";

    int res = strcmp(NULL, str2);

    EXPECT_EQ(res, -1);
}

TEST(StrcmpTest, NullPointerAsSecondArgument) {
    char str1[] = "hello";

    int res = strcmp(str1, NULL);

    EXPECT_EQ(res, -1);
}

TEST(StrcmpTest, BothPointersNull) {
    int res = strcmp(NULL, NULL);

    EXPECT_EQ(res, -1);
}

/* CHECK RETURN VALUES */
TEST(StrcmpTest, ReturnsZeroForEqualStrings) {
    char str1[] = "hello";
    char str2[] = "hello";

    int res = strcmp(str1, str2);

    EXPECT_EQ(res, 0);
}

TEST(StrcmpTest, ReturnsNegativeForStr1LessThanStr2) {
    char str1[] = "hello";
    char str2[] = "hi";

    int res = strcmp(str1, str2);

    EXPECT_LT(res, 0);
}

TEST(StrcmpTest, ReturnsPositiveForStr1GreaterThanStr2) {
    char str1[] = "hi";
    char str2[] = "hello";

    int res = strcmp(str1, str2);

    EXPECT_GT(res, 0);
}

/* ADDITIONAL TESTS */
TEST(StrcmpTest, VeryLongStringsPerformance) {
    const size_t len = 1000000;
    char* str1 = new char[len + 1]; /* allocate array */
    char* str2 = new char[len + 1];

    memset(str1, 'a', len);
    memset(str2, 'a', len);

    str1[len] = '\0';
    str2[len] = '\0';

    int res = strcmp(str1, str2);
    EXPECT_EQ(res, 0);

    delete[] str1; /* free array */
    delete[] str2;
}

TEST(StrcmpTest, UTF8MultibyteCharactersComparison) {
    const char str1[] = u8"café";
    const char str2[] = u8"cafe";

    int res = strcmp(str1, str2);

    EXPECT_GT(res, 0);
}

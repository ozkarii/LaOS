#include <gtest/gtest.h>
#include "../src/libc/include/string.h"

/* BASIC FUNCTIONALITY */
TEST(MemcpyTest, CopiesBytesCorrectly) {
    char src[] = "hello";
    char dest[6] = {0};

    /* Copy hello + \0 */
    memcpy(dest, src, sizeof(src));

    EXPECT_STREQ(dest, src);
}

TEST(MemcpyTest, CopiesPartialBuffer) {
    char src[] = "-abcdefg";
    char dest[10] = "xyz"; /* buffer size big enough */

    /* Copies only 'abc' */
    memcpy(dest + 3, src, 4);

    EXPECT_STREQ(dest, "xyz-abc");
}

TEST(MemcpyTest, HandlesLargeCopy) {
    char src[1000];
    char dest[1050];

    memset(src, 'X', sizeof(src));
    src[999] = '\0';

    memcpy(dest, src, sizeof(src));

    EXPECT_EQ(dest[999], '\0');

    EXPECT_STREQ(dest, src);
}

TEST(MemcpyTest, WorksWithBinaryData) {
    unsigned char src[] = {0x00, 0x01, 0xFF, 0x42, 0x00};
    unsigned char dest[5] = {};

    memcpy(dest, src, sizeof(src));


    for (size_t i = 0; i < sizeof(src); ++i) {
        EXPECT_EQ(dest[i], src[i]);
    }
}

/* STRUCT HANDLING */
struct MyStruct {
    int id;
    double value;
    char name[10];
};

TEST(MemcpyTest, WorksWithStructCopy) {
    MyStruct original = {42, 3.14, "Test"};
    MyStruct copy;

    memcpy(&copy, &original, sizeof(MyStruct));

    EXPECT_EQ(copy.id, original.id);
    EXPECT_DOUBLE_EQ(copy.value, original.value);
    EXPECT_STREQ(copy.name, original.name);
}

/* NULL POINTER HANDLING */
TEST(MemcpyTest, HandlesZeroLength) {
    char src[] = "abc";
    char dest[] = "xyz";

    memcpy(dest, src, 0);

    /* dest should remain unchanged */
    EXPECT_EQ(dest[0], 'x');
    EXPECT_EQ(dest[1], 'y');
    EXPECT_EQ(dest[2], 'z');
    EXPECT_EQ(dest[3], '\0');
}

TEST(MemcpyTest, HandlesNullSourcePointer) {
    char dest[] = "xyz";

    memcpy(dest, NULL, 0);

    /* dest should remain unchanged */
    EXPECT_EQ(dest[0], 'x');
    EXPECT_EQ(dest[1], 'y');
    EXPECT_EQ(dest[2], 'z');
}

TEST(MemcpyTest, HandlesNullDestinationPointer) {
    const char src[] = "abc";
    char dest[] = {'x', 'y', 'z'};

    EXPECT_NO_THROW(memcpy(NULL, src, 0));

    /* dest should remain unchanged */
    EXPECT_EQ(dest[0], 'x');
    EXPECT_EQ(dest[1], 'y');
    EXPECT_EQ(dest[2], 'z');
}

TEST(MemcpyTest, HandlesBothPointersNull) {
    char dest[] = {'x', 'y', 'z'};

    EXPECT_NO_THROW(memcpy(NULL, NULL, 0));

    /* dest should remain unchanged */
    EXPECT_EQ(dest[0], 'x');
    EXPECT_EQ(dest[1], 'y');
    EXPECT_EQ(dest[2], 'z');
}

/* EDGE CASES */
TEST(MemcpyTest, HandlesSameSourceAndDestination) {
    char buffer[] = "abc";

    void* result = memcpy(buffer, buffer, 3);

    EXPECT_STREQ(buffer, "abc");

    /* Returned pointer should match destination */
    EXPECT_EQ(result, buffer);
}

TEST(MemcpyTest, DoesNotWriteOutOfBounds) {
    char src[] = "-abcdefg";
    char dest[8] = "xyz";

    memcpy(dest + 3, src, 4);

    EXPECT_STREQ(dest, "xyz-abc");
}

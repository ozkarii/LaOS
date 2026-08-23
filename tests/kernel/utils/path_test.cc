#include <gtest/gtest.h>

extern "C" {
#include "path.h"
}

class PathTest : public ::testing::Test {};

TEST_F(PathTest, ValidPaths) {
  EXPECT_TRUE(is_valid_path("/valid/path"));
  EXPECT_TRUE(is_valid_path("/another/valid/path"));
  EXPECT_TRUE(is_valid_path("/valid/path_with-dashes_and_underscores"));
}

TEST_F(PathTest, InvalidPaths) {
  EXPECT_FALSE(is_valid_path("relative/path"));
  EXPECT_FALSE(is_valid_path("/invalid//path"));
  EXPECT_FALSE(is_valid_path("/invalid/path/with/illegal|char"));
  EXPECT_FALSE(is_valid_path("/invalid/path/with/space "));
  EXPECT_FALSE(is_valid_path("/invalid/path/with/too/many/characters/abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
  EXPECT_FALSE(is_valid_path(""));
  EXPECT_FALSE(is_valid_path(nullptr));
}

TEST_F(PathTest, GetPathWithoutMountPoint) {
  const char* mount_point = "/mnt";
  const char* path = "/mnt/some/path";
  const char* expected = "/some/path";

  const char* result = get_path_without_mount_point(path, mount_point);
  EXPECT_STREQ(result, expected);
}

TEST_F(PathTest, GetPathWithoutMountPointRoot) {
  const char* mount_point = "/";
  const char* path = "/some/path";
  const char* expected = "/some/path";

  const char* result = get_path_without_mount_point(path, mount_point);
  EXPECT_STREQ(result, expected);
}

TEST_F(PathTest, StripPrefix) {
  const char* src = "/mnt/some/path";
  const char* prefix = "/mnt";
  const char* expected = "/some/path";

  char* result = strip_prefix(src, prefix);
  EXPECT_STREQ(result, expected);
}

TEST_F(PathTest, StripPrefixNoMatch) {
  const char* src = "/mnt/some/path";
  const char* prefix = "/other";
  const char* expected = "/mnt/some/path";

  char* result = strip_prefix(src, prefix);
  EXPECT_STREQ(result, expected);
}

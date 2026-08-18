#include <gtest/gtest.h>

extern "C" {
#include "vfs.h"
extern VFS *vfs_instance;
}

class VfsTest : public ::testing::Test {
protected:
	void SetUp() override {
    vfs = (VFS*)malloc(sizeof(VFS));
    ASSERT_NE(vfs, nullptr);
    ASSERT_NE(vfs_init(vfs), -1);
  }
	void TearDown() override {
    if (vfs) {
      free(vfs);
    }
  }

  VFS *vfs = nullptr;
};

TEST(VfsInitTest, TestInitSuccess) {
  VFS vfs;
  ASSERT_NE(vfs_init(&vfs), -1);
  ASSERT_EQ(vfs_instance, &vfs);
  ASSERT_TRUE(vfs.initialized);
  for (int i = 0; i < sizeof(vfs.mounts) / sizeof(vfs.mounts[0]); ++i) {
    ASSERT_FALSE(vfs.mounts[i].active);
  }
  for (int i = 0; i < sizeof(vfs.open_files) / sizeof(vfs.open_files[0]); ++i) {
    ASSERT_FALSE(vfs.open_files[i].allocated);
  }
}

TEST(VfsInitTest, TestInitFailureOnNull) {
  ASSERT_EQ(vfs_init(nullptr), -1);
}

TEST(VfsInitTest, TestInitFailureOnAlreadyInitialized) {
  VFS vfs;
  ASSERT_NE(vfs_init(&vfs), -1);
  ASSERT_EQ(vfs_init(&vfs), -1);
}

TEST_F(VfsTest, TestRootMountSuccess) {
  VFSInterface fs_interface = {};
  void* fs_data = reinterpret_cast<void*>(0x1234);
  ASSERT_EQ(vfs_mount("/", &fs_interface, fs_data), 0);
  ASSERT_EQ(vfs_instance->mounts[0].active, true);
  ASSERT_STREQ(vfs_instance->mounts[0].path, "/");
  ASSERT_EQ(vfs_instance->mounts[0].fs, &fs_interface);
  ASSERT_EQ(vfs_instance->mounts[0].fs_data, fs_data);
}
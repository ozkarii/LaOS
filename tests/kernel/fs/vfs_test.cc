#include <gtest/gtest.h>

extern "C" {
#include "vfs.h"
extern VFS *vfs_instance;
}

class VfsInitTest : public ::testing::Test {
protected:
    void SetUp() override {
      vfs_instance = nullptr;
    }
};

class VfsTest : public VfsInitTest {
protected:
    void SetUp() override {
      VfsInitTest::SetUp();
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

TEST_F(VfsInitTest, InitSuccess) {
  VFS vfs;
  ASSERT_NE(vfs_init(&vfs), -1);
  ASSERT_EQ(vfs_instance, &vfs);
  ASSERT_TRUE(vfs.initialized);
  for (int i = 0; i < MAX_MOUNTS; ++i) {
    ASSERT_FALSE(vfs.mounts[i].active);
  }
  for (int i = 0; i < MAX_OPEN_FILES; ++i) {
    ASSERT_FALSE(vfs.open_files[i].allocated);
  }
}

TEST_F(VfsInitTest, InitFailureOnNull) {
  ASSERT_EQ(vfs_init(nullptr), -1);
}

TEST_F(VfsInitTest, InitFailureOnAlreadyInitialized) {
  VFS vfs;
  ASSERT_NE(vfs_init(&vfs), -1);
  ASSERT_EQ(vfs_init(&vfs), -1);
}

TEST_F(VfsTest, RootMountSuccess) {
  VFSInterface fs_interface = {};
  void* fs_data = reinterpret_cast<void*>(0x1234);
  ASSERT_EQ(vfs_mount("/", &fs_interface, fs_data), 0);
  ASSERT_EQ(vfs_instance->mounts[0].active, true);
  ASSERT_STREQ(vfs_instance->mounts[0].path, "/");
  ASSERT_EQ(vfs_instance->mounts[0].fs, &fs_interface);
  ASSERT_EQ(vfs_instance->mounts[0].fs_data, fs_data);
}

TEST_F(VfsTest, RootMountTwiceFailure) {
  VFSInterface fs_interface = {};
  void* fs_data = reinterpret_cast<void*>(0x1234);
  ASSERT_EQ(vfs_mount("/", &fs_interface, fs_data), 0);
  ASSERT_EQ(vfs_mount("/", &fs_interface, fs_data), -1);
}


TEST_F(VfsTest, MountFailureOnInvalidPath) {
  VFSInterface fs_interface = {};
  void* fs_data = reinterpret_cast<void*>(0x1234);
  ASSERT_EQ(vfs_mount("invalid_path", &fs_interface, fs_data), -1);
}

TEST_F(VfsTest, MountFailureOnAlreadyMountedPath) {
  VFSInterface fs_interface1 = {};
  void* fs_data1 = reinterpret_cast<void*>(0x1234);
  ASSERT_EQ(vfs_mount("/mnt", &fs_interface1, fs_data1), 0);

  VFSInterface fs_interface2 = {};
  void* fs_data2 = reinterpret_cast<void*>(0x5678);
  ASSERT_EQ(vfs_mount("/mnt", &fs_interface2, fs_data2), -1);
}

TEST_F(VfsTest, MountFailureOnMaxMounts) {
  VFSInterface fs_interface = {};
  void* fs_data = reinterpret_cast<void*>(0x1234);
  for (int i = 0; i < MAX_MOUNTS; ++i) {
    char path[16];
    snprintf(path, sizeof(path), "/mnt%d", i);
    ASSERT_EQ(vfs_mount(path, &fs_interface, fs_data), 0);
  }
  ASSERT_EQ(vfs_mount("/extra", &fs_interface, fs_data), -1);
}

TEST_F(VfsTest, OpenFailureOnInvalidPath) {
  VFSFileDescriptor* fd = vfs_open("invalid_path", 0, 0);
  ASSERT_EQ(fd, nullptr);
}

TEST_F(VfsTest, OpenFailureOnNoMount) {
  VFSFileDescriptor* fd = vfs_open("/file.txt", 0, 0);
  ASSERT_EQ(fd, nullptr);
}

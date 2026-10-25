#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <sys/types.h>

/* File type bits of st_mode. */
#define S_IFMT 0170000  /* Mask for the file type. */
#define S_IFCHR 0020000 /* Character device. */
#define S_IFDIR 0040000 /* Directory. */
#define S_IFREG 0100000 /* Regular file. */

#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)

struct stat {
  mode_t st_mode;
  dev_t st_rdev;
  size_t st_size;
};

#endif // SYS_STAT_H

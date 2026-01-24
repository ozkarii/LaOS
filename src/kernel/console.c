#include "string.h"
#include "stdio.h"
#include "console.h"
#include "io.h"
#include "armv8-a.h"
#include "gic.h"
#include "pl011.h"
#include "vfs.h"
#include "memory.h"

#define WELCOME "Welcome to LaOS"
#define LINE_MAX 256

#define GNOME_BACKSPACE 0x7f
#define KEY_UP 0x41
#define KEY_DOWN 0x42
#define KEY_RIGHT 0x43
#define KEY_LEFT 0x44

void startup_logs() {
  k_printf("Using CPU%u\r\n", GET_CPU_ID());
  k_printf("CurrentEL: EL%u\r\n", GET_CURRENT_EL() >> 2u);
  gic_print_info(k_printf);
  pl011_print_info(k_printf);
}

typedef struct StringTokens {
  char** tokens;
  size_t count;
} StringTokens;

static StringTokens tokenize_string(const char* str, const char delim) {
  StringTokens result = {NULL, 0};
  
  // Count tokens first
  size_t count = 0;
  const char* p = str;
  while (*p) {
    while (*p == delim) p++;
    if (*p) {
      count++;
      while (*p && *p != delim) p++;
    }
  }

  result.count = count;
  
  // Allocate array with NULL terminator
  result.tokens = k_malloc((count + 1) * sizeof(char*));
  result.tokens[count] = NULL;

  // Fill tokens
  size_t idx = 0;
  const char* start = str;
  while (*str) {
    while (*str == delim) str++;
    if (*str) {
      start = str;
      while (*str && *str != delim) str++;
      size_t len = str - start;
      result.tokens[idx] = k_malloc(len + 1);
      memcpy(result.tokens[idx], start, len);
      result.tokens[idx][len] = '\0';
      idx++;
    }
  }

  return result;
}

void command_ls(char** argv, size_t argc) {
  static char buf[256];
  if (argc < 2) {
    k_printf("Usage: ls <path>\n");
    return;
  }
  VFSFileDescriptor* fd = vfs_open(argv[1], MODE_READ);
  if (fd == NULL) {
    k_printf("vfs_open: failed to open path %s\n", argv[1]);
    return;
  }
  vfs_readdir(fd, buf, sizeof(buf));
  vfs_close(fd);
  k_printf("%s\n", buf);
}

void command_touch(char** argv, size_t argc) {
  if (argc < 2) {
    k_printf("Usage: touch <filename>\n");
    return;
  }
  VFSFileDescriptor* fd = vfs_open(argv[1], MODE_CREATE);
  if (fd == NULL) {
    k_printf("vfs_open: failed to create file %s\n", argv[1]);
    return;
  }
  vfs_close(fd);
}

void command_mkdir(char** argv, size_t argc) {
  if (argc < 2) {
    k_printf("Usage: mkdir <dirname>\n");
    return;
  }
  int ret = vfs_mkdir(argv[1]);
  if (ret != 0) {
    k_printf("vfs_mkdir: failed to create directory %s\n", argv[1]);
  }
}


static void exec_command(const char* command) {
  StringTokens s = tokenize_string(command, ' ');
  if (!strcmp(s.tokens[0], "ls")) {
    command_ls(s.tokens, s.count);
  }
  else if (!strcmp(s.tokens[0], "touch")) {
    command_touch(s.tokens, s.count);
  }
  else if (!strcmp(s.tokens[0], "mkdir")) {
    command_mkdir(s.tokens, s.count);
  }
  else {
    // Nothing
  }
}

// Escape sequence handler, can modify line ptr position when arrow keys pressed
static void escape_handler(const char* line, char** line_ptr) {
  char first = k_getchar();
  if (first == '[') {
    char second = k_getchar();
    switch (second) {
    case KEY_UP:
      break;
    case KEY_DOWN:
      break;
    case KEY_RIGHT:
      k_putchar('\e');
      k_putchar(first);
      k_putchar(second);
      *line_ptr -= 1;
      break;
    case KEY_LEFT:
      if (*line_ptr != line) {
        k_putchar('\e');
        k_putchar(first);
        k_putchar(second);
        *line_ptr -= 1;
      }
      break;
    }
  }
}

static inline bool is_backspace(char c) {
  return (c == '\b' || c == GNOME_BACKSPACE);
}

void console_loop(const char* prompt) {
  k_printf("%s\r\n", WELCOME);
  char line[LINE_MAX];
  
  while (1) {
    memset(line, 0, LINE_MAX);
    k_puts(prompt);
    k_putchar(' ');

    char* line_ptr = line;
    *line_ptr = '\0';

    while (1) {
      char tmp = k_getchar();
      if (tmp == (char)EOF) {
        continue;
      }

      // Execute current line and break if enter
      if (tmp == '\r') {
        k_puts("\r\n");
        exec_command(line);
        break;
      }

      // Handle escape seq
      if (tmp == '\e') {
        escape_handler(line, &line_ptr);
        continue;
      }

      // Handle backspace
      if (is_backspace(tmp)) {
        // Do nothing if at the start of the line
        if (line_ptr > line) {
          // Move cursor back, overwrite with space, move back again
          k_putchar('\b');
          k_putchar(' ');
          k_putchar('\b');
          line_ptr--;
          *line_ptr = '\0';
        }
        continue;
      }

      // Append new char to line once special characters are handled
      *line_ptr = tmp;

      // If last char in line, break
      if (line_ptr == &line[LINE_MAX - 1]) {
        k_printf("\r\nLine reached max length (%d), try again\r\n", LINE_MAX);
        break;
      }
      else {
        k_putchar(*line_ptr);
        line_ptr++;
      }

      // Make sure line_ptr always points to null terminator
      *line_ptr = '\0';

    }
  }

}

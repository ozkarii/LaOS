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


static char cwd[NAME_MAX] = "/";


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

static void free_string_tokens(StringTokens* s) {
  for (size_t i = 0; i < s->count; i++) {
    k_free(s->tokens[i]);
  }
  k_free(s->tokens);
  s->tokens = NULL;
  s->count = 0;
}

// Make sure `out` is at least NAME_MAX in size
static void resolve_relative_path(const char* cwd, const char* path, char* out) {
  if (path[0] != '/') {
    if (!strcmp(cwd, "/")) {
      snprintf(out, NAME_MAX, "/%s", path);
    }
    else {
      snprintf(out, NAME_MAX, "%s/%s", cwd, path);
    }
  }
  else {
    strcpy(out, path);
  }
}

void command_ls(char** argv, size_t argc) {
  static char readdir_buf[256];
  static char temp[NAME_MAX] = {0};

  if (argc == 2) {
    resolve_relative_path(cwd, argv[1], temp);
  } else if (argc == 1) {
    strcpy(temp, cwd);
  } else {
    k_printf("Usage: ls [ <dirname> ]\n");
    return;
  }

  VFSFileDescriptor* fd = vfs_open(temp, MODE_READ);
  if (fd == NULL) {
    k_printf("vfs_open: failed to open path %s\n", temp);
    return;
  }
  if (vfs_readdir(fd, readdir_buf, sizeof(readdir_buf)) != 0) {
    k_printf("vfs_readdir: failed to read directory %s\n", temp);
    vfs_close(fd);
    return;
  }
  vfs_close(fd);
  k_printf("%s\n", readdir_buf);
}

void command_touch(char** argv, size_t argc) {
  if (argc < 2) {
    k_printf("Usage: touch <filename>\n");
    return;
  }

  static char temp[NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], temp);

  VFSFileDescriptor* fd = vfs_open(temp, MODE_CREATE);
  if (fd == NULL) {
    k_printf("vfs_open: failed to create file %s\n", temp);
    return;
  }
  vfs_close(fd);
}

void command_mkdir(char** argv, size_t argc) {
  if (argc < 2) {
    k_printf("Usage: mkdir <dirname>\n");
    return;
  }

  static char temp[NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], temp);

  int ret = vfs_mkdir(temp);
  if (ret != 0) {
    k_printf("vfs_mkdir: failed to create directory %s\n", temp);
  }
}

void command_rm(char** argv, size_t argc) {
  if (argc < 2) {
    k_printf("Usage: rm [ <filename> | <emptydirname> ]\n");
    return;
  }

  static char temp[NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], temp);

  int ret = vfs_remove(temp);
  if (ret != 0) {
    k_printf("vfs_remove: failed to remove %s\n", temp);
  }
}

void command_cd(char** argv, size_t argc) {
  if (argc < 2) {
    k_printf("Usage: cd <dirname>\n");
    return;
  }
  
  static char temp[NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], temp);

  VFSStat stat = {0};
  int ret = vfs_stat(temp, &stat);
  if (ret != 0) {
    k_printf("vfs_stat: failed to stat %s\n", temp);
    return;
  }
  if (stat.is_directory) {
    strcpy(cwd, temp);
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
  else if (!strcmp(s.tokens[0], "rm")){
    command_rm(s.tokens, s.count);
  }
  else if (!strcmp(s.tokens[0], "cd")) {
    command_cd(s.tokens, s.count);
  }

  free_string_tokens(&s);
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

    k_printf("%s %s ", cwd, prompt);

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

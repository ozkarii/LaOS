#include <stdarg.h>

#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "fcntl.h"

#define WELCOME "Welcome to LaOS userspace shell"
#define LINE_MAX 256
#define PATH_NAME_MAX 64
#define MAX_TOKENS 16

int printf(const char* format, ...);

typedef struct StringTokens {
  char* tokens[MAX_TOKENS + 1];
  size_t count;
} StringTokens;

static char cwd[PATH_NAME_MAX] = "/";

static void tokenize_string(char* str, char delim, StringTokens* out) {
  out->count = 0;

  while (*str != '\0' && out->count < MAX_TOKENS) {
    while (*str == delim) {
      str++;
    }

    if (*str == '\0') {
      break;
    }

    out->tokens[out->count++] = str;

    while (*str != '\0' && *str != delim) {
      str++;
    }

    if (*str == '\0') {
      break;
    }

    *str = '\0';
    str++;
  }

  out->tokens[out->count] = NULL;
}

static void resolve_relative_path(const char* current_dir, const char* path, char* out) {
  if (path[0] == '/') {
    snprintf(out, PATH_NAME_MAX, "%s", path);
    return;
  }

  if (strcmp(current_dir, "/") == 0) {
    snprintf(out, PATH_NAME_MAX, "/%s", path);
  } else {
    snprintf(out, PATH_NAME_MAX, "%s/%s", current_dir, path);
  }
}

static void command_ls(char** argv, size_t argc) {
  (void)argv;
  (void)argc;
  printf("ls: not implemented\n");
}

static void command_touch(char** argv, size_t argc) {
  if (argc < 2) {
    printf("Usage: touch <filename>\n");
    return;
  }

  char abs_path[PATH_NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], abs_path);

  int fd = open(abs_path, O_CREAT | O_RDWR, 0);
  if (fd < 0) {
    printf("touch: failed to create file %s\n", abs_path);
    return;
  }

  close(fd);
}

static void command_mkdir(char** argv, size_t argc) {
  if (argc < 2) {
    printf("Usage: mkdir <dirname>\n");
    return;
  }

  char abs_path[PATH_NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], abs_path);

  int res = mkdir(abs_path, 0);
  if (res != 0) {
    printf("mkdir: failed to create directory %s\n", abs_path);
    return;
  }
}

static void command_rm(char** argv, size_t argc) {
  (void)argv;
  (void)argc;
  printf("rm: not implemented\n");
}

static void command_cd(char** argv, size_t argc) {
  if (argc < 2) {
    printf("Usage: cd <dirname>\n");
    return;
  }

  int res = chdir(argv[1]);
  if (res != 0) {
    printf("cd: failed to change directory to %s\n", argv[1]);
    return;
  }
}

static void command_cat(char** argv, size_t argc) {
  if (argc < 2) {
    printf("Usage: cat <filename>\n");
    return;
  }

  char abs_path[PATH_NAME_MAX] = {0};
  resolve_relative_path(cwd, argv[1], abs_path);

  int fd = open(abs_path, O_RDONLY, 0);
  if (fd < 0) {
    printf("cat: failed to open path %s\n", abs_path);
    return;
  }

  char read_buf[128];
  ssize_t bytes_read = 0;
  while ((bytes_read = read(fd, read_buf, sizeof(read_buf) - 1)) > 0) {
    read_buf[bytes_read] = '\0';
    printf("%s", read_buf);
  }

  if (bytes_read < 0) {
    printf("cat: failed to read file %s\n", abs_path);
  }

  close(fd);
  printf("\n");
}

static void exec_command(char* command) {
  StringTokens tokens;
  tokenize_string(command, ' ', &tokens);

  if (tokens.count == 0 || strlen(tokens.tokens[0]) == 0) {
    return;
  }

  if (strcmp(tokens.tokens[0], "ls") == 0) {
    command_ls(tokens.tokens, tokens.count);
  } else if (strcmp(tokens.tokens[0], "touch") == 0) {
    command_touch(tokens.tokens, tokens.count);
  } else if (strcmp(tokens.tokens[0], "mkdir") == 0) {
    command_mkdir(tokens.tokens, tokens.count);
  } else if (strcmp(tokens.tokens[0], "rm") == 0) {
    command_rm(tokens.tokens, tokens.count);
  } else if (strcmp(tokens.tokens[0], "cd") == 0) {
    command_cd(tokens.tokens, tokens.count);
  } else if (strcmp(tokens.tokens[0], "cat") == 0) {
    command_cat(tokens.tokens, tokens.count);
  } else {
    printf("Unknown command: %s\n", tokens.tokens[0]);
  }
}

static const char* next_stubbed_input(void) {
  static const char* scripted_commands[] = {
    "ls",
    "touch /tmp.txt",
    "cat /tmp.txt",
    "mkdir /tmpdir",
    "cd /tmpdir",
    "rm /tmp.txt",
    "unknown",
    NULL
  };
  static size_t index = 0;

  const char* cmd = scripted_commands[index];
  if (cmd != NULL) {
    index++;
  }
  return cmd;
}

int printf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  int len = vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  if (len > 0) {
    len = write(1, buffer, (size_t)len);
  }

  return len;
}

int main() {
  printf("%s\n", WELCOME);

  while (1) {
    const char* input = next_stubbed_input();
    if (input == NULL) {
      break;
    }

    char line[LINE_MAX] = {0};
    snprintf(line, sizeof(line), "%s", input);

    printf("%s # %s\n", cwd, line);
    exec_command(line);
  }

  return 0;
}
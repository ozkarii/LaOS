#include "string.h"
#include "console.h"
#include "io.h"
#include "armv8-a.h"
#include "gic.h"
#include "pl011.h"

#define WELCOME "Welcome to Laudes OS"
#define LINE_MAX 256

#define GNOME_BACKSPACE 0x7f
#define KEY_UP 0x41
#define KEY_DOWN 0x42
#define KEY_RIGHT 0x43
#define KEY_LEFT 0x44

void startup_logs() {
  k_printf("Using CPU%u\r\n", GET_MPIDR() & 0xFF);
  k_printf("CurrentEL: EL%u\r\n", GET_CURRENT_EL() >> 2u);
  gic_print_info(k_printf);
  pl011_print_info(k_printf);
}

static void exec_command(const char* command) {
  if (!strcmp(command, "info uart")) {
    pl011_print_info(k_printf);
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
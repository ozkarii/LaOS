#include "string.h"
#include "console.h"
#include "io.h"

#define WELCOME "Welcome to Laudes OS"
#define GNOME_BACKSPACE 0x7f

// Escape sequence handler, can modify line ptr position when arrow keys pressed
static void escape_handler(const char* line, char** line_ptr) {
  char first = k_getchar();
  if (first == '[') {
    /*     41
     * 44  42  43
     */
    char second = k_getchar();
    switch (second) {
    case 0x41:
      break;
    case 0x42:
      break;
    case 0x43:
      k_putchar('\e');
      k_putchar(first);
      k_putchar(second);
      *line_ptr -= 1;
      break;
    case 0x44:
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

      // Break if enter
      if (tmp == '\r') {
        break;
      }

      // Handle escape seq
      if (tmp == '\e') {
        escape_handler(line, &line_ptr);
        continue;
      }

      // Handle backspace
      if (tmp == GNOME_BACKSPACE || tmp == '\b') {
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
        k_printf("\r\nLine reached max length (%d), try again", LINE_MAX);
        break;
      }
      else {
        k_putchar(*line_ptr);
        line_ptr++;
      }

      // Make sure line_ptr always points to null terminator
      *line_ptr = '\0';

    }

    k_puts("\r\n");
  }

}
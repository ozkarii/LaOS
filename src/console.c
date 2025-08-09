#include "console.h"
#include "io.h"

#define WELCOME "Welcome to Laudes OS"
#define KB_BACKSPACE 0x7f

static void escape_handler(const char* line, char* line_ptr) {
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
      line_ptr++;
      break;
    case 0x44:
      if (line_ptr != line) {
        k_putchar('\e');
        k_putchar(first);
        k_putchar(second);
        line_ptr--;
      }
      break;
    }
  }
}

void console_loop(const char* prompt) {
  k_printf("%s\r\n", WELCOME);
  while (1)
  {
    k_puts(prompt);
    k_putchar(' ');

    char line[LINE_MAX];
    char* line_ptr = line;

    while (((*line_ptr = k_getchar()) != '\r')) {
      if (line_ptr == &line[LINE_MAX - 1]) {
        k_printf("\r\nLine reached max length (%d), try again", LINE_MAX);
        break;
      }
      if (*line_ptr == '\e') {
        escape_handler(line, line_ptr);
        continue;
      }

      if (*line_ptr == KB_BACKSPACE && line_ptr > line) {
        // Move cursor back, overwrite with space, move back again
        k_putchar('\b');
        k_putchar(' ');
        k_putchar('\b');
        line_ptr--;
      }
      else {
        k_putchar(*line_ptr);
        line_ptr++;
      }
      
    }

    k_puts("\r\n");
    line_ptr = line;
  }

}
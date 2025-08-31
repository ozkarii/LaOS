#include "string.h"
#include "console.h"
#include "io.h"
#include "armv8-a.h"
#include "gic.h"
#include "pl011.h"

#define WELCOME "Welcome to Laudes OS"
#define GNOME_BACKSPACE 0x7f

void startup_logs() {
  k_printf("Using CPU%u\r\n", GET_MPIDR() & 0xFF);
  k_printf("CurrentEL: EL%u\r\n", GET_CURRENT_EL() >> 2u);
  k_printf("VBAR_EL3: 0x%lx\r\n", GET_VBAR_EL3());
  uint32_t spsel = GET_SPSEL();
  k_printf("SPSel: %d (%s)\n", spsel & 1, (spsel & 1) ? "SP_ELx" : "SP_EL0");
  k_printf("Timer Frequency: %u MHz\r\n", GET_TIMER_FREQ() / 1000000);
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
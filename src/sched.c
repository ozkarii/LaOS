#include "string.h"
#include "armv8-a.h"
#include "sched.h"


static void init_timer(void) {
  SET_PHYS_TIMER_VALUE(0xFFFFFFFFFFFFFFFF);
  ENABLE_PHYS_TIMER();
}

bool sched_init(void) {
  memset(&sched_ctx, 0, sizeof(SchedContext));
  init_timer();
  return true;
}

void sched_timer_irq_handler(void) {
  
}


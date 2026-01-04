#ifndef SP804_H
#define SP804_H


#include <stdint.h>

#define LOAD     0x400
#define VALUE    0x404

#define CONTROL         0x408
#define CONTROL_ENABLE  (1u << 7u)
#define CONTROL_ENAFREE (1u << 9u)

#define IRQCNTL  0x40c
#define RAWIRQ   0x410
#define MSKIRQ   0x414
#define RELOAD   0x418
#define PREDIV   0x41c
#define FREECN   0x420

void sp804_enable(void);
void sp804_enable_free_counter(void);
uint32_t sp804_get_free_count(void);

#endif // SP804_H
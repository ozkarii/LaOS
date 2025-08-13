#ifndef GIC_H
#define GIC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define GIC_VERSION 2
#define GIC_MODEL "GIC-400"

#define D_CTLR              0x0
#define D_CTLR_ENABLE_GRP0  1u
#define D_CTLR_ENABLE_GRP1  (1u << 1)

#define D_TYPER                  0x4
#define D_TYPER_IT_LINES_NUMBER  0x1F  // [4:0]
#define D_TYPER_CPU_NUMBER       0xE0  // [7:5]
#define D_TYPER_SECURITY_EXTN    (1u << 10)

#define D_ISENABLER  0x100
#define D_ISPENDR    0x200
#define D_ISACTIVER  0x300
#define D_IPRIORITYR 0x400
#define D_ITARGETSR  0x800

#define C_CTLR              0x0
#define C_CTLR_ENABLE_GRP0  1u
#define C_CTLR_ENABLE_GRP1  (1u << 1)

typedef int (*log_func_t)(const char *format, ...);

void gicd_enable(void);
void gicd_disable(void);

int gicd_enable_and_activate_irq(uint32_t irq);
int gicd_set_irq_priority(uint32_t irq, uint8_t priority);
int gicd_set_irq_cpu(uint32_t irq, uint32_t cpu);
int gicd_is_irq_pending(uint32_t irq);

void gicc_enable(void);
void gicc_disable(void);

void gic_print_info(log_func_t log);

#endif // GIC_H
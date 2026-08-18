#include <stdint.h>
#include "armv8-a.h"
#include "platform.h"
#include "gic.h"

static inline void gicd_write_reg8(uint64_t offset, uint8_t val) {
  *(volatile uint8_t*)(GICD_BASE + offset) = val;
}

static inline uint8_t gicd_read_reg8(uint64_t offset) {
  return *(volatile uint8_t*)(GICD_BASE + offset);
}

static inline void gicd_write_reg32(uint64_t offset, uint32_t val) {
  *(volatile uint32_t*)(GICD_BASE + offset) = val;
}

static inline uint32_t gicd_read_reg32(uint64_t offset) {
  return *(volatile uint32_t*)(GICD_BASE + offset);
}

static inline uint64_t gicc_get_base_for_cpu(uint32_t cpu_id) {
  switch (cpu_id) {
    case 0:
      return GICC_BASE_CPU0;
    case 1:
      return GICC_BASE_CPU1;
    case 2:
      return GICC_BASE_CPU2;
    case 3:
      return GICC_BASE_CPU3;
    default:
      return GICC_BASE_LEGACY;
  }
}

static inline void gicc_write_reg32(uint64_t offset, uint32_t val, uint32_t cpu_id) {
  *(volatile uint32_t*)(gicc_get_base_for_cpu(cpu_id) + offset) = val;
}

static inline uint32_t gicc_read_reg32(uint64_t offset, uint32_t cpu_id) {
  return *(volatile uint32_t*)(gicc_get_base_for_cpu(cpu_id) + offset);
}


void gicd_enable(void) {
  gicd_write_reg32(D_CTLR, gicd_read_reg32(D_CTLR) | D_CTLR_ENABLE_GRP0 | D_CTLR_ENABLE_GRP1);
}

void gicd_disable(void) {
  gicd_write_reg32(D_CTLR, gicd_read_reg32(D_CTLR) & ~(D_CTLR_ENABLE_GRP0 | D_CTLR_ENABLE_GRP0));
}

uint64_t gicd_get_num_irqs(void) {
  return (32 * (gicd_read_reg32(D_TYPER) & D_TYPER_IT_LINES_NUMBER) + 1);
}

bool gicd_has_security_extn(void) {
  return (gicd_read_reg32(D_TYPER) & D_TYPER_SECURITY_EXTN) != 0;
}

static uint8_t gicd_get_num_priorities(void) {
  uint8_t original = gicd_read_reg8(D_IPRIORITYR);
  gicd_write_reg8(D_IPRIORITYR, 0xFF);
  uint8_t result = gicd_read_reg8(D_IPRIORITYR);
  gicd_write_reg8(D_IPRIORITYR, original);
  return result;
}

int gicd_enable_irq(uint32_t irq) {
  if (irq >= gicd_get_num_irqs()) {
    return -1; // Invalid IRQ number
  }
  uint32_t reg_offset = (irq / 32) * 4;
  uint64_t bit = 1u << (irq % 32);
  gicd_write_reg32(D_ISENABLER + reg_offset, bit);
  return 0; // Success
}

int gicd_activate_irq(uint32_t irq) {
    if (irq >= gicd_get_num_irqs()) {
    return -1; // Invalid IRQ number
  }
  uint32_t reg_offset = (irq / 32) * 4;
  uint64_t bit = 1u << (irq % 32);
  gicd_write_reg32(D_ISACTIVER + reg_offset, bit);
  return 0; // Success
}

int gicd_set_irq_priority(uint32_t irq, uint8_t priority) {
  if (irq >= gicd_get_num_irqs() ||
      priority > gicd_get_num_priorities()) {
    return -1; // Invalid IRQ number
  }
  uint32_t reg_offset = (irq / 4) * 4;
  uint32_t byte_offset = irq % 4;
  gicd_write_reg8(D_IPRIORITYR + reg_offset + byte_offset, priority);
  return 0;
}

int gicd_set_irq_cpu(uint32_t irq, uint32_t cpu) {
  if (irq >= gicd_get_num_irqs() || cpu > (GIC_MAX_CPUS - 1) || irq < SPI_START) {
    return -1; // Invalid IRQ number
  }
  uint32_t reg_offset = (irq / 4) * 4;
  uint32_t byte_offset = irq % 4;
  gicd_write_reg8(D_ITARGETSR + reg_offset + byte_offset, 1u << cpu);
  return 0;
}

int gicd_send_sgi(uint32_t sgi, uint32_t cpu_bits) {
  if (sgi >= PPI_START || cpu_bits > ((1u << GIC_MAX_CPUS) - 1)) {
    return -1;
  }
  uint32_t sgi_value = (cpu_bits << D_SGIR_CPU_TARGET_LIST_SHIFT) | (sgi & D_SGIR_INTID);
  gicd_write_reg32(D_SGIR, sgi_value);
  return 0;
}

int gicd_is_irq_pending(uint32_t irq) {
  if (irq >= gicd_get_num_irqs()) {
    return -1; // Invalid IRQ number
  }
  uint32_t reg_offset = (irq / 32) * 4;
  uint32_t bit = 1u << (irq % 32);
  return (gicd_read_reg32(D_ISPENDR + reg_offset) & bit) ? 1 : 0;
}

void gic_print_info(log_func_t log) {
  log("ARM Generic Interrupt Controller (GIC)\r\n");
  log("  Version: v%u\r\n", GIC_VERSION);
  log("  Model: %s\r\n", GIC_MODEL);
  log("GIC Distributor at 0x%x:\r\n", GICD_BASE);
  log("  Number of IRQs: %lu\r\n", gicd_get_num_irqs());
  log("  Security Extension: %s\r\n", gicd_has_security_extn() ? "Yes" : "No");
  log("  Number of Priorities: %u\r\n", gicd_get_num_priorities());
  log("  Distributor Enabled: %s\r\n",
     (gicd_read_reg32(D_CTLR) & (D_CTLR_ENABLE_GRP0 | D_CTLR_ENABLE_GRP1)) ? "Yes" : "No");
  log("GIC CPU Legacy Interface at 0x%x:\r\n", GICC_BASE_LEGACY);
  log("  Enabled: %s\r\n",
     (gicc_read_reg32(C_CTLR, -1) & (C_CTLR_ENABLE_GRP0 | C_CTLR_ENABLE_GRP1)) ? "Yes" : "No");
  log("GIC CPU Interfaces per CPU:\r\n");
  for (uint32_t cpu = 0; cpu < NUM_CPUS; cpu++) {
    log("  CPU%u Interface at 0x%x:\r\n", cpu, gicc_get_base_for_cpu(cpu));
    log("    Enabled: %s\r\n", (gicc_read_reg32(C_CTLR, cpu) & (C_CTLR_ENABLE_GRP0 | C_CTLR_ENABLE_GRP1)) ? "Yes" : "No");
  }
}

void gicc_enable(uint32_t cpu_id) {
  gicc_write_reg32(C_CTLR, gicc_read_reg32(C_CTLR, cpu_id) | C_CTLR_ENABLE_GRP0 | C_CTLR_ENABLE_GRP1, cpu_id);
}

void gicc_disable(uint32_t cpu_id) {
  gicc_write_reg32(C_CTLR, gicc_read_reg32(C_CTLR, cpu_id) & ~(C_CTLR_ENABLE_GRP0 | C_CTLR_ENABLE_GRP0), cpu_id);
}

void gicc_set_priority_mask(uint8_t value, uint32_t cpu_id) {
  gicc_write_reg32(C_PMR, value, cpu_id);
}

uint32_t gicc_get_intid_and_ack(uint32_t cpu_id) {
  return gicc_read_reg32(C_IAR, cpu_id);
}

void gicc_end_irq(uint32_t int_id, uint32_t cpu_id) {
  gicc_write_reg32(C_EOIR, int_id, cpu_id);
}
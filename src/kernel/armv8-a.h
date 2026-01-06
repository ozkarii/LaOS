#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#define USE_SMP 1
#define NUM_CPUS 4

/* REGISTER GETTER GENERATORS */

#define MAKE_MRS_GETTER_32(name, reg) \
    static inline uint32_t name(void) { \
        uint32_t val; \
        asm volatile ("mrs %0, " #reg : "=r"(val)); \
        return val; \
    }

#define MAKE_MRS_GETTER_64(name, reg) \
    static inline uint64_t name(void) { \
        uint64_t val; \
        asm volatile ("mrs %0, " #reg : "=r"(val)); \
        return val; \
    }

#define MAKE_MOV_GETTER_64(name, reg) \
    static inline uint64_t name(void) { \
        uint64_t val; \
        asm volatile ("mov %0, " #reg : "=r"(val)); \
        return val; \
    }

/* REGISTERS */

MAKE_MRS_GETTER_32(GET_MPIDR, mpidr_el1)
MAKE_MRS_GETTER_32(GET_CURRENT_EL, CurrentEL)
MAKE_MRS_GETTER_64(GET_VBAR_EL3, vbar_el3)
MAKE_MRS_GETTER_64(GET_VBAR_EL1, vbar_el1)
MAKE_MRS_GETTER_64(GET_SCR_EL3, scr_el3)
MAKE_MRS_GETTER_64(GET_CPTR_EL3, cptr_el3)
MAKE_MRS_GETTER_64(GET_SCTLR_EL3, sctlr_el3)
MAKE_MRS_GETTER_32(GET_SPSR_EL3, spsr_el3)
MAKE_MRS_GETTER_64(GET_ELR_EL3, elr_el3)
MAKE_MRS_GETTER_32(GET_ESR_EL3, esr_el3)
MAKE_MRS_GETTER_64(GET_FAR_EL3, far_el3)
MAKE_MRS_GETTER_32(GET_SPSR_EL1, spsr_el1)
MAKE_MRS_GETTER_64(GET_SP_EL1, sp_el1)
MAKE_MRS_GETTER_64(GET_MAIR_EL1, mair_el1)
MAKE_MRS_GETTER_64(GET_MAIR_EL3, mair_el3)
MAKE_MRS_GETTER_64(GET_ACTLR_EL3, actlr_el3)
MAKE_MRS_GETTER_64(GET_ACTLR_EL1, actlr_el1)
MAKE_MRS_GETTER_64(GET_SCTLR_EL1, sctlr_el1)
MAKE_MRS_GETTER_64(GET_ELR_EL1, elr_el1)
MAKE_MRS_GETTER_64(GET_SP_EL0, sp_el0)
MAKE_MRS_GETTER_64(GET_TCR_EL1, tcr_el1)
MAKE_MRS_GETTER_64(GET_TTBR0_EL1, ttbr0_el1)
MAKE_MRS_GETTER_64(GET_TTBR1_EL1, ttbr1_el1)
MAKE_MRS_GETTER_32(GET_ESR_EL1, esr_el1)
MAKE_MRS_GETTER_64(GET_FAR_EL1, far_el1)
MAKE_MRS_GETTER_32(GET_DAIF, daif)
MAKE_MRS_GETTER_32(GET_NZCV, nzcv)
MAKE_MRS_GETTER_32(GET_SPSEL, spsel)

/* GPRs */
MAKE_MOV_GETTER_64(GET_SP, sp)
MAKE_MOV_GETTER_64(GET_FP, x29)
MAKE_MOV_GETTER_64(GET_LR, x30)
MAKE_MOV_GETTER_64(GET_X0, x0)
MAKE_MOV_GETTER_64(GET_X1, x1)
MAKE_MOV_GETTER_64(GET_X2, x2)
MAKE_MOV_GETTER_64(GET_X3, x3)
MAKE_MOV_GETTER_64(GET_X4, x4)
MAKE_MOV_GETTER_64(GET_X5, x5)
MAKE_MOV_GETTER_64(GET_X6, x6)
MAKE_MOV_GETTER_64(GET_X7, x7)

/* TIMERS */

MAKE_MRS_GETTER_32(GET_TIMER_FREQ, cntfrq_el0)
MAKE_MRS_GETTER_64(GET_TIMER_COUNT, cntpct_el0)

#define SET_PHYS_TIMER_VALUE(tval) \
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(tval))

#define ENABLE_PHYS_TIMER() \
    asm volatile ("msr cntp_ctl_el0, %0" : : "r"(1))

#define DISABLE_PHYS_TIMER() \
    asm volatile ("msr cntp_ctl_el0, %0" : : "r"(0))

#define GET_PHYS_TIMER_VALUE() ({ \
    uint64_t val; \
    asm volatile ("mrs %0, cntp_tval_el0" : "=r"(val)); \
    val; \
})

/* CPU */

#define GET_CPU_ID() (GET_MPIDR() & 0xFF)

/* INTERRUPTS */

#define EL1_PHY_TIM_IRQ 30u

#define ENABLE_IRQ() \
    asm volatile ("msr daifclr, #2" ::: "memory")

#define DISABLE_IRQ() \
    asm volatile ("msr daifset, #2" ::: "memory")

#define ENABLE_FIQ() \
    asm volatile ("msr daifclr, #1" ::: "memory")

#define DISABLE_FIQ() \
    asm volatile ("msr daifset, #1" ::: "memory")

#define ENABLE_ALL_INTERRUPTS() \
    asm volatile ("msr daifclr, #3" ::: "memory")

#define DISABLE_ALL_INTERRUPTS() \
    asm volatile ("msr daifset, #3" ::: "memory")

#define WAIT_FOR_INTERRUPT() \
    asm volatile ("wfi")


/* REGISTER DUMP */

static inline void cpu_dump_registers(void (*printf_func)(const char* format, ...)) {
    printf_func("\n========= Register Dump Start =========\n");
    
    printf_func("\n--- General Registers ---\n");
    printf_func("SP (x31):         0x%lx\n", GET_SP());
    printf_func("FP (x29):         0x%lx\n", GET_FP());
    printf_func("LR (x30):         0x%lx\n", GET_LR());
    printf_func("X0:               0x%lx\n", GET_X0());
    printf_func("X1:               0x%lx\n", GET_X1());
    printf_func("X2:               0x%lx\n", GET_X2());
    printf_func("X3:               0x%lx\n", GET_X3());
    printf_func("X4:               0x%lx\n", GET_X4());
    printf_func("X5:               0x%lx\n", GET_X5());
    printf_func("X6:               0x%lx\n", GET_X6());
    printf_func("X7:               0x%lx\n", GET_X7());
    printf_func("DAIF:             0x%x\n", GET_DAIF());
    printf_func("NZCV:             0x%x\n", GET_NZCV());
    printf_func("CurrentEL:        0x%x (EL%u)\n", GET_CURRENT_EL(), GET_CURRENT_EL() >> 2);

    if ((GET_CURRENT_EL() >> 2) < 1) {
        printf_func("\n========= Register Dump End =========\n");
        return;
    }

    printf_func("\n--- EL1 Registers ---\n");
    printf_func("ESR_EL1:          0x%x (decode: https://esr.arm64.dev/#0x%x)\n",
                GET_ESR_EL1(), GET_ESR_EL1());
                printf_func("ELR_EL1:          0x%lx\n", GET_ELR_EL1());
    printf_func("FAR_EL1:          0x%lx\n", GET_FAR_EL1());
    printf_func("SPSR_EL1:         0x%x\n", GET_SPSR_EL1());
    printf_func("MPIDR_EL1:        0x%x\n", GET_MPIDR());
    printf_func("SCTLR_EL1:        0x%lx\n", GET_SCTLR_EL1());
    printf_func("VBAR_EL1:         0x%lx\n", GET_VBAR_EL1());
    printf_func("MAIR_EL1:         0x%lx\n", GET_MAIR_EL1());
    printf_func("ACTLR_EL1:        0x%lx\n", GET_ACTLR_EL1());
    printf_func("TCR_EL1:          0x%lx\n", GET_TCR_EL1());
    printf_func("TTBR0_EL1:        0x%lx\n", GET_TTBR0_EL1());
    printf_func("TTBR1_EL1:        0x%lx\n", GET_TTBR1_EL1());

    if ((GET_CURRENT_EL() >> 2) < 3) {
        printf_func("\n========= Register Dump End =========\n");
        return;
    }

    printf_func("\n--- SP Registers ---\n");
    printf_func("SP_EL1:           0x%lx\n", GET_SP_EL1());
    printf_func("SP_EL0:           0x%lx\n", GET_SP_EL0());

    printf_func("\n--- EL3 Registers ---\n");
    printf_func("SCR_EL3:          0x%lx\n", GET_SCR_EL3());
    printf_func("SCTLR_EL3:        0x%lx\n", GET_SCTLR_EL3());
    printf_func("SPSR_EL3:         0x%x\n", GET_SPSR_EL3());
    printf_func("ELR_EL3:          0x%lx\n", GET_ELR_EL3());
    printf_func("ESR_EL3:          0x%x\n", GET_ESR_EL3());
    printf_func("FAR_EL3:          0x%lx\n", GET_FAR_EL3());
    printf_func("VBAR_EL3:         0x%lx\n", GET_VBAR_EL3());
    printf_func("CPTR_EL3:         0x%lx\n", GET_CPTR_EL3());
    printf_func("MAIR_EL3:         0x%lx\n", GET_MAIR_EL3());
    printf_func("ACTLR_EL3:        0x%lx\n", GET_ACTLR_EL3());
    
    printf_func("\n========= Register Dump End =========\n");
}

#endif // CPU_H
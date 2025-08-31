#ifndef CPU_H
#define CPU_H

#define EL1_PHY_TIM_IRQ 30u

/* REGISTERS */

#define GET_MPIDR() ( \
    { \
        unsigned int mpidr; \
        asm volatile ("mrs %0, mpidr_el1" : "=r"(mpidr)); \
        mpidr; \
    })

#define GET_CURRENT_EL() ( \
    { \
        unsigned int current_el; \
        asm volatile ("mrs %0, CurrentEL" : "=r"(current_el)); \
        current_el; \
    })

#define GET_VBAR_EL3() ( \
    { \
        uint64_t vbar; \
        asm volatile ("mrs %0, vbar_el3" : "=r"(vbar)); \
        vbar; \
    })

#define GET_SPSEL() ( \
    { \
        uint32_t spsel; \
        asm volatile ("mrs %0, spsel" : "=r"(spsel)); \
        spsel; \
    })

#define GET_DAIF() ( \
    { \
        uint32_t daif; \
        asm volatile ("mrs %0, daif" : "=r"(daif)); \
        daif; \
    })

/* TIMERS */

#define GET_TIMER_FREQ() ( \
    { \
        uint32_t freq; \
        asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq)); \
        freq; \
    })

#define GET_TIMER_COUNT() ( \
    { \
        uint64_t count; \
        asm volatile ("mrs %0, cntpct_el0" : "=r"(count)); \
        count; \
    })

#define SET_SECURE_TIMER_VALUE(tval) \
    asm volatile ("msr cntps_tval_el1, %0" : : "r"(tval))

#define GET_SECURE_TIMER_VALUE() ( \
    { \
        uint32_t tval; \
        asm volatile ("mrs %0, cntps_tval_el1" : "=r"(tval)); \
        tval; \
    })

#define ENABLE_SECURE_TIMER() \
    asm volatile ("msr cntps_ctl_el1, %0" : : "r"(1))

#define DISABLE_SECURE_TIMER() \
    asm volatile ("msr cntps_ctl_el1, %0" : : "r"(0))

#define GET_SECURE_TIMER_STATUS() ( \
    { \
        uint32_t ctl; \
        asm volatile ("mrs %0, cntps_ctl_el1" : "=r"(ctl)); \
        ctl; \
    })


/* INTERRUPTS */

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


#endif // CPU_H
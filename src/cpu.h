#ifndef CPU_H
#define CPU_H

#define GET_MPIDR() ( \
    { \
        unsigned int mpidr; \
        asm volatile ("mrs %0, mpidr_el1" : "=r"(mpidr)); \
        mpidr; \
    })

#endif // CPU_H
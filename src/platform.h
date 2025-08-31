#ifndef PLATFORM_H
#define PLATFORM_H

#define PLATFORM_NAME "BCM2711"
#define LOW_PERIPHERAL_MODE


#ifdef LOW_PERIPHERAL_MODE

#define UART0_BASE      0xfe201000
#define SYS_TIMER_BASE  0xfe003000
#define ARM_TIMER_BASE  0xfe00b400
#define GICD_BASE       0xff841000
#define GICC_BASE       0xff842000

#endif // LOW_PERIPHERAL_MODE

#define VC_IRQ_BASE 96 

#define UART_IRQ          (VC_IRQ_BASE + 57)
#define SYS_TIMER_C0_IRQ  (VC_IRQ_BASE)
#define SYS_TIMER_C1_IRQ  (VC_IRQ_BASE + 1)
#define SYS_TIMER_C2_IRQ  (VC_IRQ_BASE + 2)
#define SYS_TIMER_C3_IRQ  (VC_IRQ_BASE + 3)


#endif // PLATFORM_H
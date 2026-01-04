#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "armv8-a.h"
#include "platform.h"

#define BLOCK_SIZE (1UL << 30)  // 1 GiB
#define PAGE_SIZE 4096
#define L1_PAGE_TABLE_ENTRIES 512

/*
Page table descriptor bits:
+---+--------+-----+-----+---+------------------------+---+----+----+----+----+------+----+----+
| R |   SW   | UXN | PXN | R | Output address [47:12] | R | AF | SH | AP | NS | INDX | TB | VB |
+---+--------+-----+-----+---+------------------------+---+----+----+----+----+------+----+----+
 63  58    55 54    53    52  47                    12 11  10   9  8 7  6 5    4    2 1    0

R    - reserved
SW   - reserved for software use
UXN  - unprivileged execute never
PXN  - privileged execute never
AF   - access flag
SH   - shareable attribute
AP   - access permission
NS   - security bit
INDX - index into MAIR register
TB   - table descriptor bit
VB   - validity descriptor bit
*/

// Validity + Table bits [1:0]
#define VB_SHIFT        0
#define TB_SHIFT        1
#define DESC_INVALID    (0 << VB_SHIFT)                     // 0b00
#define DESC_BLOCK      (1 << VB_SHIFT) | (0 << TB_SHIFT)   // 0b01
#define DESC_TABLE      (1 << VB_SHIFT) | (1 << TB_SHIFT)   // 0b11
#define DESC_PAGE       (1 << VB_SHIFT) | (1 << TB_SHIFT)   // 0b11 (L3 only)

// INDX - MAIR index [4:2]
// 0 = device, 1 = Normal
#define INDX_SHIFT      2
#define INDX_MASK       (0x7 << INDX_SHIFT)
#define INDX(n)         ((n) << INDX_SHIFT)
#define INDX_DEVICE     INDX(0)
#define INDX_NORMAL_WB  INDX(1)
#define INDX_NORMAL_NC  INDX(2)

// NS - Non-secure bit [5]
#define NS_SHIFT        5
#define NS              (1 << NS_SHIFT)

// AP - Access permission [7:6]
#define AP_SHIFT        6
#define AP_MASK         (0x3 << AP_SHIFT)
#define AP_RW_EL1       (0 << AP_SHIFT)  // EL1 RW, EL0 none
#define AP_RW_ALL       (1 << AP_SHIFT)  // EL1 RW, EL0 RW
#define AP_RO_EL1       (2 << AP_SHIFT)  // EL1 RO, EL0 none
#define AP_RO_ALL       (3 << AP_SHIFT)  // EL1 RO, EL0 RO

// SH - Shareability [9:8]
#define SH_SHIFT        8
#define SH_MASK         (0x3 << SH_SHIFT)
#define SH_NONE         (0 << SH_SHIFT)
#define SH_OUTER        (2 << SH_SHIFT)
#define SH_INNER        (3 << SH_SHIFT)

// AF - Access flag [10]
#define AF_SHIFT        10
#define AF              (1 << AF_SHIFT)

// Reserved [11]

// Output address [47:12]
#define OA_SHIFT        12
#define OA_MASK         (0xFFFFFFFFFUL << OA_SHIFT)

// Reserved [52]

// PXN - Privileged execute never [53]
#define PXN_SHIFT       53
#define PXN             (1UL << PXN_SHIFT)

// UXN - Unprivileged execute never [54]
#define UXN_SHIFT       54
#define UXN             (1UL << UXN_SHIFT)

// SW - Software use [58:55]
#define SW_SHIFT        55
#define SW_MASK         (0xFUL << SW_SHIFT)

// Reserved [63:59]

// MAIR attribute values
#define MAIR_DEVICE_nGnRnE  0x00
#define MAIR_NORMAL_WB      0xFF
#define MAIR_NORMAL_NC      0x44


void mmu_init(void);


#endif // MMU_H
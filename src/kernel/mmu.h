#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stdbool.h>

// Page/Block sizes
#define BLOCK_SIZE_L1  0x40000000UL  // 1GB (L1 block)
#define BLOCK_SIZE_L2  0x200000UL    // 2MB (L2 block)

// Table entries
#define L1_PAGE_TABLE_ENTRIES 4    // For 32-bit VA space with 1GB blocks
#define L2_PAGE_TABLE_ENTRIES 512  // 512 * 2MB = 1GB coverage

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
#define VB_MASK         1UL
#define TB_SHIFT        1
#define DESC_INVALID    (0UL << VB_SHIFT)                     // 0b00
#define DESC_BLOCK      (1UL << VB_SHIFT) | (0UL << TB_SHIFT)   // 0b01
#define DESC_TABLE      (1UL << VB_SHIFT) | (1UL << TB_SHIFT)   // 0b11
#define DESC_PAGE       (1UL << VB_SHIFT) | (1UL << TB_SHIFT)   // 0b11 (L3 only)

// INDX - MAIR index [4:2]
// 0 = device, 1 = Normal
#define INDX_SHIFT      2
#define INDX_MASK       (0x7UL << INDX_SHIFT)
#define INDX(n)         ((UINT64_C(n)) << INDX_SHIFT)
#define INDX_DEVICE     INDX(0)
#define INDX_NORMAL_WB  INDX(1)
#define INDX_NORMAL_NC  INDX(2)

// NS - Non-secure bit [5]
#define NS_SHIFT        5
#define NS              (1UL << NS_SHIFT)

// AP - Access permission [7:6]
#define AP_SHIFT        6
#define AP_MASK         (0x3UL << AP_SHIFT)
#define AP_RW_EL1       (0UL << AP_SHIFT)  // EL1 RW, EL0 none
#define AP_RW_ALL       (1UL << AP_SHIFT)  // EL1 RW, EL0 RW
#define AP_RO_EL1       (2UL << AP_SHIFT)  // EL1 RO, EL0 none
#define AP_RO_ALL       (3UL << AP_SHIFT)  // EL1 RO, EL0 RO

// SH - Shareability [9:8]
#define SH_SHIFT        8
#define SH_MASK         (0x3UL << SH_SHIFT)
#define SH_NONE         (0UL << SH_SHIFT)
#define SH_OUTER        (2UL << SH_SHIFT)
#define SH_INNER        (3UL << SH_SHIFT)

// AF - Access flag [10]
#define AF_SHIFT        10
#define AF              (1UL << AF_SHIFT)

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

uint64_t* mmu_create_user_l2_table(void);
void mmu_free_user_l2_table(uint64_t* l2_table);
void mmu_set_user_l2_table(uint64_t* l2_table);

#endif // MMU_H
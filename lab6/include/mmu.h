#ifndef MMU_H
#define MMU_H

#include "stddef.h"

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) | (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE 0b11L
#define PD_BLOCK 0b01L
#define PD_UNX (1L << 54)
#define PD_KNX (1L << 53)
#define PD_ACCESS (1L << 10)
#define PD_UK_ACCESS (1L << 6)
#define PD_RDONLY    (1L << 7)
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#define PHYS_TO_VIRT(x) (x + 0xffff000000000000)
#define VIRT_TO_PHYS(x) (x - 0xffff000000000000)
#define ENTRY_ADDR_MASK   0xfffffffff000L

#ifndef __ASSEMBLER__

#define kernel_pgd_addr 0x1000
#define kernel_pud_addr 0x2000

void *set_2M_kernel_mmu(void *x0);
void map_one_page(size_t *pgd_p, size_t va, size_t pa, size_t flag);
void mappages(size_t *pgd_p, size_t va, size_t size, size_t pa, size_t flag);
void free_page_tables(size_t *page_table, int level);

#endif //__ASSEMBLER__

#endif
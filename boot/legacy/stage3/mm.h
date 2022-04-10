/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>
#include <multiboot.h>

#define MEM_USABLE             1
#define MEM_RESERVED           2
#define MEM_ACPI_RECLAIMABLE   3
#define MEM_ACPI_NVS_MEMORY    4
#define MEM_BAD                5

struct mem_area {

   u64 base;
   u64 len;
   u32 type;
   u32 acpi;
};

struct mem_info {

   struct mem_area *mem_areas;
   u32 count;
};

/*
 * Reads from BIOS system's memory map and store it in the ma_buffer.
 * At the end, it sets the fields of the struct mem_info structure `mi`.
 */
void read_memory_map(void *buffer, size_t buf_size, struct mem_info *mi);
void poison_usable_memory(struct mem_info *mi);

/*
 * Get the first usable memory area of size `size` with address >= `min_paddr`.
 * Returns `0` in case of failure.
 */
ulong get_usable_mem(struct mem_info *mi, ulong min_paddr, ulong size);

/* Get usable memory at the highest address possible */
ulong get_high_usable_mem(struct mem_info *mi, ulong size);

static inline u32 bios_to_multiboot_mem_region(u32 bios_mem_type)
{
   STATIC_ASSERT(MEM_USABLE == MULTIBOOT_MEMORY_AVAILABLE);
   STATIC_ASSERT(MEM_RESERVED == MULTIBOOT_MEMORY_RESERVED);
   STATIC_ASSERT(MEM_ACPI_RECLAIMABLE == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE);
   STATIC_ASSERT(MEM_ACPI_NVS_MEMORY == MULTIBOOT_MEMORY_NVS);
   STATIC_ASSERT(MEM_BAD == MULTIBOOT_MEMORY_BADRAM);

   return bios_mem_type;
}

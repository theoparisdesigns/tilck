/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>

/* Limit is 20 bit */
#define GDT_LIMIT_MAX (0x000FFFFF)

/* GDT 'flags' (4 bits) */
#define GDT_GRAN_4KB          (1 << 3)
#define GDT_GRAN_BYTE              (0)
#define GDT_32BIT             (1 << 2)
#define GDT_16BIT                  (0)

/* GDT 'access' flags (8 bits) */

#define GDT_ACCESS_ACC     (1 << 0)  /*
                                      * Accessed bit. The CPU sets it to 1 when
                                      * the segment is accessed.
                                      */

#define GDT_ACCESS_RW      (1 << 1)  /*
                                      * For code segments it means readable,
                                      * otherwise the segment is unreadable.
                                      * [code segments cannot be writable.]
                                      *
                                      * For data segments, it means writable.
                                      */

#define GDT_ACCESS_DC      (1 << 2)  /*
                                      * Direction/Conforming bit
                                      *
                                      * For data segments: direction.
                                      *   0 the segment grows up (regular)
                                      *   1 the segment grown down
                                      *
                                      * For code segments: conforming.
                                      *
                                      *   0 the code can be executed only from
                                      *     the ring set in PL.
                                      *
                                      *   1 the code can be executed also from
                                      *     a lower priv. level. E.g. code in
                                      *     ring 3 can far-jump to conforming
                                      *     code in ring 2.
                                      */

#define GDT_ACCESS_EX       (1 << 3)  /*
                                       * Executable bit.
                                       *   0 means a data segment
                                       *   1 means a code segment
                                       */

#define GDT_ACCESS_S        (1 << 4)   /*
                                        * Descriptor type.
                                        *   0 means system (e.g. TSS, LDT)
                                        *   1 means regular (code/data)
                                        */

#define GDT_ACCESS_PL0      (0 << 5)   /* Ring 0 */
#define GDT_ACCESS_PL1      (1 << 5)   /* Ring 1 */
#define GDT_ACCESS_PL2      (2 << 5)   /* Ring 2 */
#define GDT_ACCESS_PL3      (3 << 5)   /* Ring 3 */

#define GDT_ACCESS_PRESENT  (1 << 7)   /* Must be set for valid segments */

#define GDT_DESC_TYPE_TSS (GDT_ACCESS_PRESENT | GDT_ACCESS_EX | GDT_ACCESS_ACC)
#define GDT_DESC_TYPE_LDT (GDT_ACCESS_PRESENT | GDT_ACCESS_RW)

/* An useful shortcut for saving some space */
#define GDT_ACC_REG (GDT_ACCESS_PRESENT | GDT_ACCESS_S)

struct gdt_entry {

   u16 limit_low;
   u16 base_low;
   u8 base_middle;

   union {

      struct {
         u8 type: 4;  /* EX + DC + RW + ACC */
         u8 s : 1;    /* 0 = system desc, 1 = regular desc */
         u8 dpl : 2;  /* desc privilege level */
         u8 p : 1;    /* present */
      };

      u8 access;
   };

   union {

      struct {
         u8 lim_high: 4;
         u8 avl : 1; /* available bit */
         u8 l : 1;   /* 64-bit segment (IA-32e only) */
         u8 d : 1;   /* default operation size. 0 = 16 bit, 1 = 32 bit */
         u8 g : 1;   /* granularity: 0 = byte, 1 = 4 KB */
      };

      struct {
         u8 limit_high: 4;
         u8 flags: 4;
      };
   };

   u8 base_high;

} PACKED;

#define INVALID_ENTRY_NUM ((u32) -1)
#define USER_DESC_FLAGS_EMPTY ((1 << 3) | (1 << 5))

struct user_desc {

   u32 entry_number;
   ulong base_addr;
   u32 limit;

   union {

      struct {
         u32 seg_32bit : 1;       /* Controls GDT_32BIT */
         u32 contents : 2;        /* Controls GDT_ACCESS_DC and GDT_ACCESS_EX */
         u32 read_exec_only : 1;  /* Controls GDT_ACCESS_RW */
         u32 limit_in_pages : 1;  /* Controls GDT_GRAN_4KB */
         u32 seg_not_present : 1; /* Controls GDT_ACCESS_PRESENT */
         u32 useable : 1;
         u32 ignored : 25;
      };

      u32 flags;
   };
};

void load_ldt(u32 entry_index_in_gdt, u32 dpl);
void gdt_set_entry(struct gdt_entry *e, ulong base, ulong lim, u8 accs, u8 fl);
int gdt_add_entry(struct gdt_entry *e);
void gdt_clear_entry(u32 index);
void gdt_entry_inc_ref_count(u32 n);

#define TSS_MAIN                   0
#define TSS_DOUBLE_FAULT           1

extern struct tss_entry tss_array[2] ALIGNED_AT(PAGE_SIZE);

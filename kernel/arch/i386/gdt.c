/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>

#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/sched.h>
#include <tilck/kernel/user.h>
#include <tilck/kernel/syscalls.h>

#include "gdt_int.h"
#include "double_fault.h"

static struct gdt_entry initial_gdt_in_bss[8];
static s32 initial_gdt_refcount_in_bss[ARRAY_SIZE(initial_gdt_in_bss)];

static u32 gdt_size = ARRAY_SIZE(initial_gdt_in_bss);
static struct gdt_entry *gdt = initial_gdt_in_bss;
static s32 *gdt_refcount = initial_gdt_refcount_in_bss;

/*
 * Tilck doesn't use i386's tasks because they don't exist in many
 * architectures. Therefore, we have just need a two tss entries: one generic
 * and one dedicated for the double-fault handling.
 *
 * NOTE: TSS entries do not require to be aligned at page boundary, but they
 * should not cross page boundaries.
 *
 * From Intel's System Programming Guide: <<
 *    Avoid placing a page boundary in the part of the TSS that the processor
 *    reads during a task switch (the first 104 bytes). The processor may not
 *    correctly perform address translations if a boundary occurs in this area.
 *    During a task switch, the processor reads and writes into the first 104
 *    bytes of each TSS (using contiguous physical addresses beginning with the
 *    physical address of the first byte of the TSS). So, after TSS access
 *    begins, if part of the 104 bytes is not physically contiguous, the
 *    processor will access incorrect information without generating a
 *    page-fault exception.
 * >>
 */

struct tss_entry tss_array[2] ALIGNED_AT(PAGE_SIZE);

static void load_gdt(struct gdt_entry *gdt, u32 entries_count);


void
gdt_set_entry(struct gdt_entry *e,
              ulong base,
              ulong limit,
              u8 access,
              u8 flags)
{
   /*
    * This function is supposed to be used only for gdt_entry elements NOT in
    * gdt (like &gdt[3]). Usually, `e` is just pointer to variable on the stack
    * later passed as argument to set_entry_num().
    */

   ASSERT(!((ulong)gdt <= (ulong)e && (ulong)e < ((ulong)gdt + gdt_size)));

   ASSERT(limit <= GDT_LIMIT_MAX); /* limit is only 20 bits */
   ASSERT(flags <= 0xf); /* flags is 4 bits */

   e->base_low = (base & 0xffff);
   e->base_middle = (base >> 16) & 0xff;
   e->base_high = (base >> 24) & 0xff;

   e->limit_low = (limit & 0xffff);
   e->limit_high = (limit >> 16) & 0x0f;

   e->access = access;
   e->flags = flags & 0xf;
}

static void
set_entry_num(u32 n, struct gdt_entry *e)
{
   ulong var;
   disable_interrupts(&var);
   {
      ASSERT(n < gdt_size);
      ASSERT(gdt_refcount[n] >= 0);

      gdt[n] = *e;
      gdt_refcount[n]++;
   }
   enable_interrupts(&var);
}

void gdt_clear_entry(u32 n)
{
   ulong var;
   disable_interrupts(&var);
   {
      ASSERT(n < gdt_size);
      ASSERT(gdt_refcount[n] > 0);

      if (--gdt_refcount[n] == 0) {
         bzero(&gdt[n], sizeof(struct gdt_entry));
      }
   }
   enable_interrupts(&var);
}

void gdt_entry_inc_ref_count(u32 n)
{
   ulong var;
   disable_interrupts(&var);
   {
      ASSERT(n < gdt_size);
      ASSERT(gdt_refcount[n] > 0);
      ASSERT(gdt[n].access);

      gdt_refcount[n]++;
   }
   enable_interrupts(&var);
}

static void
set_entry_num2(u32 n,
               ulong base,
               ulong limit,
               u8 access,
               u8 flags)
{
   struct gdt_entry e;
   gdt_set_entry(&e, base, limit, access, flags);
   set_entry_num(n, &e);
}

static NODISCARD int gdt_expand(void)
{
   void *old_gdt_ptr;
   void *old_gdt_refcount_ptr;
   u32 old_gdt_size;
   u32 new_size;

   ASSERT(are_interrupts_enabled());
   disable_preemption();
   {
      old_gdt_ptr = gdt;
      old_gdt_refcount_ptr = gdt_refcount;
      old_gdt_size = gdt_size;
      new_size = gdt_size * 2;

      if (new_size > 64 * KB) {
         enable_preemption();
         return -ENOMEM;
      }

      void *new_gdt = kzalloc_array_obj(struct gdt_entry, new_size);
      void *new_gdt_refcount;

      if (!new_gdt) {
         enable_preemption();
         return -ENOMEM;
      }

      new_gdt_refcount = kzalloc_array_obj(s32, new_size);

      if (!new_gdt_refcount) {
         kfree2(new_gdt, new_size);
         enable_preemption();
         return -1;
      }

      memcpy(new_gdt, gdt, sizeof(struct gdt_entry) * gdt_size);
      memcpy(new_gdt_refcount, gdt_refcount, sizeof(s32) * gdt_size);

      disable_interrupts_forced();
      {
         gdt = new_gdt;
         gdt_size = new_size;
         gdt_refcount = new_gdt_refcount;
         load_gdt(new_gdt, new_size);
      }
      enable_interrupts_forced();
   }
   enable_preemption();

   if (old_gdt_ptr != initial_gdt_in_bss) {

      ASSERT(old_gdt_refcount_ptr != initial_gdt_refcount_in_bss);

      kfree_array_obj(old_gdt_ptr, struct gdt_entry, old_gdt_size);
      kfree_array_obj(old_gdt_refcount_ptr, s32, old_gdt_size);
   }

   return 0;
}

int gdt_add_entry(struct gdt_entry *e)
{
   int rc = -1;

   disable_preemption();
   {
      for (u32 n = 1; n < gdt_size; n++) {
         if (!gdt[n].access) {
            set_entry_num(n, e);
            rc = (int)n;
            break;
         }
      }
   }
   enable_preemption();
   return rc;
}

static int gdt_add_ldt_entry(void *ldt_ptr, u32 size)
{
   struct gdt_entry e;

   gdt_set_entry(&e,
                 (ulong) ldt_ptr,
                 size,
                 GDT_DESC_TYPE_LDT,
                 GDT_GRAN_BYTE | GDT_32BIT);

   return gdt_add_entry(&e);
}

void set_kernel_stack(ulong stack)
{
   ulong var;
   disable_interrupts(&var);
   {
      /* Kernel stack segment = data seg */
      tss_array[TSS_MAIN].ss0 = X86_KERNEL_DATA_SEL;
      tss_array[TSS_MAIN].esp0 = stack;
      wrmsr(MSR_IA32_SYSENTER_ESP, stack);
   }
   enable_interrupts(&var);
}

static void load_gdt(struct gdt_entry *g, u32 entries_count)
{
   ASSERT(!are_interrupts_enabled());
   ASSERT(entries_count <= 64 * KB);

   struct {

      u16 size_minus_one;
      struct gdt_entry *gdt_vaddr;

   } PACKED gdt_ptr = {

      (u16)(sizeof(struct gdt_entry) * entries_count - 1),
      g,
   };

   asmVolatile("lgdt (%0)"
               : /* no output */
               : "q" (&gdt_ptr)
               : "memory");
}

void load_tss(u32 entry_index_in_gdt, u32 dpl)
{
   ASSERT(!are_interrupts_enabled());
   ASSERT(dpl <= 3); /* descriptor privilege level [0..3] */

   /*
    * Inline assembly comments: here we need to use %w0 instead of %0
    * beacuse 'ltr' requires a 16-bit register, like AX. That's what does
    * the 'w' modifier [https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html].
    */

   asmVolatile("ltr %w0"
               : /* no output */
               : "q" (X86_SELECTOR(entry_index_in_gdt, TABLE_GDT, dpl))
               : "memory");
}

void init_segmentation(void)
{
   ASSERT(!are_interrupts_enabled());

   /* Our NULL descriptor */
   set_entry_num2(0, 0, 0, 0, 0);

   /* Kernel code segment */
   set_entry_num2(1,
                  0,              /* base addr */
                  GDT_LIMIT_MAX,  /* full 4-GB segment */
                  GDT_ACC_REG | GDT_ACCESS_PL0 | GDT_ACCESS_RW | GDT_ACCESS_EX,
                  GDT_GRAN_4KB | GDT_32BIT);

   /* Kernel data segment */
   set_entry_num2(2,
                  0,
                  GDT_LIMIT_MAX,
                  GDT_ACC_REG | GDT_ACCESS_PL0 | GDT_ACCESS_RW,
                  GDT_GRAN_4KB | GDT_32BIT);

   /* Usermode code segment */
   set_entry_num2(3,
                  0,
                  GDT_LIMIT_MAX,
                  GDT_ACC_REG | GDT_ACCESS_PL3 | GDT_ACCESS_RW | GDT_ACCESS_EX,
                  GDT_GRAN_4KB | GDT_32BIT);

   /* Usermode data segment */
   set_entry_num2(4,
                  0,
                  GDT_LIMIT_MAX,
                  GDT_ACC_REG | GDT_ACCESS_PL3 | GDT_ACCESS_RW,
                  GDT_GRAN_4KB | GDT_32BIT);

   /* GDT entry for our TSS */
   set_entry_num2(5,
                  (ulong)&tss_array[TSS_MAIN],  /* TSS addr */
                  sizeof(tss_array[TSS_MAIN]),  /* limit: struct TSS size */
                  GDT_DESC_TYPE_TSS | GDT_ACCESS_PL0,
                  GDT_GRAN_BYTE | GDT_32BIT);

   /* Register other special GDT entires */
   register_double_fault_tss_entry();

   /* Load the GDT */
   load_gdt(gdt, gdt_size);

   /* Load the TSS */
   load_tss(5 /* TSS index in GDT */, 0 /* priv. level */);
}

static void DEBUG_set_thread_area(struct user_desc *d)
{
   printk(NO_PREFIX "set_thread_area(e: %i,\n"
                    "                base: %p,\n"
                    "                lim: %p,\n"
                    "                32-bit: %u,\n",
                    d->entry_number,
                    TO_PTR(d->base_addr),
                    TO_PTR(d->limit),
                    d->seg_32bit);

   printk(NO_PREFIX "                contents: %u,\n"
                    "                re_only: %u,\n"
                    "                lim in pag: %u,\n",
                    d->contents,
                    d->read_exec_only,
                    d->limit_in_pages);

   printk(NO_PREFIX "                seg_not_pres: %u,\n"
                    "                useable: %u)\n",
                    d->seg_not_present,
                    d->useable);
}

static int find_available_slot_in_user_task(void)
{
   struct process *pi = get_curr_proc();
   arch_proc_members_t *arch = get_proc_arch_fields(pi);

   for (int i = 0; i < ARRAY_SIZE(arch->gdt_entries); i++)
      if (!arch->gdt_entries[i])
         return i;

   return -1;
}

static int get_user_task_slot_for_gdt_entry(u32 gdt_entry_num)
{
   struct process *pi = get_curr_proc();
   arch_proc_members_t *arch = get_proc_arch_fields(pi);

   for (int i = 0; i < ARRAY_SIZE(arch->gdt_entries); i++)
      if (arch->gdt_entries[i] == gdt_entry_num)
         return i;

   return -1;
}

static void gdt_set_slot(struct process *pi, u16 slot, u16 gdt_index)
{
   get_proc_arch_fields(pi)->gdt_entries[slot] = gdt_index;
}

int sys_set_thread_area(void *arg)
{
   int rc = 0;
   struct gdt_entry e = {0};
   struct user_desc dc;
   struct user_desc *ud = arg;

   rc = copy_from_user(&dc, ud, sizeof(struct user_desc));

   if (rc != 0)
      return -EFAULT;

   disable_preemption();

   if (!(dc.flags == USER_DESC_FLAGS_EMPTY && !dc.base_addr && !dc.limit)) {
      gdt_set_entry(&e, dc.base_addr, dc.limit, 0, 0);
      e.s = 1;
      e.dpl = 3;
      e.d = dc.seg_32bit;
      e.type |= (dc.contents << 2);
      e.type |= !dc.read_exec_only ? GDT_ACCESS_RW : 0;
      e.g = dc.limit_in_pages;
      e.avl = dc.useable;
      e.p = !dc.seg_not_present;
   } else {
      /* The user passed an empty descriptor: entry_number cannot be -1 */
      if (dc.entry_number == INVALID_ENTRY_NUM) {
         rc = -EINVAL;
         goto out;
      }
   }

   if (dc.entry_number == INVALID_ENTRY_NUM) {

      int slot = find_available_slot_in_user_task();

      if (slot < 0) {
         rc = -ESRCH;
         goto out;
      }

      dc.entry_number = (u32)gdt_add_entry(&e);

      if (dc.entry_number == INVALID_ENTRY_NUM) {

         rc = gdt_expand();

         if (rc < 0) {
            rc = -ESRCH;
            goto out;
         }

         dc.entry_number = (u32)gdt_add_entry(&e);
         ASSERT(dc.entry_number != INVALID_ENTRY_NUM);
      }

      gdt_set_slot(get_curr_proc(), (u16)slot, (u16)dc.entry_number);
      goto out;
   }

   /* Handling the case where the user specified a GDT entry number */

   int slot = get_user_task_slot_for_gdt_entry(dc.entry_number);

   if (slot < 0) {
      /* A GDT entry with that index has never been allocated by this task */

      if (dc.entry_number >= gdt_size || gdt[dc.entry_number].access) {
         /* The entry is out-of-bounds or it's used by another task */
         rc = -EINVAL;
         goto out;
      }

      /* The entry is available, now find a slot */
      slot = find_available_slot_in_user_task();

      if (slot < 0) {
         /* Unable to find a free slot in this struct task struct */
         rc = -ESRCH;
         goto out;
      }

      gdt_set_slot(get_curr_proc(), (u16)slot, (u16)dc.entry_number);
   }

   ASSERT(dc.entry_number < gdt_size);

   set_entry_num(dc.entry_number, &e);

   /*
    * We're here because either we found a slot already containing this index
    * (therefore it must be valid) or the index is in-bounds and it is free.
    */

out:
   enable_preemption();

   if (!rc) {

      /*
       * Positive case: we get here with rc = SUCCESS, now flush back the
       * the struct user_desc (we might have changed its entry_number).
       */
      rc = copy_to_user(ud, &dc, sizeof(struct user_desc));

      if (rc < 0)
         rc = -EFAULT;
   }

   return rc;
}

void copy_main_tss_on_regs(regs_t *ctx)
{
   *ctx = (regs_t) {
      .kernel_resume_eip   = 0,
      .custom_flags        = 0,
      .gs                  = tss_array[TSS_MAIN].gs,
      .fs                  = tss_array[TSS_MAIN].fs,
      .es                  = tss_array[TSS_MAIN].es,
      .ds                  = tss_array[TSS_MAIN].ds,
      .edi                 = tss_array[TSS_MAIN].edi,
      .esi                 = tss_array[TSS_MAIN].esi,
      .ebp                 = tss_array[TSS_MAIN].ebp,
      .esp                 = tss_array[TSS_MAIN].esp,
      .ebx                 = tss_array[TSS_MAIN].ebx,
      .edx                 = tss_array[TSS_MAIN].edx,
      .ecx                 = tss_array[TSS_MAIN].ecx,
      .eax                 = tss_array[TSS_MAIN].eax,
      .int_num             = 0,
      .err_code            = 0,
      .eip                 = tss_array[TSS_MAIN].eip,
      .cs                  = tss_array[TSS_MAIN].cs,
      .eflags              = tss_array[TSS_MAIN].eflags,
      .useresp             = tss_array[TSS_MAIN].esp,
      .ss                  = tss_array[TSS_MAIN].ss0,
   };
}

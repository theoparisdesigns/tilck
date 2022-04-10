/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/interrupts.h>
#include <tilck/kernel/sched.h>

static inline bool in_fault_resumable_code(void)
{
   return get_curr_task()->faults_resume_mask != 0;
}

static inline bool is_fault_resumable(int fault_num)
{
   ASSERT(fault_num <= 31);
   return (1u << fault_num) & get_curr_task()->faults_resume_mask;
}

static inline int get_fault_num(u32 r)
{
   for (int i = 0; i < 32; i++)
      if (r & (1 << i))
         return i;

   return -1;
}

void handle_resumable_fault(regs_t *r);
u32 fault_resumable_call(u32 faults_mask, void *func, u32 nargs, ...);


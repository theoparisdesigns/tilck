/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>
#include <tilck/common/arch/generic_x86/x86_utils.h>

NORETURN void panic(const char *fmt, ...)
{
   printk("\n********************* BOOTLOADER PANIC *********************\n");

   va_list args;
   va_start(args, fmt);
   vprintk(fmt, args);
   va_end(args);

   printk("\n");

   while (true) {
      halt();
   }
}

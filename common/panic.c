/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * This whole file exists in order to allow non-bootloader/kernel code to use
 * the 'common' code. Since that common code (like fat32_base) uses ASSERT
 * and panic(), we need to have a common generic and weak panic implementation,
 * meant for traditional usermode code.
 */

#include <tilck/common/basic_defs.h>
#include <tilck/common/assert.h>

#ifndef USERMODE_APP

   /*
    * NOTE: this case exists ONLY to make the file to compile!
    * The bootloaders and the kernel MUST have their custom panic() function.
    */
   #include <tilck/common/printk.h>
   #define abort() while (1)
   #define printf printk
   #define vprintf vprintk

#else

   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
   #include <stdarg.h>

#endif

NORETURN WEAK void panic(const char *fmt, ...)
{
   printf("\n********************* PANIC *********************\n");

   va_list args;
   va_start(args, fmt);
   vprintf(fmt, args);
   va_end(args);

   printf("\n");
   abort();
}

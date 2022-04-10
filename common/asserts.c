/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>

NORETURN void assert_failed(const char *expr, const char *file, int line)
{
   panic("ASSERTION '%s' FAILED in %s:%i\n", expr, file, line);
}

NORETURN void not_reached(const char *file, int line)
{
   panic("NOT_REACHED in %s:%i\n", file, line);
}

NORETURN void not_implemented(const char *file, int line)
{
   panic("NOT_IMPLEMENTED in %s:%i\n", file, line);
}

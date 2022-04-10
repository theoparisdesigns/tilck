/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/string_util.h>

#include <tilck/kernel/sort.h>

/*
 * Insertion sort for pointer-size "objects".
 */
void insertion_sort_ptr(void *a, u32 elem_count, cmpfun_ptr cmp)
{
   ulong *arr = a;

   for (u32 i = 1; i < elem_count; i++) {

      ulong elem = arr[i]; // save the element that has to be placed
      u32 j;              // 'j' is its destination

      for (j = i; j > 0 && cmp(&arr[j - 1], &elem) > 0; j--) {
         arr[j] = arr[j-1]; // the first arr[j] was arr[i] = elem.
      }

      arr[j] = elem;
   }
}

/*
 * Generic insertion_sort implementation for objects of size 'elem_size'.
 */

void
insertion_sort_generic(void *a, ulong elem_size, u32 elem_count, cmpfun_ptr cmp)
{
   char elem[256];
   u32 j;

   ASSERT(elem_size <= sizeof(elem));

   for (u32 i = 1; i < elem_count; i++) {

      memcpy(&elem, a + i * elem_size, elem_size); /* elem = a[i] */

      for (j = i; j > 0 && cmp(a + (j - 1) * elem_size, &elem) > 0; j--) {
         memcpy(a + j * elem_size,
                a + (j - 1) * elem_size,
                elem_size); /* a[j] = a[j-1] */
      }

      memcpy(a + j * elem_size, &elem, elem_size); /* a[j] = elem */
   }
}

/* Reverse an array of pointer-sized elements */

void
array_reverse_ptr(void *a, u32 elem_count)
{
   ulong *arr = a;
   ulong tmp;

   for (u32 i = 0; i < elem_count / 2; i++) {
      tmp = arr[i];
      arr[i] = arr[elem_count - i - 1];
      arr[elem_count - i - 1] = tmp;
   }
}

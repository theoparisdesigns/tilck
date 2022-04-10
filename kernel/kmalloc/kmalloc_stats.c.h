/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef _KMALLOC_C_

   #error This is NOT a header file and it is not meant to be included

   /*
    * The only purpose of this file is to keep kmalloc.c shorter.
    * Yes, this file could be turned into a regular C source file, but at the
    * price of making several static functions and variables in kmalloc.c to be
    * just non-static. We don't want that. Code isolation is a GOOD thing.
    */

#endif

#include <tilck/kernel/bintree.h>

struct kmalloc_acc_alloc {

   struct bintree_node node;
   size_t size;
   size_t count;
};

static size_t alloc_arr_elems;
static size_t alloc_arr_used;
static struct kmalloc_acc_alloc *alloc_arr;
static struct kmalloc_acc_alloc *alloc_tree_root;

static void kmalloc_account_alloc(size_t size)
{
   struct kmalloc_acc_alloc *obj;

   if (!alloc_arr)
      return;

   obj = bintree_find_ptr(alloc_tree_root,
                          size,
                          struct kmalloc_acc_alloc,
                          node,
                          size);

   if (obj) {
      obj->count++;
      return;
   }

   if (alloc_arr_used == alloc_arr_elems) {

      /*
       * While it won't be very complex to make `alloc_arr` to be dynamically
       * resizable (with re-allocation + copy), it seems unnecessary to add
       * such a feature for this debug utility. Reason: currently, there are
       * just 42 distinct chunk sizes in Tilck, after running all of its system
       * tests, while with the current pre-allocated buffer (16 KB), we have
       * than 800+ slots, for 32-bit systems. This 20-fold ratio seems enough
       * even for the medium-long term. In particular, it's enough for the
       * kmalloc perf test, which ends up requiring > 588 slots.
       */
      panic("kmalloc: No more space in alloc_arr");
   }

   obj = &alloc_arr[alloc_arr_used++];
   bintree_node_init(&obj->node);
   obj->size = size;
   obj->count = 1;

   DEBUG_CHECKED_SUCCESS(
      bintree_insert_ptr(&alloc_tree_root,
                         obj,
                         struct kmalloc_acc_alloc,
                         node,
                         size)
   );
}

static void kmalloc_init_heavy_stats(void)
{
   ASSERT(!is_preemption_enabled());

   if (KERNEL_TEST_INT) {
      /* It does not make sense to run those heavy stats in unit tests */
      return;
   }

   const size_t alloc_arr_bytes = 4 * PAGE_SIZE;
   alloc_arr_elems = alloc_arr_bytes / sizeof(struct kmalloc_acc_alloc);
   alloc_arr_used = 0;

   alloc_arr = kmalloc(alloc_arr_bytes);

   if (!alloc_arr)
      panic("Unable to alloc memory for the kmalloc heavy stats");

   printk("kmalloc: heavy stats enabled (%zu elems)\n", alloc_arr_elems);
   kmalloc_account_alloc(alloc_arr_bytes);
}

void debug_kmalloc_chunks_stats_start_read(struct debug_kmalloc_chunks_ctx *ctx)
{
   if (!KMALLOC_HEAVY_STATS)
      return;

   ASSERT(!is_preemption_enabled());
   bintree_in_order_visit_start(&ctx->ctx,
                                alloc_tree_root,
                                struct kmalloc_acc_alloc,
                                node,
                                true);
}

bool
debug_kmalloc_chunks_stats_next(struct debug_kmalloc_chunks_ctx *ctx,
                                size_t *size, size_t *count)
{
   if (!KMALLOC_HEAVY_STATS)
      return false;

   ASSERT(!is_preemption_enabled());

   struct kmalloc_acc_alloc *obj;
   obj = bintree_in_order_visit_next(&ctx->ctx);

   if (!obj)
      return false;

   *size = obj->size;
   *count = obj->count;
   return true;
}


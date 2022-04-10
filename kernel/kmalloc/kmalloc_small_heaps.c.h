/* SPDX-License-Identifier: BSD-2-Clause */

#ifndef _KMALLOC_C_

   #error This is NOT a header file and it is not meant to be included

#endif

/*
 * NOTE: the trick to make the small heap to work well without the number of
 * small heaps to explode is to allow it to allocate just a small fraction of
 * its actual size, like 1/16th.
 */
#define SMALL_HEAP_MAX_ALLOC (SMALL_HEAP_SIZE / 16 - 1)

#define SMALL_HEAP_NODE_ALLOC_SZ \
   MAX(sizeof(struct small_heap_node), SMALL_HEAP_MAX_ALLOC + 1)

/*
 * Be careful with this: incrementing its value to 2 does not reduce with any of
 * the current tests the value of shs.lifetime_created_heaps_count: this means
 * just wasting memory. While, setting it to 0, increases that by +1. It's a
 * good parameter to have and evaluate it's effects in the long term, with more
 * and more complex uses cases.
 */

#if !KERNEL_TEST_INT
   #define MAX_EMPTY_SMALL_HEAPS    1
#else
   #define MAX_EMPTY_SMALL_HEAPS    0
#endif

struct small_heap_node {

   struct list_node node;          /* all nodes */
   struct list_node avail_node;    /* non-full nodes, including empty ones */
   struct kmalloc_heap heap;
};

static struct kmalloc_small_heaps_stats shs;
static struct list small_heaps_list;
static struct list avail_small_heaps_list;

static inline struct small_heap_node *alloc_small_heap_node(void)
{
   return kzmalloc(SMALL_HEAP_NODE_ALLOC_SZ);
}

static inline void free_small_heap_node(struct small_heap_node *obj)
{
   kfree2(obj, SMALL_HEAP_NODE_ALLOC_SZ);
}

static inline void
add_in_avail_list(struct small_heap_node *node)
{
   ASSERT(node->heap.mem_allocated < node->heap.size);
   shs.not_full_count++;

   if (shs.not_full_count > shs.peak_not_full_count)
      shs.peak_not_full_count = shs.not_full_count;

   list_add_tail(&avail_small_heaps_list, &node->avail_node);
}

static inline void
remove_from_avail_list(struct small_heap_node *node)
{
   ASSERT(!list_node_is_empty(&node->avail_node));

   ASSERT(
      node->heap.mem_allocated == node->heap.size ||
      node->heap.mem_allocated == SMALL_HEAP_MD_SIZE
   );

   ASSERT(shs.not_full_count > 0);

   list_remove(&node->avail_node);
   shs.not_full_count--;

   DEBUG_ONLY(list_node_init(&node->avail_node));
}

static inline void
register_small_heap_node(struct small_heap_node *node)
{
   ASSERT(node->heap.mem_allocated < node->heap.size);

   list_add_tail(&small_heaps_list, &node->node);
   shs.tot_count++;

   if (shs.tot_count > shs.peak_count)
      shs.peak_count = shs.tot_count;

   ASSERT(list_node_is_empty(&node->avail_node));
   add_in_avail_list(node);
}

static inline void
unregister_small_heap_node(struct small_heap_node *node)
{
   ASSERT(!list_node_is_empty(&node->avail_node));
   remove_from_avail_list(node);

   ASSERT(shs.tot_count > 0);

   list_remove(&node->node);
   shs.tot_count--;

   ASSERT(node->heap.mem_allocated == SMALL_HEAP_MD_SIZE);
   ASSERT(list_node_is_empty(&node->avail_node));
}

static struct small_heap_node *
alloc_new_small_heap_unsafe(void)
{
   struct small_heap_node *new_node;
   void *heap_data, *metadata;
   size_t actual_metadata_size;
   size_t small_heap_sz = SMALL_HEAP_SIZE;

   /*
    * NOTE: here KMALLOC_FL_DONT_ACCOUNT is used in other to avoid the "heavy"
    * kmalloc stats (when enabled) to account this allocation, as it's not
    * really a proper allocation: it's the creation of a small heap. Instead,
    * the allocation for its metadata is explicitly accounted (see below) since
    * some memory was actually consumed.
    */

   if (!(heap_data = general_kmalloc(&small_heap_sz, KMALLOC_FL_DONT_ACCOUNT)))
      return NULL;

   ASSERT(small_heap_sz == SMALL_HEAP_SIZE);
   metadata = heap_data;
   new_node = alloc_small_heap_node();

   if (!new_node) {
      kfree2(heap_data, SMALL_HEAP_SIZE);
      return NULL;
   }

   DEBUG_ONLY(list_node_init(&new_node->node));
   DEBUG_ONLY(list_node_init(&new_node->avail_node));

   bool success =
      kmalloc_create_heap(&new_node->heap,
                          (ulong)heap_data,
                          SMALL_HEAP_SIZE,
                          SMALL_HEAP_MBS,
                          0,
                          true,
                          metadata,
                          NULL,
                          NULL);

   if (!success) {
      free_small_heap_node(new_node);
      kfree2(heap_data, SMALL_HEAP_SIZE);
      return NULL;
   }

   // Allocate heap's metadata inside the heap itself
   actual_metadata_size = new_node->heap.metadata_size;

   DEBUG_ONLY_UNSAFE(void *actual_metadata =)
      per_heap_kmalloc(&new_node->heap, &actual_metadata_size, 0);
   ASSERT(actual_metadata == metadata);

   if (KMALLOC_HEAVY_STATS)
      kmalloc_account_alloc(actual_metadata_size);

   shs.lifetime_created_heaps_count++;
   register_small_heap_node(new_node);
   return new_node;
}

static struct small_heap_node *
alloc_new_small_heap(void)
{
   static ATOMIC(bool) in_use;
   struct small_heap_node *res;
   bool exp = false;

   if (!atomic_cas_strong(&in_use, &exp, true, mo_relaxed, mo_relaxed))
      return NULL;

   res = alloc_new_small_heap_unsafe();
   atomic_store_explicit(&in_use, false, mo_relaxed);
   return res;
}

static void destroy_small_heap(struct small_heap_node *node)
{
   unregister_small_heap_node(node);
   kfree2((void *)node->heap.vaddr, SMALL_HEAP_SIZE);
   free_small_heap_node(node);
}

static void *small_heaps_kmalloc(size_t *size, u32 flags)
{
   struct small_heap_node *new_node;
   struct small_heap_node *pos;
   void *ret;
   bool was_empty;

   ASSERT(!is_preemption_enabled());
   ASSERT(*size <= (SMALL_HEAP_SIZE - SMALL_HEAP_MBS));

   list_for_each_ro(pos, &avail_small_heaps_list, avail_node) {

      if (pos->heap.size - pos->heap.mem_allocated < *size)
         continue;

      was_empty = pos->heap.mem_allocated == SMALL_HEAP_MD_SIZE;

      /* We've found a heap with (potentially) enough space */
      if ((ret = per_heap_kmalloc(&pos->heap, size, flags))) {

         /*
          * The alloc succeeded. If now the heap is full, remove it from the
          * 'avail' struct list.
          */

         if (was_empty) {
            shs.empty_count--;
            ASSERT(shs.empty_count >= 0);
         }

         if (pos->heap.mem_allocated == pos->heap.size)
            remove_from_avail_list(pos);

         return ret;
      }
   }

   if (!(new_node = alloc_new_small_heap()))
      return NULL;

   // Now finally do the user allocation in the new heap
   ret = per_heap_kmalloc(&new_node->heap, size, flags);

   ASSERT(ret != NULL); // We've just created the node, if should be almost
                        // empty (expect for the metadata). There is no reason
                        // the allocation to fail.
   return ret;
}

static int
small_heaps_kfree(void *ptr, size_t *size, u32 flags)
{
   ASSERT(!is_preemption_enabled());
   struct small_heap_node *pos, *node = NULL;
   const ulong vaddr = (ulong) ptr;
   bool was_full;

   list_for_each_ro(pos, &small_heaps_list, node) {

      const ulong hva = pos->heap.vaddr;
      const ulong hend = pos->heap.heap_last_byte-pos->heap.min_block_size+1;

      if (IN_RANGE_INC(vaddr, hva, hend)) {
         node = pos;
         break;
      }
   }

   if (!node)
      return -ENOENT;

   was_full = node->heap.mem_allocated == node->heap.size;
   per_heap_kfree(&node->heap, ptr, size, flags);

   if (was_full) {

      /* If the heap was full before the free, now it isn't anymore */
      add_in_avail_list(node);

      /*
       * Because of the metadata, it's impossible with one single chunk to free
       * all the available space in the small heap: that would require having
       * a chunk with size different than a power of 2.
       */
      ASSERT(node->heap.mem_allocated > SMALL_HEAP_MD_SIZE);

   } else {

      /* The chunk wasn't full: we have to check if it's "empty" now. */
      if (node->heap.mem_allocated == SMALL_HEAP_MD_SIZE) {
         if (MAX_EMPTY_SMALL_HEAPS && shs.empty_count < MAX_EMPTY_SMALL_HEAPS) {
            shs.empty_count++;
         } else {
            destroy_small_heap(node);
         }
      }
   }

   return 0;
}

/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>
#include <tilck/kernel/bintree.h>

struct debug_kmalloc_heap_info {

   ulong vaddr;
   size_t size;
   size_t mem_allocated;
   size_t min_block_size;
   size_t alloc_block_size;
   int region;
};

struct kmalloc_small_heaps_stats {

   int tot_count;
   int peak_count;
   int not_full_count;
   int peak_not_full_count;
   int empty_count;
   int lifetime_created_heaps_count;
};

struct debug_kmalloc_chunks_ctx {
   struct bintree_walk_ctx ctx;
};

struct debug_kmalloc_stats {

   struct kmalloc_small_heaps_stats small_heaps;
   size_t chunk_sizes_count;
};

bool
debug_kmalloc_get_heap_info(int heap_num, struct debug_kmalloc_heap_info *i);

void
debug_kmalloc_get_heap_info_by_ptr(struct kmalloc_heap *h,
                                   struct debug_kmalloc_heap_info *i);

void
debug_kmalloc_get_stats(struct debug_kmalloc_stats *stats);

void
debug_kmalloc_chunks_stats_start_read(struct debug_kmalloc_chunks_ctx *ctx);

bool
debug_kmalloc_chunks_stats_next(struct debug_kmalloc_chunks_ctx *ctx,
                                size_t *size,
                                size_t *count);


/* Leak-detector and kmalloc logging */

void debug_kmalloc_start_leak_detector(bool save_metadata);
void debug_kmalloc_stop_leak_detector(bool show_leaks);

void debug_kmalloc_start_log(void);
void debug_kmalloc_stop_log(void);

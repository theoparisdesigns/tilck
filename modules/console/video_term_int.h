/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>

struct vterm;

enum term_scroll_type {
   term_scroll_up = 0,
   term_scroll_down = 1,
};

enum term_action_type {

   a_none,
   a_write,
   a_direct_write,               // [1]
   a_del_generic,
   a_scroll,                     // [2]
   a_set_col_offset,
   a_move_cur,
   a_move_cur_rel,
   a_reset,
   a_pause_output,
   a_restart_output,
   a_enable_cursor,
   a_use_alt_buffer,
   a_non_buf_scroll,             // [3]
   a_insert_blank_lines,
   a_delete_lines,
   a_set_scroll_region,
   a_insert_blank_chars,
   a_simple_del_chars,
   a_simple_erase_chars,
};

/*
 * NOTES [enum term_action_type].
 *
 *    [1] direct write without filters/move_cursor/flush
 *
 *    [2] arg1 = rows, arg2 = direction (0 = up, 1 = down)
 *          WARNING: up => text moves down, down => text moves up.
 *
 *    [3] non_buf scroll: (arg1: rows, arg2: direction (term_scroll_type))
 *          WARNING: up and down are inverted if compared to a_scroll.
 *              up => text moves up => new blank lines at the bottom
 *              down => text moves down => new blank lines at the top
 *
 *          REASON: because the `CSI n S` sequence is called SU (Scroll Up) and
 *             the `CSI n T` sequence is called SD (Scroll Down), despite what
 *             traditionally up and down mean when it's about scrolling.
 */

enum term_del_type {

   TERM_DEL_PREV_CHAR,
   TERM_DEL_PREV_WORD,
   TERM_DEL_ERASE_IN_DISPLAY,
   TERM_DEL_ERASE_IN_LINE,
};

/* --- interface exposed to the term filter func --- */

struct term_action {

   union {

      struct {
         u32 type1 :  8;
         u32 arg   : 24;
      };

      struct {
         u32 type2 :  8;
         u32 arg1  : 12;
         u32 arg2  : 12;
      };

      struct {
         u32 type3 :  8;
         u32 len   : 16;
         u32 col   :  8;
         void *ptr;
      };

      struct {
         u32 type4 :  8;
         u32 unused: 24;

         u16 arg16_1;
         u16 arg16_2;
      };
   };
};

STATIC_ASSERT(sizeof(struct term_action) == (2 * sizeof(ulong)));

u16 vterm_get_curr_row(struct vterm *t);
u16 vterm_get_curr_col(struct vterm *t);

static ALWAYS_INLINE void
term_make_action_write(struct term_action *a,
                       const char *buf,
                       size_t len,
                       u8 color)
{
   *a = (struct term_action) {
      .type3 = a_write,
      .len = UNSAFE_MIN((u32)len, MAX_TERM_WRITE_LEN),
      .col = color,
      .ptr = (void *)buf,
   };
}

static ALWAYS_INLINE void
term_make_action_direct_write(struct term_action *a,
                              const char *buf,
                              size_t len,
                              u8 color)
{
   *a = (struct term_action) {
      .type3 = a_direct_write,
      .len = UNSAFE_MIN((u32)len, MAX_TERM_WRITE_LEN),
      .col = color,
      .ptr = (void *)buf,
   };
}

static ALWAYS_INLINE void
term_make_action_del_prev_char(struct term_action *a)
{
   *a = (struct term_action) {
      .type1 = a_del_generic,
      .arg = TERM_DEL_PREV_CHAR,
   };
}

static ALWAYS_INLINE void
term_make_action_del_prev_word(struct term_action *a)
{
   *a = (struct term_action) {
      .type1 = a_del_generic,
      .arg = TERM_DEL_PREV_WORD,
   };
}

static ALWAYS_INLINE void
term_make_action_erase_in_display(struct term_action *a, u32 mode)
{
   *a = (struct term_action) {
      .type2 = a_del_generic,
      .arg1 = TERM_DEL_ERASE_IN_DISPLAY,
      .arg2 = mode,
   };
}

static ALWAYS_INLINE void
term_make_action_erase_in_line(struct term_action *a, u32 mode)
{
   *a = (struct term_action) {
      .type2 = a_del_generic,
      .arg1 = TERM_DEL_ERASE_IN_LINE,
      .arg2 = mode,
   };
}

static ALWAYS_INLINE void
term_make_action_scroll(struct term_action *a,
                        enum term_scroll_type st,
                        u32 rows)
{
   *a = (struct term_action) {
      .type2 = a_scroll,
      .arg1 = rows,
      .arg2 = st,
   };
}

static ALWAYS_INLINE void
term_make_action_set_col_off(struct term_action *a, u32 off)
{
   *a = (struct term_action) {
      .type1 = a_set_col_offset,
      .arg = off,
   };
}

static ALWAYS_INLINE void
term_make_action_move_cursor(struct term_action *a, u32 row, u32 col)
{
   *a = (struct term_action) {
      .type4 = a_move_cur,
      .arg16_1 = LO_BITS(row, 16, u16),
      .arg16_2 = LO_BITS(col, 16, u16),
   };
}

static ALWAYS_INLINE void
term_make_action_move_cursor_rel(struct term_action *a, int dr, int dc)
{
   *a = (struct term_action) {
      .type4 = a_move_cur_rel,
      .arg16_1 = LO_BITS((u32)dr, 16, u16),
      .arg16_2 = LO_BITS((u32)dc, 16, u16),
   };
}

static ALWAYS_INLINE void
term_make_action_reset(struct term_action *a)
{
   *a = (struct term_action) {
      .type1 = a_reset,
   };
}

static ALWAYS_INLINE void
term_make_action_pause_output(struct term_action *a)
{
   *a = (struct term_action) {
      .type1 = a_pause_output,
      .arg = 0,
   };
}

static ALWAYS_INLINE void
term_make_action_restart_output(struct term_action *a)
{
   *a = (struct term_action) {
      .type1 = a_restart_output,
      .arg = 0,
   };
}

static ALWAYS_INLINE void
term_make_action_set_cursor_enabled(struct term_action *a, bool value)
{
   *a = (struct term_action) {
      .type1 = a_enable_cursor,
      .arg = value,
   };
}

static ALWAYS_INLINE void
term_make_action_use_alt_buffer(struct term_action *a, bool value)
{
   *a = (struct term_action) {
      .type1 = a_use_alt_buffer,
      .arg = value,
   };
}

static ALWAYS_INLINE void
term_make_action_non_buf_scroll(struct term_action *a,
                                enum term_scroll_type st,
                                u32 rows)
{
   *a = (struct term_action) {
      .type2 = a_non_buf_scroll,
      .arg1 = rows,
      .arg2 = st,
   };
}

static ALWAYS_INLINE void
term_make_action_ins_blank_lines(struct term_action *a, u32 n)
{
   *a = (struct term_action) {
      .type1 = a_insert_blank_lines,
      .arg = n,
   };
}

static ALWAYS_INLINE void
term_make_action_del_lines(struct term_action *a, u32 n)
{
   *a = (struct term_action) {
      .type1 = a_delete_lines,
      .arg = n,
   };
}

static ALWAYS_INLINE void
term_make_action_set_scroll_region(struct term_action *a,
                                   u32 start,
                                   u32 end)
{
   *a = (struct term_action) {
      .type2 = a_set_scroll_region,
      .arg1 = start,
      .arg2 = end,
   };
}

static ALWAYS_INLINE void
term_make_action_ins_blank_chars(struct term_action *a, u16 num)
{
   *a = (struct term_action) {
      .type1 = a_insert_blank_chars,
      .arg = num,
   };
}

static ALWAYS_INLINE void
term_make_action_simple_del_chars(struct term_action *a, u16 num)
{
   *a = (struct term_action) {
      .type1 = a_simple_del_chars,
      .arg = num,
   };
}

static ALWAYS_INLINE void
term_make_action_simple_erase_chars(struct term_action *a, u16 num)
{
   *a = (struct term_action) {
      .type1 = a_simple_erase_chars,
      .arg = num,
   };
}

/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

extern u32 font_w;
extern u32 font_h;
extern u32 vga_rgb_colors[16];
extern bool __use_framebuffer;

u32 fb_get_width(void);
u32 fb_get_height(void);
u32 fb_get_bpp(void);

void fb_map_in_kernel_space(void);
void fb_raw_color_lines(u32 iy, u32 h, u32 color);
void fb_draw_cursor_raw(u32 ix, u32 iy, u32 color);
void fb_draw_char_failsafe(u32 x, u32 y, u16 entry);
void fb_draw_char_optimized(u32 x, u32 y, u16 e);
void fb_draw_char_optimized_row(u32 y, u16 *entries, u32 count, bool fpu);
void fb_copy_from_screen(u32 ix, u32 iy, u32 w, u32 h, u32 *buf);
void fb_copy_to_screen(u32 ix, u32 iy, u32 w, u32 h, u32 *buf);
void fb_lines_shift_up(u32 src_y, u32 dst_y, u32 lines_count);
bool fb_pre_render_char_scanlines(void);
bool fb_alloc_shadow_buffer(void);
void fb_raw_perf_screen_redraw(u32 color, bool use_fpu);
void fb_set_font(void *font);
void fb_draw_banner(void);

void fb_fill_fix_info(void *fix_info);
void fb_fill_var_info(void *var_info);
void fb_user_mmap(pdir_t *pdir, void *vaddr, size_t mmap_len);

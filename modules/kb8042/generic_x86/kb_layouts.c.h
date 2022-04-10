/* SPDX-License-Identifier: BSD-2-Clause */

static char numkey[128] = {
   [71] = '7', '8', '9',
   [75] = '4', '5', '6',
   [79] = '1', '2', '3',
   [82] = '0', '.',
};

static char kbd_us[128] =
{
   0,  '\033', '1', '2', '3', '4', '5', '6', '7', '8',
   '9', '0', '-', '=', '\x7f',  /* Backspace key => ASCII DEL */
   '\t',       /* Tab */
   'q', 'w', 'e', 'r',  /* 19 */
   't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\r',
   0,       /* 29   - Control */
   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  /* 39 */
   '\'', '`',
   0,      /* Left shift */
   '\\', 'z', 'x', 'c', 'v', 'b', 'n',       /* 49 */
   'm', ',', '.', '/',
   0,            /* Right shift */
   '*',
   0, /* Alt */
   ' ',  /* Space bar */
   0, /* Caps lock */
   0, /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,
   0, /* < ... F10 */
   0, /* 69 - Num lock*/
   0, /* Scroll Lock */
   0, /* Home key */
   0, /* Up Arrow */
   0, /* Page Up */
   '-',
   0, /* Left Arrow */
   0,
   0, /* Right Arrow */
   '+',
   0, /* 79 - End key*/
   0, /* Down Arrow */
   0, /* Page Down */
   0, /* Insert Key */
   0, /* Delete Key */
   0,   0,   '\\',
   0, /* F11 Key */
   0, /* F12 Key */
   0, /* All other keys are undefined */
};

static char kbd_us_up[128] =
{
   0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
   '(', ')', '_', '+', '\b',
   '\t',       /* Tab */
   'Q', 'W', 'E', 'R',  /* 19 */
   'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
   0,       /* 29   - Control */
   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',  /* 39 */
   '\"', '~',
   0,      /* Left shift */
   '|', 'Z', 'X', 'C', 'V', 'B', 'N',       /* 49 */
   'M', '<', '>', '?',
   0,            /* Right shift */
   '*',
   0, /* Alt */
   ' ',  /* Space bar */
   0, /* Caps lock */
   0, /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,
   0, /* < ... F10 */
   0, /* 69 - Num lock */
   0, /* Scroll Lock */
   0, /* Home key */
   0, /* Up Arrow */
   0, /* Page Up */
   '-',
   0, /* Left Arrow */
   0,
   0, /* Right Arrow */
   '+',
   0, /* 79 - End key */
   0, /* Down Arrow */
   0, /* Page Down */
   0, /* Insert Key */
   0, /* Delete Key */
   0,   0,  '|',
   0, /* F11 Key */
   0, /* F12 Key */
   0, /* All other keys are undefined */
};

static char *us_kb_layouts[2] =
{
   kbd_us,
   kbd_us_up
};

static u8 mediumraw_e0_keys[256] =
{
   [KEY_NUMPAD_UP]         = 0x67,    // non-numpad up
   [KEY_NUMPAD_DOWN]       = 0x6c,    // non-numpad down
   [KEY_NUMPAD_RIGHT]      = 0x6a,    // non-numpad right
   [KEY_NUMPAD_LEFT]       = 0x69,    // non-numpad left

   [KEY_LEFT_CTRL]         = 0x61,    // right ctrl
   [KEY_LEFT_ALT]          = 0x64,    // right alt

   [KEY_NUMPAD_INS]        = 0x6e,    // non-numpad INS
   [KEY_NUMPAD_HOME]       = 0x66,    // non-numpad HOME
   [KEY_NUMPAD_DEL]        = 0x6f,    // non-numpad DEL
   [KEY_NUMPAD_END]        = 0x6b,    // non-numpad END

   [KEY_NUMPAD_PAGE_UP]    = 0x68,    // non-numpad page up
   [KEY_NUMPAD_PAGE_DOWN]  = 0x6d,    // non-numpad page down

   [0x1c]                  = 0x60,    // numpad enter
   [0x35]                  = 0x62,    // numpad slash
};

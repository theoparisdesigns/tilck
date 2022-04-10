/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>

/*
 * DEBUG utils used to dump into a human-readable form a termios struct.
 * NOTE: This is intentionally a "header" and not a C file, in order to avoid
 * bootloaders to get this code compiled-in. However, this is NOT a regular
 * header file that contains declarations, but actually just a C file "exposed"
 * as a header, as the ".c.h" extension remarks.
 */

#include <termios.h>      // system header
#include <sys/ioctl.h>    // system header

#ifndef USERMODE_APP

   #include <tilck/common/string_util.h>
   #define TERMIOS_DEBUG_PRINT(...) printk(NO_PREFIX __VA_ARGS__)

#else

   #include <stdio.h>
   #define TERMIOS_DEBUG_PRINT(...) printf(__VA_ARGS__)

#endif

#define PRINT_FLAG(val, flag) \
   if ((val) & flag) TERMIOS_DEBUG_PRINT(#flag " ")

#define PRINT_IFEQ(val, e) \
   if ((val) == (e)) TERMIOS_DEBUG_PRINT(#e " ")

static void dump_c_iflag(struct termios *t)
{
   TERMIOS_DEBUG_PRINT("c_iflag: ");
   PRINT_FLAG(t->c_iflag, IGNBRK);
   PRINT_FLAG(t->c_iflag, BRKINT);
   PRINT_FLAG(t->c_iflag, IGNPAR);
   PRINT_FLAG(t->c_iflag, PARMRK);
   PRINT_FLAG(t->c_iflag, INPCK);
   PRINT_FLAG(t->c_iflag, ISTRIP);
   PRINT_FLAG(t->c_iflag, INLCR);
   PRINT_FLAG(t->c_iflag, IGNCR);
   PRINT_FLAG(t->c_iflag, ICRNL);
   PRINT_FLAG(t->c_iflag, IUCLC);
   PRINT_FLAG(t->c_iflag, IXON);
   PRINT_FLAG(t->c_iflag, IXANY);
   PRINT_FLAG(t->c_iflag, IXOFF);
   PRINT_FLAG(t->c_iflag, IMAXBEL);
   PRINT_FLAG(t->c_iflag, IUTF8);
   TERMIOS_DEBUG_PRINT("\n");
}

static void dump_c_oflag(struct termios *t)
{
   TERMIOS_DEBUG_PRINT("c_oflag: ");
   PRINT_FLAG(t->c_oflag, OPOST);
   PRINT_FLAG(t->c_oflag, OLCUC);
   PRINT_FLAG(t->c_oflag, ONLCR);
   PRINT_FLAG(t->c_oflag, OCRNL);
   PRINT_FLAG(t->c_oflag, ONOCR);
   PRINT_FLAG(t->c_oflag, ONLRET);
   PRINT_FLAG(t->c_oflag, OFILL);
   PRINT_FLAG(t->c_oflag, OFDEL);
   PRINT_FLAG(t->c_oflag, NLDLY);     // mask
   PRINT_FLAG(t->c_oflag, CRDLY);     // mask
   PRINT_FLAG(t->c_oflag, TABDLY);    // mask
   PRINT_FLAG(t->c_oflag, BSDLY);     // mask
   PRINT_FLAG(t->c_oflag, VTDLY);     // mask
   PRINT_FLAG(t->c_oflag, FFDLY);     // mask
   TERMIOS_DEBUG_PRINT("\n");
}

static void dump_c_cflag(struct termios *t)
{
   TERMIOS_DEBUG_PRINT("c_oflag: ");
   int val = (t->c_cflag & CBAUD);

   if (val & CBAUDEX) {

      val &= ~CBAUDEX;

      PRINT_IFEQ(val, B57600);
      PRINT_IFEQ(val, B115200);
      PRINT_IFEQ(val, B230400);
      PRINT_IFEQ(val, B460800);
      PRINT_IFEQ(val, B500000);
      PRINT_IFEQ(val, B576000);
      PRINT_IFEQ(val, B921600);
      PRINT_IFEQ(val, B1000000);
      PRINT_IFEQ(val, B1152000);
      PRINT_IFEQ(val, B1500000);
      PRINT_IFEQ(val, B2000000);
      PRINT_IFEQ(val, B2500000);
      PRINT_IFEQ(val, B3000000);
      PRINT_IFEQ(val, B3500000);
      PRINT_IFEQ(val, B4000000);

   } else {

      PRINT_IFEQ(val, B0);
      PRINT_IFEQ(val, B50);
      PRINT_IFEQ(val, B75);
      PRINT_IFEQ(val, B110);
      PRINT_IFEQ(val, B134);
      PRINT_IFEQ(val, B150);
      PRINT_IFEQ(val, B200);
      PRINT_IFEQ(val, B300);
      PRINT_IFEQ(val, B600);
      PRINT_IFEQ(val, B1200);
      PRINT_IFEQ(val, B1800);
      PRINT_IFEQ(val, B2400);
      PRINT_IFEQ(val, B4800);
      PRINT_IFEQ(val, B9600);
      PRINT_IFEQ(val, B19200);
      PRINT_IFEQ(val, B38400);
   }


   val = t->c_cflag & CSIZE;

   PRINT_IFEQ(val, CS5);
   PRINT_IFEQ(val, CS6);
   PRINT_IFEQ(val, CS7);
   PRINT_IFEQ(val, CS8);

   PRINT_FLAG(t->c_cflag, CSTOPB);
   PRINT_FLAG(t->c_cflag, CREAD);
   PRINT_FLAG(t->c_cflag, PARENB);
   PRINT_FLAG(t->c_cflag, PARODD);
   PRINT_FLAG(t->c_cflag, HUPCL);
   PRINT_FLAG(t->c_cflag, CLOCAL);
   PRINT_FLAG(t->c_cflag, CIBAUD);   // mask
   PRINT_FLAG(t->c_cflag, CMSPAR);
   PRINT_FLAG(t->c_cflag, CRTSCTS);
   TERMIOS_DEBUG_PRINT("\n");
}

static void dump_c_lflag(struct termios *t)
{
   TERMIOS_DEBUG_PRINT("c_lflag: ");
   PRINT_FLAG(t->c_lflag, ISIG);
   PRINT_FLAG(t->c_lflag, ICANON);
   PRINT_FLAG(t->c_lflag, XCASE);
   PRINT_FLAG(t->c_lflag, ECHO);
   PRINT_FLAG(t->c_lflag, ECHOE);
   PRINT_FLAG(t->c_lflag, ECHOK);
   PRINT_FLAG(t->c_lflag, ECHONL);
   PRINT_FLAG(t->c_lflag, ECHOCTL);
   PRINT_FLAG(t->c_lflag, ECHOPRT);
   PRINT_FLAG(t->c_lflag, ECHOKE);
   PRINT_FLAG(t->c_lflag, FLUSHO);
   PRINT_FLAG(t->c_lflag, NOFLSH);
   PRINT_FLAG(t->c_lflag, TOSTOP);
   PRINT_FLAG(t->c_lflag, PENDIN);
   PRINT_FLAG(t->c_lflag, IEXTEN);
   TERMIOS_DEBUG_PRINT("\n");
}


static const char *get_cc_name(cc_t cc)
{
#define CC_ENTRY(e) [e] = #e

   static const char *cc_names[NCCS] = {

      CC_ENTRY(VINTR),
      CC_ENTRY(VQUIT),
      CC_ENTRY(VERASE),
      CC_ENTRY(VKILL),
      CC_ENTRY(VEOF),
      CC_ENTRY(VTIME),
      CC_ENTRY(VMIN),
      CC_ENTRY(VSWTC),
      CC_ENTRY(VSTART),
      CC_ENTRY(VSTOP),
      CC_ENTRY(VSUSP),
      CC_ENTRY(VEOL),
      CC_ENTRY(VREPRINT),
      CC_ENTRY(VDISCARD),
      CC_ENTRY(VWERASE),
      CC_ENTRY(VLNEXT),
      CC_ENTRY(VEOL2),

   };

#undef CC_ENTRY

   return cc_names[cc];
}

static void dump_c_cc(struct termios *t)
{
   TERMIOS_DEBUG_PRINT("c_cc: \n");

   for (u8 i = 0; i < NCCS; i++) {

      const char *name = get_cc_name(i);

      if (name)
         TERMIOS_DEBUG_PRINT("[%-12s]: 0x%x\n", name, t->c_cc[i]);
      else
         TERMIOS_DEBUG_PRINT("[%-12d]: 0x%x\n", i, t->c_cc[i]);
   }

   TERMIOS_DEBUG_PRINT("\n");
}

static void debug_dump_termios(struct termios *t)
{
   dump_c_iflag(t);
   dump_c_oflag(t);
   dump_c_cflag(t);
   dump_c_lflag(t);
   dump_c_cc(t);
}

#undef PRINT_FLAG
#undef TERMIOS_DEBUG_PRINT

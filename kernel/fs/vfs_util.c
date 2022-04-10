/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/string_util.h>
#include <tilck/kernel/fs/vfs.h>
#include <tilck/kernel/errno.h>

static void drop_last_component(char **d_ref, char *const dest)
{
   char *d = *d_ref;

   d--; /* go back to the last written char */
   ASSERT(*d == '/');

   while (d > dest + 1) {

      *d-- = 0;

      if (*d == '/') {

         d++;     /*
                   * advance 'd' before breaking: it points to the next dest
                   * char, not to the last written one.
                   */
         break;
      }
   }

   *d_ref = d;
}

int
compute_abs_path(const char *path, const char *str_cwd, char *dest, u32 dest_s)
{
   const char *p;
   char *d = dest;

   ASSERT(*str_cwd == '/');

   if (!*path)
      return -ENOENT;

   if (*path != '/') {

      u32 cl = (u32)strlen(str_cwd);
      ASSERT(cl > 0);

      /* The current working directory is ALWAYS supposed to be ending in '/' */
      ASSERT(str_cwd[cl - 1] == '/');

      if (dest_s < strlen(path) + cl + 1)
         return -ENAMETOOLONG;

      memcpy(dest, str_cwd, cl + 1);
      d = dest + cl;

   } else {

      /* path is absolute */
      if (dest_s < strlen(path) + 1)
         return -ENAMETOOLONG;
   }


   for (p = path; *p;) {

      if (*p == '/' && p[1] == '/') {
         p++; continue; /* skip multiple slashes */
      }

      if (*p != '.')
         goto copy_char;

      /* the current character, *p, is '.' */

      if (d > dest && d[-1] != '/')
         goto copy_char;

      if (p[1] == '/' || !p[1]) {

         p++; // skip '.'

         if (*p)  // the path does NOT end here
            p++;

         continue;

      } else if (p[1] == '.' && (p[2] == '/' || !p[2])) {

         ASSERT(d > dest);

         if (d > dest + 1)
            drop_last_component(&d, dest);
         /*
          * else
          *    dest == "/": we cannot go past that
          */

         p += 2; /* skip ".." */

         if (*p) // the path does NOT end here
            p++;

         continue;

      } else {

copy_char:
         *d++ = *p++; /* default case: just copy the character */
      }
   }

   ASSERT(d > dest);

   if (d > dest + 1 && d[-1] == '/') {
      if (p[-1] != '/') {
         /* drop the trailing '/' only if the user path did NOT end with '/' */
         d--;
      }
   }

   ASSERT(d > dest); // dest must contain at least 1 char, '/'.
   *d = 0;
   return 0;
}

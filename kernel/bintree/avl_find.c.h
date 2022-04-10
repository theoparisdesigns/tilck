/* SPDX-License-Identifier: BSD-2-Clause */

#if BINTREE_PTR_FUNCS
   #define CMP(a, b) bintree_find_ptr_cmp(a, b, field_off)
#else
   #define CMP(a, b) objval_cmpfun(a, b)
#endif

#if BINTREE_PTR_FUNCS
void *
bintree_find_ptr_internal(void *root_obj,
                          const void *value_ptr,
                          long bintree_offset,
                          long field_off)
#else
void *
bintree_find_internal(void *root_obj,
                      const void *value_ptr,
                      cmpfun_ptr objval_cmpfun,
                      long bintree_offset)
#endif
{
   long c;

   while (root_obj) {

      if (!(c = CMP(root_obj, value_ptr)))
         return root_obj;

      // root_obj is smaller then val => val is bigger => go right.
      root_obj = c < 0 ? RIGHT_OF(root_obj) : LEFT_OF(root_obj);
   }

   return NULL;
}

#undef CMP

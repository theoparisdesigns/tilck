/* SPDX-License-Identifier: BSD-2-Clause */

#include <cstdio>
#include <random>
#include <vector>
#include <algorithm>
#include <gtest/gtest.h>

using namespace std;
using namespace testing;

extern "C" {
   #include <tilck/kernel/sort.h>
}

static long less_than_cmp_int(const void *a, const void *b)
{
   const long a_val = *(const long *)a;
   const long b_val = *(const long *)b;

   if (a_val < b_val)
      return -1;

   if (a_val == b_val)
      return 0;

   return 1;
}

static bool my_is_sorted(ulong *arr, int len, cmpfun_ptr cmp)
{
   if (len <= 1)
      return true;

   for (int i = 0; i < len - 1; i++)
      if (cmp(&arr[i], &arr[i+1]) > 0)
         return false;

   return true;
}

void
random_fill_vec(default_random_engine &eng,
                lognormal_distribution<> &dist,
                vector<long> &v,
                u32 elems)
{
   v.clear();
   v.resize(elems);

   for (u32 i = 0; i < elems; i++) {
      v[i] = (long) round(dist(eng));
   }
}

TEST(insertion_sort_ptr, basic_test)
{
   long vec[] = { 3, 4, 1, 0, -3, 10, 2 };

   insertion_sort_ptr((ulong *)&vec, ARRAY_SIZE(vec), less_than_cmp_int);
   ASSERT_TRUE(my_is_sorted((ulong *)vec, ARRAY_SIZE(vec), less_than_cmp_int));
}

TEST(insertion_sort_ptr, random)
{
   random_device rdev;
   const auto seed = rdev();
   default_random_engine e(seed);
   lognormal_distribution<> dist(5.0, 3);
   cout << "[ INFO     ] random seed: " << seed << endl;

   vector<long> vec;
   random_fill_vec(e, dist, vec, 1000);
   insertion_sort_ptr((ulong *)&vec[0], vec.size(), less_than_cmp_int);
   ASSERT_TRUE(my_is_sorted((ulong *)&vec[0], vec.size(), less_than_cmp_int));
}

TEST(insertion_sort_generic, basic_test)
{
   long vec[] = { 3, 4, 1, 0, -3, 10, 2 };

   insertion_sort_generic((ulong *)&vec, sizeof(long),
                          ARRAY_SIZE(vec), less_than_cmp_int);
   ASSERT_TRUE(my_is_sorted((ulong *)vec, ARRAY_SIZE(vec), less_than_cmp_int));
}

TEST(insertion_sort_generic, random)
{
   random_device rdev;
   const auto seed = rdev();
   default_random_engine e(seed);
   lognormal_distribution<> dist(5.0, 3);
   cout << "[ INFO     ] random seed: " << seed << endl;

   vector<long> vec;
   random_fill_vec(e, dist, vec, 1000);
   insertion_sort_generic((ulong *)&vec[0], sizeof(vec[0]),
                          vec.size(), less_than_cmp_int);
   ASSERT_TRUE(my_is_sorted((ulong *)&vec[0], vec.size(), less_than_cmp_int));
}

bool array_reverse_ptr_check(const vector<ulong> &vec)
{
   vector<ulong> copy = vec;

   array_reverse_ptr((void *)&vec[0], (u32)vec.size());
   reverse(copy.begin(), copy.end());

   return copy == vec;
}

TEST(reverse_array, basic)
{
   EXPECT_TRUE(array_reverse_ptr_check({ }));
   EXPECT_TRUE(array_reverse_ptr_check({ 1 }));
   EXPECT_TRUE(array_reverse_ptr_check({ 1, 2 }));
   EXPECT_TRUE(array_reverse_ptr_check({ 1, 2, 3 }));
   EXPECT_TRUE(array_reverse_ptr_check({ 1, 2, 3, 4 }));
}

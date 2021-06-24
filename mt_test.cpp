/*
 * Multithread library
 *
 * MIT License
 *
 * Copyright (c) 2021 Alex Dolzhenkov
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "thread_pool.hpp"
#include "sort.hpp"
#include "unique.hpp"
#include <stdio.h>
#include <assert.h>

static std::atomic<size_t> f_without_arg_call_count;
static void f_without_arg()
{
    fprintf(stderr, "%s called\n", __PRETTY_FUNCTION__);
    f_without_arg_call_count++;
}

static std::atomic<size_t> f_with_args_call_count;
static std::atomic<size_t> f_with_args_arg0;
static std::atomic<size_t> f_with_args_arg1;
static void f_with_args(size_t arg0, size_t arg1)
{
    fprintf(stderr, "%s called with args: %lu %lu\n", __PRETTY_FUNCTION__, arg0, arg1);
    f_with_args_arg0 = arg0;
    f_with_args_arg1 = arg1;
    f_with_args_call_count++;
}

#ifdef MT_POOL_RET_SUPPORT
static std::atomic<size_t> f_with_ret_call_count;
static std::atomic<size_t> f_with_ret_arg0;
static std::atomic<size_t> f_with_ret_arg1;
static size_t f_with_ret(size_t arg0, size_t arg1)
{
    fprintf(stderr, "%s called with args: %lu %lu\n", __PRETTY_FUNCTION__, arg0, arg1);
    f_with_ret_arg0 = arg0;
    f_with_ret_arg1 = arg1;
    f_with_ret_call_count++;
    return arg0 + arg1;
}
#endif

static void test_one_call_without_arg()
{
    // Given:
    mt::thread_pool tpool;
    f_without_arg_call_count = 0;

    // When:
    tpool.push(f_without_arg);
    tpool.wait(); // to make sure that all tasks were finished

    // Then:
    assert(f_without_arg_call_count == 1);
}

static void test_a_few_calls_without_arg()
{
    // Given:
    mt::thread_pool tpool;
    f_without_arg_call_count = 0;

    // When:
    tpool.push(f_without_arg);
    tpool.push(f_without_arg);
    tpool.push(f_without_arg);
    tpool.push(f_without_arg);
    tpool.wait(); // to make sure that all tasks were finished

    // Then:
    assert(f_without_arg_call_count == 4);
}

static void test_one_call_with_args()
{
    // Given:
    mt::thread_pool tpool;
    f_with_args_call_count = 0;
    f_with_args_arg0 = 0;
    f_with_args_arg1 = 0;

    // When:
    tpool.push(f_with_args, 123, 456);
    tpool.wait(); // to make sure that all tasks were finished

    // Then:
    assert(f_with_args_call_count == 1);
    assert(f_with_args_arg0 == 123);
    assert(f_with_args_arg1 == 456);
}

static void test_a_few_calls_with_args()
{
    // Given:
    mt::thread_pool tpool;
    f_with_args_call_count = 0;
    f_with_args_arg0 = 0;
    f_with_args_arg1 = 0;

    // When:
    tpool.push(f_with_args, 123, 456);
    tpool.push(f_with_args, 1234, 4567);
    tpool.push(f_with_args, 12345, 45678);
    tpool.push(f_with_args, 111, 222);
    tpool.wait(); // to make sure that all tasks were finished

    // Then:
    assert(f_with_args_call_count == 4);
}

#ifdef MT_POOL_RET_SUPPORT
static void test_one_call_with_ret()
{
    // Given:
    mt::thread_pool tpool;
    f_with_ret_call_count = 0;
    f_with_ret_arg0 = 0;
    f_with_ret_arg1 = 0;

    // When:
    auto future = tpool.push(f_with_ret, 123, 456);
    auto ret = future.get();

    // Then:
    assert(f_with_ret_call_count == 1);
    assert(f_with_ret_arg0 == 123);
    assert(f_with_ret_arg1 == 456);
    assert(ret == 123 + 456);
}

static void test_a_few_calls_with_ret()
{
    // Given:
    mt::thread_pool tpool;
    f_with_ret_call_count = 0;
    f_with_ret_arg0 = 0;
    f_with_ret_arg1 = 0;

    // When:
    auto future1 = tpool.push(f_with_ret, 123, 456);
    auto future2 = tpool.push(f_with_ret, 1234, 4567);
    auto future3 = tpool.push(f_with_ret, 12345, 45678);
    auto future4 = tpool.push(f_with_ret, 111, 222);

    // Then:
    assert(future1.get() == 123 + 456);
    assert(future2.get() == 1234 + 4567);
    assert(future3.get() == 12345 + 45678);
    assert(future4.get() == 111 + 222);
    assert(f_with_ret_call_count == 4);
}
#endif

template<size_t SIZE = 0x10000000>
static void test_sort_rand()
{
    std::vector<uint32_t> actual(SIZE);
    std::vector<uint32_t> expected(SIZE);

    // Given:
    for (auto& d: actual) { d = rand()*rand(); }
    expected = actual;
    auto stl_start = std::chrono::high_resolution_clock::now();
    std::sort(expected.begin(), expected.end());
    auto stl_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> stl_time = stl_end - stl_start;

    // When:
    auto mt_start = std::chrono::high_resolution_clock::now();
    mt::sort(actual.begin(), actual.end());
    auto mt_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mt_time = mt_end - mt_start;

    // Then:
    assert(actual == expected);
    fprintf(stderr, "%s: stl: %0.3fsec, mt: %0.3fsec\n", __PRETTY_FUNCTION__, stl_time.count(), mt_time.count());

    // At Ryzen 9 3950x:
    // "void test_sort_rand() [with long unsigned int SIZE = 268435456]: stl: 19.898sec, mt: 2.487sec"
}

template<size_t SIZE = 0x10000000>
static void test_sort_sorted()
{
    std::vector<uint32_t> actual(SIZE);
    std::vector<uint32_t> expected(SIZE);

    // Given:
    for (auto& d: actual) { d = rand()*rand(); }
    mt::sort(actual.begin(), actual.end());
    expected = actual;
    auto stl_start = std::chrono::high_resolution_clock::now();
    std::sort(expected.begin(), expected.end());
    auto stl_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> stl_time = stl_end - stl_start;

    // When:
    auto mt_start = std::chrono::high_resolution_clock::now();
    mt::sort(actual.begin(), actual.end());
    auto mt_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mt_time = mt_end - mt_start;

    // Then:
    assert(actual == expected);
    fprintf(stderr, "%s: stl: %0.3fsec, mt: %0.3fsec\n", __PRETTY_FUNCTION__, stl_time.count(), mt_time.count());

    // At Ryzen 9 3950x:
    // "void test_sort_sorted() [with long unsigned int SIZE = 268435456]: stl: 3.547sec, mt: 0.582sec"
}

template<size_t SIZE = 0x10000000>
static void test_sort_a_lot_of_duplicates()
{
    std::vector<uint32_t> actual(SIZE);
    std::vector<uint32_t> expected(SIZE);

    // Given:
    for (auto& d: actual) { d = rand() % UINT8_MAX; }
    expected = actual;
    auto stl_start = std::chrono::high_resolution_clock::now();
    std::sort(expected.begin(), expected.end());
    auto stl_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> stl_time = stl_end - stl_start;

    // When:
    auto mt_start = std::chrono::high_resolution_clock::now();
    mt::sort(actual.begin(), actual.end());
    auto mt_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mt_time = mt_end - mt_start;

    // Then:
    assert(actual == expected);
    fprintf(stderr, "%s: stl: %0.3fsec, mt: %0.3fsec\n", __PRETTY_FUNCTION__, stl_time.count(), mt_time.count());

    // At Ryzen 9 3950x (should be updated after fix):
    // "void test_sort_a_lot_of_duplicates() [with long unsigned int SIZE = 268435456]: stl: 7.917sec, mt: 2.369sec"
}

template<size_t SIZE = 0x100000000>
static void test_unique()
{
    std::vector<uint32_t> actual(SIZE);
    std::vector<uint32_t> expected(SIZE);

    // Given:
    for (auto& d: actual) { d = rand()*rand(); }
    mt::sort(actual.begin(), actual.end());
    expected = actual;
    auto stl_start = std::chrono::high_resolution_clock::now();
    auto stl_last = std::unique(expected.begin(), expected.end());
    auto stl_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> stl_time = stl_end - stl_start;
    expected.resize(std::distance(expected.begin(), stl_last));

    // When:
    auto mt_start = std::chrono::high_resolution_clock::now();
    auto mt_last = mt::unique(actual.begin(), actual.end());
    auto mt_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mt_time = mt_end - mt_start;
    actual.resize(std::distance(actual.begin(), mt_last));

    // Then:
    assert(actual.size() == expected.size());
    assert(actual == expected);
    fprintf(stderr, "%s: stl: %0.3fsec, mt: %0.3fsec\n", __PRETTY_FUNCTION__, stl_time.count(), mt_time.count());

    // At Ryzen 9 3950x (32Gb+ RAM required):
    // "void test_unique() [with long unsigned int SIZE = 4294967296]: stl: 13.942sec, mt: 1.357sec"
}

template<size_t SIZE = 0x100000000>
static void test_unique_a_lot_of_duplicates()
{
    std::vector<uint32_t> actual(SIZE);
    std::vector<uint32_t> expected(SIZE);

    // Given:
    for (auto& d: actual) { d = rand() % UINT8_MAX; }
    mt::sort(actual.begin(), actual.end());
    expected = actual;
    auto stl_start = std::chrono::high_resolution_clock::now();
    auto stl_last = std::unique(expected.begin(), expected.end());
    auto stl_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> stl_time = stl_end - stl_start;
    expected.resize(std::distance(expected.begin(), stl_last));

    // When:
    auto mt_start = std::chrono::high_resolution_clock::now();
    auto mt_last = mt::unique(actual.begin(), actual.end());
    auto mt_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> mt_time = mt_end - mt_start;
    actual.resize(std::distance(actual.begin(), mt_last));

    // Then:
    assert(actual.size() == expected.size());
    assert(actual == expected);
    fprintf(stderr, "%s: stl: %0.3fsec, mt: %0.3fsec\n", __PRETTY_FUNCTION__, stl_time.count(), mt_time.count());

    // At Ryzen 9 3950x (32Gb+ RAM required):
    // "void test_unique_a_lot_of_duplicates() [with long unsigned int SIZE = 4294967296]: stl: 2.116sec, mt: 0.378sec"
}

int main()
{
    test_one_call_without_arg();
    test_a_few_calls_without_arg();

    test_one_call_with_args();
    test_a_few_calls_with_args();

#ifdef MT_POOL_RET_SUPPORT
    test_one_call_with_ret();
    test_a_few_calls_with_ret();
#endif

    test_sort_rand();
    test_sort_sorted();
    test_sort_a_lot_of_duplicates();

    test_unique();
    test_unique_a_lot_of_duplicates();

    return 0;
}

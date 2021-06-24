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

/** @file mt/sort.hpp
 *
 * @brief The main aim of this library is implementation standard function using
 * multithread algorithms. Also it allows to implement your self multithread
 * algorithms more easy.
 *
 * @note This library required modern C++11 or higher
 *
 * @author Alex Dolzhenkov
 */

#ifndef MT_SORT_HPP
#define MT_SORT_HPP

#include "thread_pool.hpp"
#include <algorithm>        // for std::sort

#ifndef CONSTEXPR
#if __cplusplus >= 201703L
#define CONSTEXPR contexpr
#else
#define CONSTEXPR
#endif
#endif

namespace mt {

/**
 *  @brief Sort the elements of a sequence using a predicate for comparison.
 *  @param  begin           An iterator linked with first element.
 *  @param  end             Another iterator linked with last element.
 *  @param  cmp             A comparison functor.
 *  @param  threads_amount  Amount of thread which may be used for sorting.
 *  @return  Nothing.
 *
 *  Sorts the elements in the range @p [begin,end) in ascending order,
 *  such that @p __comp(*(i+1),*i) is false for every iterator @e i in the
 *  range @p [begin,end-1).
 *
 *  The relative ordering of equivalent elements is not preserved, use
 *  @p stable_sort() if this is needed.
*/
template<typename RandomAccessIterator, typename Compare/*, size_t chunk_size = 0x100000u*/>
CONSTEXPR inline void sort(RandomAccessIterator begin, RandomAccessIterator end, Compare cmp, size_t threads_amount = std::thread::hardware_concurrency())
{
    size_t chunk_size = std::distance(begin, end) / (threads_amount * 8);
    mt::thread_pool pool(threads_amount);

    static const std::function<void(RandomAccessIterator, RandomAccessIterator)> quick_sort = [&chunk_size, &pool, &cmp](RandomAccessIterator begin, RandomAccessIterator end) {
        const size_t sz = end - begin;
        if (sz <= 1) return;

        if (sz > chunk_size) {
            // from https://en.cppreference.com/w/cpp/algorithm/partition
            auto pivot = *std::next(begin, std::distance(begin,end)/2);
            RandomAccessIterator middle1 = std::partition(begin, end,
                         [pivot, &cmp](const typeof(*begin)& em){ return cmp(em, pivot); });
            RandomAccessIterator middle2 = std::partition(middle1, end,
                         [pivot, &cmp](const typeof(*begin)& em){ return !cmp(pivot, em); });
            pool.push(quick_sort, begin, middle1);
            pool.push(quick_sort, middle2, end);
        } else {
            std::sort(begin, end, cmp);
        }
    };

    pool.push(quick_sort, begin, end);
}


/**
 *  @brief Sort the elements of a sequence.
 *  @param  begin           An iterator linked with first element.
 *  @param  end             Another iterator linked with last element.
 *  @param  threads_amount  Amount of thread which may be used for sorting.
 *  @return  Nothing.
 *
 *  Sorts the elements in the range @p [begin,end) in ascending order,
 *  such that for each iterator @e i in the range @p [begin,end-1),
 *  *(i+1)<*i is false.
 *
 *  The relative ordering of equivalent elements is not preserved, use
 *  @p stable_sort() if this is needed.
*/
template<typename RandomAccessIterator, typename Compare = std::less<typename std::iterator_traits<RandomAccessIterator>::value_type>>
CONSTEXPR inline void sort(RandomAccessIterator begin, RandomAccessIterator end, size_t threads_amount = std::thread::hardware_concurrency())
{
    mt::sort(begin, end, Compare(), threads_amount);
}

}

#endif // MT_SORT_HPP

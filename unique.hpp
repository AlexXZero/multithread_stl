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

/** @file mt/unique.hpp
 *
 * @brief The main aim of this library is implementation standard function using
 * multithread algorithms. Also it allows to implement your self multithread
 * algorithms more easy.
 *
 * @note This library required modern C++11 or higher
 *
 * @author Alex Dolzhenkov
 */

#ifndef MT_UNIQUE_HPP
#define MT_UNIQUE_HPP

#include <vector>       // for std::vector
#include <algorithm>    // for std::unique
#include <thread>       // for std::thread
#include <future>       // for std::async
#include <string.h>

#ifndef CONSTEXPR
#if __cplusplus >= 201703L
#define CONSTEXPR contexpr
#else
#define CONSTEXPR
#endif
#endif

namespace mt {

template<class ForwardIt, class BinaryPredicate>
CONSTEXPR inline ForwardIt unique(ForwardIt begin, ForwardIt end, BinaryPredicate p, size_t threads_amount = std::thread::hardware_concurrency())
{
    std::vector<std::future<ForwardIt>> threads;
    size_t part_size = std::distance(begin, end) / threads_amount;

    for (size_t i = 0; i < threads_amount; i++) {
        auto _begin = begin + part_size * i;
        auto _end = begin + part_size * (i + 1);
        if (i == threads_amount - 1) _end = end;
        threads.push_back(std::async(std::launch::async, [_begin, _end]{ return std::unique(_begin, _end); }));
    }

    auto last = threads[0].get();
    for (size_t i = 1; i < threads_amount; i++) {
        auto _begin = begin + part_size * i;
        auto _last = threads[i].get();
        if (p(*_begin, *(last - 1))) _begin++;
        std::copy(_begin, _last, last);
        last += std::distance(_begin, _last);
    }
    return last;
}


template<class ForwardIt, typename Pred = std::equal_to<typename std::iterator_traits<ForwardIt>::value_type>>
CONSTEXPR inline ForwardIt unique(ForwardIt first, ForwardIt last, size_t threads_amount = std::thread::hardware_concurrency())
{
    return mt::unique(first, last, Pred(), threads_amount);
}

}

#endif // MT_UNIQUE_HPP

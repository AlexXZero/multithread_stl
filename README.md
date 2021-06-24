# multithread_stl
Multithread implementation of some popular STL functions like std::sort

# Description
The main aim of this library is implementation standard function using
multithread algorithms. Also it allows to implement your self multithread
algorithms more easy.

_Note: This library required modern C++11 or higher_

# Test results:

At Ryzen 9 3950x, 128Gb RAM:
void test_sort_rand() [with long unsigned int SIZE = 268435456]: stl: 19.898sec, mt: 2.487sec
void test_sort_sorted() [with long unsigned int SIZE = 268435456]: stl: 3.547sec, mt: 0.582sec
void test_sort_a_lot_of_duplicates() [with long unsigned int SIZE = 268435456]: stl: 7.917sec, mt: 2.369sec
void test_unique() [with long unsigned int SIZE = 4294967296]: stl: 13.942sec, mt: 1.357sec
void test_unique_a_lot_of_duplicates() [with long unsigned int SIZE = 4294967296]: stl: 2.116sec, mt: 0.378sec

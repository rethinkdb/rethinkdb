/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <algorithm>
#include <vector>

#include <boost/foreach.hpp>
#include "high_resolution_timer.hpp"

#include <boost/heap/heap_merge.hpp>

#if defined(__GNUC__) && (!defined __INTEL_COMPILER)
#define no_inline __attribute__ ((noinline))
#else
#define no_inline
#endif

typedef std::vector<int> test_data;

const int num_benchmarks = 1;

inline test_data make_sequential_test_data(int size)
{
    test_data v(size);
    for (int i = 0; i != size; ++i)
        v[i] = i;
    return v;
}

inline test_data make_test_data(int size)
{
    test_data v = make_sequential_test_data(size);
    std::random_shuffle(v.begin(), v.end());
    return v;
}

const int max_data = 20;

test_data const & get_data(int index)
{
    static std::vector <test_data> data;
    if (data.empty())
        for (int i = 0; i != num_benchmarks; ++i)
            data.push_back(make_test_data((1<<(max_data+1))+1024));

    return data[index];
}


#define DEFINE_BENCHMARKS_SELECTOR(SUFFIX)  \
struct make_##SUFFIX                        \
{                                           \
    template <typename heap>                \
    struct rebind {                         \
        typedef run_##SUFFIX<heap> type;    \
    };                                      \
};


template <typename pri_queue>
void fill_heap(pri_queue & q, int index, int size, int offset = 0)
{
    test_data const & data = get_data(index);

    for (int i = 0; i != size + 1; ++i) {
        q.push(data[i]);
        int top = q.top();
        q.pop();
        q.push(top);
    }
}

template <typename pri_queue,
          typename handle_container
         >
void fill_heap_with_handles(pri_queue & q, handle_container & handles, int index, int size, int offset = 0)
{
    test_data const & data = get_data(index);

    for (int i = 0; i != size + 1; ++i) {
        handles[i] = q.push(data[i]);

        if (i > 0) {
            typename pri_queue::handle_type last = handles[i-1];
            int val = *last;
            q.erase(last);
            handles[i-1] = q.push(val);
        }
    }
}


template <typename pri_queue>
struct run_sequential_push
{
    run_sequential_push(int size):
        size(size)
    {}

    void prepare(int index)
    {
        q.clear();
        fill_heap(q, index, size);
    }

    no_inline void operator()(int index)
    {
        test_data const & data = get_data(index);

        for (int i = 0; i != 16; ++i)
            q.push(data[(1<<max_data) + i]);
    }

    pri_queue q;
    int size;
};

DEFINE_BENCHMARKS_SELECTOR(sequential_push)

template <typename pri_queue>
struct run_sequential_pop
{
    run_sequential_pop(int size):
        size(size)
    {}

    void prepare(int index)
    {
        q.clear();
        fill_heap(q, index, size);
    }

    no_inline void operator()(int index)
    {
        for (int i = 0; i != 16; ++i)
            q.pop();
    }

    pri_queue q;
    int size;
};

DEFINE_BENCHMARKS_SELECTOR(sequential_pop)

template <typename pri_queue>
struct run_sequential_increase
{
    run_sequential_increase(int size):
        size(size), handles(size)
    {}

    void prepare(int index)
    {
        q.clear();
        fill_heap_with_handles(q, handles, index, size);
    }

    no_inline void operator()(int index)
    {
        test_data const & data = get_data(index);
        for (int i = 0; i != 16; ++i)
            q.increase(handles[i], data[i] + (1<<max_data));
    }

    pri_queue q;
    int size;
    std::vector<typename pri_queue::handle_type> handles;
};

DEFINE_BENCHMARKS_SELECTOR(sequential_increase)

template <typename pri_queue>
struct run_sequential_decrease
{
    run_sequential_decrease(int size):
        size(size), handles(size)
    {}

    void prepare(int index)
    {
        q.clear();
        fill_heap_with_handles(q, handles, index, size);
    }

    no_inline void operator()(int index)
    {
        test_data const & data = get_data(index);
        for (int i = 0; i != 16; ++i)
            q.decrease(handles[i], data[i] + (1<<max_data));
    }

    pri_queue q;
    int size;
    std::vector<typename pri_queue::handle_type> handles;
};

DEFINE_BENCHMARKS_SELECTOR(sequential_decrease)

template <typename pri_queue>
struct run_merge_and_clear
{
    run_merge_and_clear(int size):
        size(size)
    {}

    void prepare(int index)
    {
        q.clear();
        r.clear();

        fill_heap(q, index, size);
        fill_heap(r, index, size, size);
    }

    no_inline void operator()(int index)
    {
        boost::heap::heap_merge(q, r);
    }

    pri_queue q, r;
    int size;
};

DEFINE_BENCHMARKS_SELECTOR(merge_and_clear)

template <typename pri_queue>
struct run_equivalence
{
    run_equivalence(int size):
        size(size)
    {}

    void prepare(int index)
    {
        q.clear();
        r.clear();
        s.clear();

        fill_heap(q, index, size);
        fill_heap(r, index, size, size);
        fill_heap(s, index, size);
    }

    no_inline bool operator()(int index)
    {
        return (q == r) ^ (q == s);
    }

    pri_queue q, r, s;
    int size;
};

DEFINE_BENCHMARKS_SELECTOR(equivalence)


template <typename benchmark>
inline double run_benchmark(benchmark & b)
{
    boost::high_resolution_timer timer;
    std::vector<double> results;

    for (int i = 0; i != num_benchmarks; ++i)
    {
        b.prepare(i);
        timer.restart();
        b(i);
        double t = timer.elapsed();
        results.push_back(t);
    }

    return results.at(results.size()/2); // median
}

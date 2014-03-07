/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <iostream>
#include <iomanip>


#include "../../../boost/heap/d_ary_heap.hpp"
#include "../../../boost/heap/pairing_heap.hpp"
#include "../../../boost/heap/fibonacci_heap.hpp"
#include "../../../boost/heap/binomial_heap.hpp"
#include "../../../boost/heap/skew_heap.hpp"

#include "heap_benchmarks.hpp"

using namespace std;

template <typename benchmark_selector>
void run_benchmarks_immutable(void)
{
    for (int i = 4; i != max_data; ++i) {
        for (int j = 0; j != 8; ++j) {
            int size = 1<<i;
            if (j%4 == 1)
                size += 1<<(i-3);
            if (j%4 == 2)
                size += 1<<(i-2);
            if (j%4 == 3)
                size += (1<<(i-3)) + (1<<(i-2));
            if (j >= 4)
                size += (1<<(i-1));

            cout << size << "\t";
            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<2> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<2>, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<4> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<4>, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<8> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<8>, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::binomial_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::fibonacci_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::pairing_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::skew_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::skew_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }
            cout << endl;
        }
    }
}

template <typename benchmark_selector>
void run_benchmarks_mutable(void)
{
    for (int i = 4; i != max_data; ++i)
    {
        for (int j = 0; j != 8; ++j)
        {
            int size = 1<<i;
            if (j%4 == 1)
                size += 1<<(i-3);
            if (j%4 == 2)
                size += 1<<(i-2);
            if (j%4 == 3)
                size += (1<<(i-3)) + (1<<(i-2));
            if (j >= 4)
                size += (1<<(i-1));

            cout << size << "\t";
            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<2>, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<4>, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::d_ary_heap<long, boost::heap::arity<8>, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::binomial_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::fibonacci_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::pairing_heap<long> >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }

            {
                typedef typename benchmark_selector::
                    template rebind<boost::heap::skew_heap<long, boost::heap::mutable_<true> > >
                    ::type benchmark_functor;
                benchmark_functor benchmark(size);
                double result = run_benchmark(benchmark);
                cout << result << '\t';
            }
            cout << endl;
        }
    }
}

int main()
{
    cout << fixed << setprecision(12);

    cout << "sequential push" << endl;
    run_benchmarks_immutable<make_sequential_push>();

    cout << endl << "sequential pop" << endl;
    run_benchmarks_immutable<make_sequential_pop>();

    cout << endl << "sequential increase" << endl;
    run_benchmarks_mutable<make_sequential_increase>();

    cout << endl << "sequential decrease" << endl;
    run_benchmarks_mutable<make_sequential_decrease>();

    cout << endl << "merge_and_clear" << endl;
    run_benchmarks_immutable<make_merge_and_clear>();

    cout << endl << "equivalence" << endl;
    run_benchmarks_immutable<make_equivalence>();
}

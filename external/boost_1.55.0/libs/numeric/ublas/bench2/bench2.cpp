//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#include "bench2.hpp"

void header (std::string text) {
    std::cout << text << std::endl;
}

template<class T>
struct peak_c_plus {
    typedef T value_type;

    void operator () (int runs) const {
        try {
            static T s (0);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                s += T (0);
//                sink_scalar (s);
            }
            footer<value_type> () (0, 1, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};
template<class T>
struct peak_c_multiplies {
    typedef T value_type;

    void operator () (int runs) const {
        try {
            static T s (1);
            boost::timer t;
            for (int i = 0; i < runs; ++ i) {
                s *= T (1);
//                sink_scalar (s);
            }
            footer<value_type> () (0, 1, runs, t.elapsed ());
        }
        catch (std::exception &e) {
            std::cout << e.what () << std::endl;
        }
    }
};

template<class T>
void peak<T>::operator () (int runs) {
    header ("peak");

    header ("plus");
    peak_c_plus<T> () (runs);

    header ("multiplies");
    peak_c_multiplies<T> () (runs);
}


template <typename scalar> 
void do_bench (std::string type_string, int scale)
{
    header (type_string);
    peak<scalar> () (1000000 * scale);

    header (type_string + ", 3");
    bench_1<scalar, 3> () (1000000 * scale);
    bench_2<scalar, 3> () (300000 * scale);
    bench_3<scalar, 3> () (100000 * scale);

    header (type_string + ", 10");
    bench_1<scalar, 10> () (300000 * scale);
    bench_2<scalar, 10> () (30000 * scale);
    bench_3<scalar, 10> () (3000 * scale);

    header (type_string + ", 30");
    bench_1<scalar, 30> () (100000 * scale);
    bench_2<scalar, 30> () (3000 * scale);
    bench_3<scalar, 30> () (100 * scale);

    header (type_string + ", 100");
    bench_1<scalar, 100> () (30000 * scale);
    bench_2<scalar, 100> () (300 * scale);
    bench_3<scalar, 100> () (3 * scale);
}

int main (int argc, char *argv []) {

    int scale = 1;
    if (argc > 1)
        scale = std::atoi (argv [1]);

#ifdef USE_FLOAT
    do_bench<float> ("FLOAT", scale);
#endif

#ifdef USE_DOUBLE
    do_bench<double> ("DOUBLE", scale);
#endif

#ifdef USE_STD_COMPLEX
#ifdef USE_FLOAT
    do_bench<std::complex<float> > ("COMPLEX<FLOAT>", scale);
#endif

#ifdef USE_DOUBLE
    do_bench<std::complex<double> > ("COMPLEX<DOUBLE>", scale);
#endif
#endif

    return 0;
}

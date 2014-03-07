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

#include <boost/numeric/interval.hpp>
#include <boost/numeric/interval/io.hpp>
#include "../bench1/bench11.cpp"


#ifdef USE_FLOAT
template struct bench_1<boost::numeric::interval<float>, 3>;
template struct bench_1<boost::numeric::interval<float>, 10>;
template struct bench_1<boost::numeric::interval<float>, 30>;
template struct bench_1<boost::numeric::interval<float>, 100>;
#endif

#ifdef USE_DOUBLE
template struct bench_1<boost::numeric::interval<double>, 3>;
template struct bench_1<boost::numeric::interval<double>, 10>;
template struct bench_1<boost::numeric::interval<double>, 30>;
template struct bench_1<boost::numeric::interval<double>, 100>;
#endif

#ifdef USE_BOOST_COMPLEX
#ifdef USE_FLOAT
template struct bench_1<boost::complex<boost::numeric::interval<float> >, 3>;
template struct bench_1<boost::complex<boost::numeric::interval<float> >, 10>;
template struct bench_1<boost::complex<boost::numeric::interval<float> >, 30>;
template struct bench_1<boost::complex<boost::numeric::interval<float> >, 100>;
#endif

#ifdef USE_DOUBLE
template struct bench_1<boost::complex<boost::numeric::interval<double> >, 3>;
template struct bench_1<boost::complex<boost::numeric::interval<double> >, 10>;
template struct bench_1<boost::complex<boost::numeric::interval<double> >, 30>;
template struct bench_1<boost::complex<boost::numeric::interval<double> >, 100>;
#endif
#endif

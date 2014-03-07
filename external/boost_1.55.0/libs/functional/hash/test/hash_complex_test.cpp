
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./config.hpp"

#if !defined(BOOST_HASH_TEST_EXTENSIONS)

int main() {}

#else

#ifdef BOOST_HASH_TEST_STD_INCLUDES
#  include <functional>
#else
#  include <boost/functional/hash.hpp>
#endif

#include <boost/detail/lightweight_test.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable:4244)   // conversion from 'unsigned long' to
                                // 'unsigned short', possible loss of data
#pragma warning(disable:4245)   // conversion from 'int' to
                                // 'const unsigned short',
                                // signed/unsigned mismatch
#pragma warning(disable:4305)   // truncation from 'double' to
                                // 'const std::complex<float>::_Ty'
#pragma warning(disable:4309)   // truncation of constant value
#pragma warning(disable:4512)   // assignment operator could not be generated
#if BOOST_MSVC < 1400
#pragma warning(disable:4267)   // conversion from 'size_t' to 'unsigned int',
                                // possible loss of data
#endif
#endif

#if defined(__GNUC__) && !defined(BOOST_INTEL_CXX_VERSION)
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif

#include <complex>
#include <sstream>
#include <boost/limits.hpp>

template <class T>
void generic_complex_tests(std::complex<T> v)
{
    BOOST_HASH_TEST_NAMESPACE::hash<std::complex<T> > complex_hasher;

    BOOST_TEST(complex_hasher(v) == complex_hasher(v));

    BOOST_HASH_TEST_NAMESPACE::hash<T> real_hasher;
    T real = v.real();
    T imag = v.imag();

    BOOST_TEST(real_hasher(real) == complex_hasher(std::complex<T>(real)));

    if(imag != 0 && real_hasher(real) == complex_hasher(v)) {
        std::ostringstream os;
        os<<"real_hasher("<<real<<") == complex_hasher("
            <<v.real()<<" + "<<v.imag()<<"i) == "
            <<real_hasher(real)<<" (This might not be a bug).";
        BOOST_ERROR(os.str().c_str());
    }
}

template <class Float>
void complex_float_tests(Float*)
{
    typedef std::complex<Float> complex;
    generic_complex_tests(complex(0,0));
    generic_complex_tests(complex(0.5,0));
    generic_complex_tests(complex(25,0));
    generic_complex_tests(complex(25,0));
    generic_complex_tests(complex(static_cast<Float>(-67.5324535),static_cast<Float>(56.23578678)));
}

template <class Integer>
void complex_integral_tests(Integer*)
{
    typedef std::complex<Integer> complex;
    generic_complex_tests(complex(0,0));
    generic_complex_tests(complex(15342,124));
    generic_complex_tests(complex(25,54356));
    generic_complex_tests(complex(5325,2346));
    generic_complex_tests(complex(-243897,-49923874));
    generic_complex_tests(complex(-543,763));
}

int main()
{
    // I've comments out the short and unsigned short tests
    // as they cause warnings and don't really test
    // anything that the other tests already deal with.

    complex_float_tests((float*) 0);
    complex_float_tests((double*) 0);
    complex_float_tests((long double*) 0);
    //complex_integral_tests((short*) 0);
    complex_integral_tests((int*) 0);
    complex_integral_tests((long*) 0);
    //complex_integral_tests((unsigned short*) 0);
    complex_integral_tests((unsigned int*) 0);
    complex_integral_tests((unsigned long*) 0);

    return boost::report_errors();
}

#endif // BOOST_HASH_TEST_EXTENSIONS

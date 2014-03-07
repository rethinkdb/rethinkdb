#ifndef BOOST_SERIALIZATION_TEST_A_HPP
#define BOOST_SERIALIZATION_TEST_A_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// A.hpp    simple class test

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream> // for friend output operators
#include <cstddef> // size_t
#include <string>
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
}
#endif

#include <boost/detail/workaround.hpp>
#include <boost/limits.hpp>
#include <boost/cstdint.hpp>

#include <boost/serialization/access.hpp>

#if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
    #include <boost/detail/workaround.hpp>
    #if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
    #include <boost/archive/dinkumware.hpp>
    #endif
#endif

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>

#include <boost/preprocessor/facilities/empty.hpp>

#include "test_decl.hpp"

#if defined(A_IMPORT)
    #define DLL_DECL IMPORT_DECL
#elif defined(A_EXPORT)
    #define DLL_DECL EXPORT_DECL
#else
    #define DLL_DECL(x)
#endif

class DLL_DECL(BOOST_PP_EMPTY()) A
{
private:
    friend class boost::serialization::access;
    // note: from an aesthetic perspective, I would much prefer to have this
    // defined out of line.  Unfortunately, this trips a bug in the VC 6.0
    // compiler. So hold our nose and put it her to permit running of tests.
    // mscvc 6.0 requires template functions to be implemented. For this
    // reason we can't make abstract.
    #if BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
        template<class Archive>
        void serialize(
            Archive &ar,
            const unsigned int /* file_version */
        ){
            ar & BOOST_SERIALIZATION_NVP(b);
            #ifndef BOOST_NO_INT64_T
            ar & BOOST_SERIALIZATION_NVP(f);
            ar & BOOST_SERIALIZATION_NVP(g);
            #endif
            #if BOOST_WORKAROUND(__BORLANDC__,  <= 0x551 )
                int i;
                if(BOOST_DEDUCED_TYPENAME Archive::is_saving::value){
                    i = l;
                    ar & BOOST_SERIALIZATION_NVP(i);
                }
                else{
                    ar & BOOST_SERIALIZATION_NVP(i);
                    l = i;
                }
            #else
                ar & BOOST_SERIALIZATION_NVP(l);
            #endif
            ar & BOOST_SERIALIZATION_NVP(m);
            ar & BOOST_SERIALIZATION_NVP(n);
            ar & BOOST_SERIALIZATION_NVP(o);
            ar & BOOST_SERIALIZATION_NVP(p);
            ar & BOOST_SERIALIZATION_NVP(q);
            #ifndef BOOST_NO_CWCHAR
            ar & BOOST_SERIALIZATION_NVP(r);
            #endif
            ar & BOOST_SERIALIZATION_NVP(c);
            ar & BOOST_SERIALIZATION_NVP(s);
            ar & BOOST_SERIALIZATION_NVP(t);
            ar & BOOST_SERIALIZATION_NVP(u);
            ar & BOOST_SERIALIZATION_NVP(v);
            ar & BOOST_SERIALIZATION_NVP(w);
            ar & BOOST_SERIALIZATION_NVP(x);
            ar & BOOST_SERIALIZATION_NVP(y);
            #ifndef BOOST_NO_STD_WSTRING
            ar & BOOST_SERIALIZATION_NVP(z);
            #endif
        }
    #else
        template<class Archive>
        void serialize(
            Archive &ar,
            const unsigned int /* file_version */
        );
    #endif
    bool b;
    #ifndef BOOST_NO_INT64_T
    boost::int64_t f;
    boost::uint64_t g;
    #endif
    enum h {
        i = 0,
        j,
        k
    } l;
    std::size_t m;
    signed long n;
    unsigned long o;
    signed  short p;
    unsigned short q;
    #ifndef BOOST_NO_CWCHAR
    wchar_t r;
    #endif
    char c;
    signed char s;
    unsigned char t;
    signed int u;
    unsigned int v;
    float w;
    double x;
    std::string y;
    #ifndef BOOST_NO_STD_WSTRING
    std::wstring z;
    #endif
public:
    A();
    bool check_equal(const A &rhs) const;
    bool operator==(const A &rhs) const;
    bool operator!=(const A &rhs) const;
    bool operator<(const A &rhs) const; // used by less
    // hash function for class A
    operator std::size_t () const;
    friend std::ostream & operator<<(std::ostream & os, A const & a);
};

#undef DLL_DECL

#endif // BOOST_SERIALIZATION_TEST_A_HPP

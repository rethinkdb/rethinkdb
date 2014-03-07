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

#include <cassert>
#include <cstdlib> // for rand()
#include <cmath> // for fabs()
#include <cstddef> // size_t

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::rand; 
    using ::fabs;
    using ::size_t;
}
#endif

//#include <boost/test/test_exec_monitor.hpp>
#include <boost/limits.hpp>
#include <boost/cstdint.hpp>

#include <boost/detail/workaround.hpp>
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
#include <boost/archive/dinkumware.hpp>
#endif

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/access.hpp>

class A
{
private:
    friend class boost::serialization::access;
    // note: from an aesthetic perspective, I would much prefer to have this
    // defined out of line.  Unfortunately, this trips a bug in the VC 6.0
    // compiler. So hold our nose and put it her to permit running of tests.
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
    bool operator==(const A &rhs) const;
    bool operator!=(const A &rhs) const;
    bool operator<(const A &rhs) const; // used by less
    // hash function for class A
    operator std::size_t () const;
    friend std::ostream & operator<<(std::ostream & os, A const & a);
    friend std::istream & operator>>(std::istream & is, A & a);
};

//BOOST_TEST_DONT_PRINT_LOG_VALUE(A);

template<class S>
void randomize(S &x)
{
    assert(0 == x.size());
    for(;;){
        unsigned int i = std::rand() % 27;
        if(0 == i)
            break;
        x += static_cast<BOOST_DEDUCED_TYPENAME S::value_type>('a' - 1 + i);
    }
}

template<class T>
void accumulate(std::size_t & s, const T & t){
    const char * tptr = (const char *)(& t);
    unsigned int count = sizeof(t);
    while(count-- > 0){
        s += *tptr++;
    }
}

A::operator std::size_t () const {
    std::size_t retval = 0;
    accumulate(retval, b);
    #ifndef BOOST_NO_INT64_T
    accumulate(retval, f);
    accumulate(retval, g);
    #endif
    accumulate(retval, l);
    accumulate(retval, m);
    accumulate(retval, n);
    accumulate(retval, o);
    accumulate(retval, p);
    accumulate(retval, q);
    #ifndef BOOST_NO_CWCHAR
    accumulate(retval, r);
    #endif
    accumulate(retval, c);
    accumulate(retval, s);
    accumulate(retval, t);
    accumulate(retval, u);
    accumulate(retval, v);
    return retval;
}

inline A::A() :
    b(true),
    #ifndef BOOST_NO_INT64_T
    f(std::rand() * std::rand()),
    g(std::rand() * std::rand()),
    #endif
    l(static_cast<enum h>(std::rand() % 3)),
    m(std::rand()),
    n(std::rand()),
    o(std::rand()),
    p(std::rand()),
    q(std::rand()),
    #ifndef BOOST_NO_CWCHAR
    r(std::rand()),
    #endif
    c(std::rand()),
    s(std::rand()),
    t(std::rand()),
    u(std::rand()),
    v(std::rand()),
    w((float)std::rand()),
    x((double)std::rand())
{
    randomize(y);
    #ifndef BOOST_NO_STD_WSTRING
    randomize(z);
    #endif
}

inline bool A::operator==(const A &rhs) const
{
    if(b != rhs.b)
        return false;
    if(l != rhs.l)
        return false;
    #ifndef BOOST_NO_INT64_T
    if(f != rhs.f)
        return false;
    if(g != rhs.g)
        return false;
    #endif
    if(m != rhs.m)
        return false;
    if(n != rhs.n)
        return false;
    if(o != rhs.o)
        return false;
    if(p != rhs.p)
        return false; 
    if(q != rhs.q)
        return false;
    #ifndef BOOST_NO_CWCHAR
    if(r != rhs.r)
        return false; 
    #endif
    if(c != rhs.c)
        return false;
    if(s != rhs.s)
        return false;
    if(t != rhs.t)
        return false;
    if(u != rhs.u)
        return false; 
    if(v != rhs.v)
        return false; 
    if(w == 0 && std::fabs(rhs.w) > std::numeric_limits<float>::epsilon())
        return false;
    if(std::fabs(rhs.w/w - 1.0) > std::numeric_limits<float>::epsilon())
        return false;
    if(x == 0 && std::fabs(rhs.x - x) > std::numeric_limits<float>::epsilon())
        return false;
    if(std::fabs(rhs.x/x - 1.0) > std::numeric_limits<float>::epsilon())
        return false;
    if(0 != y.compare(rhs.y))
        return false;
    #ifndef BOOST_NO_STD_WSTRING
    if(0 != z.compare(rhs.z))
        return false;
    #endif      
    return true;
}

inline bool A::operator!=(const A &rhs) const
{
    return ! (*this == rhs);
}

inline bool A::operator<(const A &rhs) const
{
    if(b != rhs.b)
        return b < rhs.b;
    #ifndef BOOST_NO_INT64_T
    if(f != rhs.f)
        return f < rhs.f;
    if(g != rhs.g)
        return g < rhs.g;
    #endif
    if(l != rhs.l )
        return l < rhs.l;
    if(m != rhs.m )
        return m < rhs.m;
    if(n != rhs.n )
        return n < rhs.n;
    if(o != rhs.o )
        return o < rhs.o;
    if(p != rhs.p )
        return p < rhs.p;
    if(q != rhs.q )
        return q < rhs.q;
    #ifndef BOOST_NO_CWCHAR
    if(r != rhs.r )
        return r < rhs.r;
    #endif
    if(c != rhs.c )
        return c < rhs.c;
    if(s != rhs.s )
        return s < rhs.s;
    if(t != rhs.t )
        return t < rhs.t;
    if(u != rhs.u )
        return u < rhs.u; 
    if(v != rhs.v )
        return v < rhs.v;
    if(w != rhs.w )
        return w < rhs.w; 
    if(x != rhs.x )
        return x < rhs.x;
    int i = y.compare(rhs.y);
    if(i !=  0 )
        return i < 0;
    #ifndef BOOST_NO_STD_WSTRING
    int j = z.compare(rhs.z);
    if(j !=  0 )
        return j < 0;
    #endif
    return false;
}

#endif // BOOST_SERIALIZATION_TEST_A_HPP

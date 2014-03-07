/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// A.cpp    simple class test

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cassert>
#include <cstdlib> // rand()
#include <cmath>   // fabs()
#include <cstddef> // size_t

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::rand; 
    using ::fabs;
    using ::size_t;
}
#endif

#include <boost/detail/workaround.hpp>
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
#include <boost/archive/dinkumware.hpp>
#endif

#include "A.hpp"

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

#if defined(_MSC_VER)
#pragma warning(push) // Save warning settings.
#pragma warning(disable : 4244) // Disable possible loss of data warning

#endif 
A::A() :
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
    c(0xff & std::rand()),
    s(0xff & std::rand()),
    t(0xff & std::rand()),
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

#if defined(_MSC_VER)
#pragma warning(pop) // Restore warnings to previous state.
#endif 

bool A::operator==(const A &rhs) const
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

bool A::operator!=(const A &rhs) const
{
    return ! (*this == rhs);
}

bool A::operator<(const A &rhs) const
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

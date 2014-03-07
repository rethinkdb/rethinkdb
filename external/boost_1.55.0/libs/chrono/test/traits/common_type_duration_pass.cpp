//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx
//  Copyright 2010 Vicente J. Botet Escriba
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#include <boost/chrono/chrono.hpp>
#include <boost/type_traits/is_same.hpp>

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT) 
#define NOTHING ""
#endif

template <class D1, class D2, class De>
void
test()
{
    typedef typename boost::common_type<D1, D2>::type Dc;
    BOOST_CHRONO_STATIC_ASSERT((boost::is_same<Dc, De>::value), NOTHING, (D1, D2, Dc, De));
}

void testall()
{
    test<boost::chrono::duration<int, boost::ratio<1, 100> >,
         boost::chrono::duration<long, boost::ratio<1, 1000> >,
         boost::chrono::duration<long, boost::ratio<1, 1000> > >();
    test<boost::chrono::duration<long, boost::ratio<1, 100> >,
         boost::chrono::duration<int, boost::ratio<1, 1000> >,
         boost::chrono::duration<long, boost::ratio<1, 1000> > >();
    test<boost::chrono::duration<char, boost::ratio<1, 30> >,
         boost::chrono::duration<short, boost::ratio<1, 1000> >,
         boost::chrono::duration<int, boost::ratio<1, 3000> > >();
    test<boost::chrono::duration<double, boost::ratio<21, 1> >,
         boost::chrono::duration<short, boost::ratio<15, 1> >,
         boost::chrono::duration<double, boost::ratio<3, 1> > >();
}

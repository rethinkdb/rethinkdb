// Copyright (C) 2006 Arkadiy Vertleyb
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (http://www.boost.org/LICENSE_1_0.txt)

#include <boost/typeof/typeof.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>

void f()
{}

template<class T> 
struct tpl
{
    typedef BOOST_TYPEOF_TPL(&f) type;
};

typedef void(*fun_type)();
 
BOOST_STATIC_ASSERT((boost::is_same<tpl<void>::type, fun_type>::value));

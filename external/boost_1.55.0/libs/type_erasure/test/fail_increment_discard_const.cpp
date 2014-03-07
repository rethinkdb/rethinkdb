// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id: fail_increment_discard_const.cpp 78429 2012-05-12 02:37:24Z steven_watanabe $

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/operators.hpp>

using namespace boost::type_erasure;

int main()
{
    const int i = 0;
    any<incrementable<>, const _self&> x(i);
    ++x;
}

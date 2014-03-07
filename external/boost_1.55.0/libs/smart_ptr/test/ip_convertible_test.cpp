#include <boost/config.hpp>

//  wp_convertible_test.cpp
//
//  Copyright (c) 2008 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/detail/lightweight_test.hpp>
#include <boost/intrusive_ptr.hpp>

//

struct W
{
};

void intrusive_ptr_add_ref( W* )
{
}

void intrusive_ptr_release( W* )
{
}

struct X: public virtual W
{
};

struct Y: public virtual W
{
};

struct Z: public X
{
};

int f( boost::intrusive_ptr<X> )
{
    return 1;
}

int f( boost::intrusive_ptr<Y> )
{
    return 2;
}

int main()
{
    BOOST_TEST( 1 == f( boost::intrusive_ptr<Z>() ) );
    return boost::report_errors();
}

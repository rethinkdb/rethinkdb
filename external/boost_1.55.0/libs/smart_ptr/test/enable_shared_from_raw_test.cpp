#include <boost/config.hpp>

//
//  weak_from_raw_test.cpp
//
//  Copyright (c) 2009 Frank Mori Hess
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/smart_ptr/enable_shared_from_raw.hpp>

#include <boost/detail/lightweight_test.hpp>


struct X: public boost::enable_shared_from_raw
{};

void basic_weak_from_raw_test()
{
    X *p(new X);
    boost::weak_ptr<X> weak = boost::weak_from_raw(p);
    BOOST_TEST(weak.expired());
    boost::shared_ptr<X> shared(p);
    weak = boost::weak_from_raw(p);
    BOOST_TEST(weak.expired() == false);
    boost::shared_ptr<X> shared2(weak);
    BOOST_TEST((shared < shared2 || shared2 < shared) == false);
    BOOST_TEST(shared.get() == p);
}

int main()
{
    basic_weak_from_raw_test();
    return boost::report_errors();
}

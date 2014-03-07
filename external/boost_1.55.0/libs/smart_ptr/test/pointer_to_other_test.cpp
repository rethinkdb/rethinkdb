//
//  pointer_to_other_test.cpp - a test for boost/pointer_to_other.hpp
//
//  Copyright (c) 2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/pointer_to_other.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/intrusive_ptr.hpp>

#include <memory>

template<class T, class U> void assert_same_type( T** pt = 0, U** pu = 0 )
{
    pt = pu;
}

struct X;
struct Y;

int main()
{
    // shared_ptr

    assert_same_type< boost::pointer_to_other< boost::shared_ptr<X>, Y >::type, boost::shared_ptr<Y> >();
    assert_same_type< boost::pointer_to_other< boost::shared_ptr<X>, void >::type, boost::shared_ptr<void> >();
    assert_same_type< boost::pointer_to_other< boost::shared_ptr<void>, Y >::type, boost::shared_ptr<Y> >();

    // shared_array

    assert_same_type< boost::pointer_to_other< boost::shared_array<X>, Y >::type, boost::shared_array<Y> >();
    assert_same_type< boost::pointer_to_other< boost::shared_array<X>, void >::type, boost::shared_array<void> >();
    assert_same_type< boost::pointer_to_other< boost::shared_array<void>, Y >::type, boost::shared_array<Y> >();

    // scoped_ptr

    assert_same_type< boost::pointer_to_other< boost::scoped_ptr<X>, Y >::type, boost::scoped_ptr<Y> >();
    assert_same_type< boost::pointer_to_other< boost::scoped_ptr<X>, void >::type, boost::scoped_ptr<void> >();
    assert_same_type< boost::pointer_to_other< boost::scoped_ptr<void>, Y >::type, boost::scoped_ptr<Y> >();

    // scoped_array

    assert_same_type< boost::pointer_to_other< boost::scoped_array<X>, Y >::type, boost::scoped_array<Y> >();
    assert_same_type< boost::pointer_to_other< boost::scoped_array<X>, void >::type, boost::scoped_array<void> >();
    assert_same_type< boost::pointer_to_other< boost::scoped_array<void>, Y >::type, boost::scoped_array<Y> >();

    // intrusive_ptr

    assert_same_type< boost::pointer_to_other< boost::intrusive_ptr<X>, Y >::type, boost::intrusive_ptr<Y> >();
    assert_same_type< boost::pointer_to_other< boost::intrusive_ptr<X>, void >::type, boost::intrusive_ptr<void> >();
    assert_same_type< boost::pointer_to_other< boost::intrusive_ptr<void>, Y >::type, boost::intrusive_ptr<Y> >();

    // auto_ptr

    assert_same_type< boost::pointer_to_other< std::auto_ptr<X>, Y >::type, std::auto_ptr<Y> >();
    assert_same_type< boost::pointer_to_other< std::auto_ptr<X>, void >::type, std::auto_ptr<void> >();
    assert_same_type< boost::pointer_to_other< std::auto_ptr<void>, Y >::type, std::auto_ptr<Y> >();

    // raw pointer
   
    assert_same_type< boost::pointer_to_other< X *, Y >::type, Y * >();
    assert_same_type< boost::pointer_to_other< X *, void >::type, void * >();
    assert_same_type< boost::pointer_to_other< void *, Y >::type, Y * >();

    return 0;
}

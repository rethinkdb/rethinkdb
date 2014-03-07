///////////////////////////////////////////////////////////////////////////////
// noinvoke.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/proto/core.hpp>
#include <boost/proto/transform/make.hpp>
#include <boost/type_traits/add_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/test/unit_test.hpp>
namespace proto=boost::proto;
using proto::_;

struct Test
  : proto::when<
        _
      , proto::noinvoke<
            // This remove_pointer invocation is bloked by noinvoke
            boost::remove_pointer<
                // This add_pointer invocation is *not* blocked by noinvoke
                boost::add_pointer<_>
            >
        >()
    >
{};

struct Test2
  : proto::when<
        _
        // This add_pointer gets invoked because a substitution takes place
        // within it.
      , boost::add_pointer<
            proto::noinvoke<
                // This remove_pointer invocation is bloked by noinvoke
                boost::remove_pointer<
                    // This add_pointer invocation is *not* blocked by noinvoke
                    boost::add_pointer<_>
                >
            >
        >()
    >
{};

template<typename T, typename U>
struct select2nd
{
    typedef U type;
};

struct Test3
  : proto::when<
        _
        // This add_pointer gets invoked because a substitution takes place
        // within it.
      , select2nd<
            void
          , proto::noinvoke<
                // This remove_pointer invocation is bloked by noinvoke
                select2nd<
                    void
                    // This add_pointer invocation is *not* blocked by noinvoke
                  , boost::add_pointer<_>
                >
            >
        >()
    >
{};


void test_noinvoke()
{
    typedef proto::terminal<int>::type Int;
    Int i = {42};
    
    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::result_of<Test(Int)>::type
          , boost::remove_pointer<Int *>
        >
    ));
    
    boost::remove_pointer<Int *> t = Test()(i);

    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::result_of<Test2(Int)>::type
          , boost::remove_pointer<Int *> *
        >
    ));
    
    boost::remove_pointer<Int *> * t2 = Test2()(i);

    BOOST_MPL_ASSERT((
        boost::is_same<
            boost::result_of<Test3(Int)>::type
          , select2nd<void, Int *>
        >
    ));
    
    select2nd<void, Int *> t3 = Test3()(i);
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test proto::noinvoke");

    test->add(BOOST_TEST_CASE(&test_noinvoke));

    return test;
}

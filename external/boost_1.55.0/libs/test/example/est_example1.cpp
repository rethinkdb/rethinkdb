//  (C) Copyright Gennadiy Rozental 2005-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/exception_safety.hpp>
#include <boost/test/mock_object.hpp>
using namespace boost::itest;

// Boost
#include <boost/bind.hpp>

//____________________________________________________________________________//

// Here is the function we are going to use to see all execution path variants

template<typename Employee,typename Ostr>
typename Employee::string_type
validate_and_return_salary( Employee const& e, Ostr& os )
{
    if( e.Title() == "CEO" || e.Salary() > 100000 )
        os << e.First() << " " << e.Last() << " is overpaid";

    return e.First() + " " + e.Last();
}

//____________________________________________________________________________//

// Mock object we are going to use for this test

struct EmpMock : mock_object<> {
     typedef mock_object<>    mo_type;
     typedef mo_type          string_type;

     string_type const&  Title() const { BOOST_ITEST_MOCK_FUNC( EmpMock::Title ); }
     string_type const&  First() const { BOOST_ITEST_MOCK_FUNC( EmpMock::First ); }
     string_type const&  Last() const  { BOOST_ITEST_MOCK_FUNC( EmpMock::Last ); }

     mo_type const&      Salary() const { BOOST_ITEST_MOCK_FUNC( EmpMock::Salary ); }
};

//____________________________________________________________________________//

BOOST_TEST_EXCEPTION_SAFETY( test_all_exec_path )
{
   validate_and_return_salary( EmpMock(), mock_object<>::prototype() );
}

//____________________________________________________________________________//

// EOF

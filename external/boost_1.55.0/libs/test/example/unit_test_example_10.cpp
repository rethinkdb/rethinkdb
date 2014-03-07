//  (C) Copyright Gennadiy Rozental 2001-2008.
//  (C) Copyright Gennadiy Rozental & Ullrich Koethe 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>
using namespace boost::unit_test;
using boost::test_tools::close_at_tolerance;
using boost::test_tools::percent_tolerance;

// BOOST
#include <boost/lexical_cast.hpp>

// STL
#include <functional>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>

//____________________________________________________________________________//

struct account {
    account()
    : m_amount(0.0)
    {}

    void deposit(double amount) { m_amount += amount; }
    void withdraw(double amount)
    {
        if(amount > m_amount)
        {
            throw std::logic_error("You don't have that much money!");
        }
        m_amount -= amount;
    }
    double balance() const { return m_amount; }

private:
    double m_amount;
};

//____________________________________________________________________________//

struct account_test {
    account_test( double init_value ) { m_account.deposit( init_value ); }

    account m_account;  // a very simple fixture

    void test_init()
    {
        // different kinds of non-critical tests
        // they report the error and continue

        // standard assertion
        // reports 'error in "account_test::test_init": test m_account.balance() >= 0.0 failed' on error
        BOOST_CHECK( m_account.balance() >= 0.0 );

        // customized assertion
        // reports 'error in "account_test::test_init": Initial balance should be more then 1, was actual_value' on error
        BOOST_CHECK_MESSAGE( m_account.balance() > 1.0,
                             "Initial balance should be more then 1, was " << m_account.balance() );

        // equality assertion (not very wise idea use equality check on floating point values)
        // reports 'error in "account_test::test_init": test m_account.balance() == 5.0 failed [actual_value != 5]' on error
        BOOST_CHECK_EQUAL( m_account.balance(), 5.0 );

        // closeness assertion for floating-point numbers (symbol (==) used to mark closeness, (!=) to mark non closeness )
        // reports 'error in "account_test::test_init": test m_account.balance() (==) 10.0 failed [actual_value (!=) 10 (1e-010)]' on error
        BOOST_CHECK_CLOSE( m_account.balance(), 10.0, /* tolerance */ 1e-10 );
    }

    void test_deposit()
    {
        // these 2 statements just to show that usage manipulators doesn't hurt Boost.Test output
        std::cout << "Current balance: " << std::hex << (int)m_account.balance() << std::endl;
        std::cerr << "Current balance: " << std::hex << (int)m_account.balance() << std::endl;

        float curr_ballance = (float)m_account.balance();
        float deposit_value;

        std::cout << "Enter deposit value:\n";
        std::cin  >> deposit_value;

        m_account.deposit( deposit_value );

        // correct result validation; could fail due to rounding errors; use BOOST_CHECK_CLOSE instead
        // reports "test m_account.balance() == curr_ballance + deposit_value failed" on error
        BOOST_CHECK( m_account.balance() == curr_ballance + deposit_value );

        // different kinds of critical tests

        // reports 'fatal error in "account_test::test_deposit": test m_account.balance() >= 100.0 failed' on error
        BOOST_REQUIRE( m_account.balance() >= 100.0 );

        // reports 'fatal error in "account_test::test_deposit": Balance should be more than 500.1, was actual_value' on error
        BOOST_REQUIRE_MESSAGE( m_account.balance() > 500.1,
                               "Balance should be more than 500.1, was " << m_account.balance());

        // reports 'fatal error in "account_test::test_deposit": test std::not_equal_to<double>()(m_account.balance(), 999.9) failed
        //          for (999.9, 999.9)' on error
        BOOST_REQUIRE_PREDICATE( std::not_equal_to<double>(), (m_account.balance())(999.9) );

        // reports 'fatal error in "account_test::test_deposit": test close_at_tolerance<double>( 1e-9 )( m_account.balance(), 605.5)
        //          failed for (actual_value, 605.5)
        BOOST_REQUIRE_PREDICATE( close_at_tolerance<double>( percent_tolerance( 1e-9 ) ),
                                 (m_account.balance())(605.5) );
    }

    void test_withdraw()
    {
        float curr_ballance = (float)m_account.balance();

        m_account.withdraw(2.5);

        // correct result validation; could fail due to rounding errors; use BOOST_CHECK_CLOSE instead
        // reports "test m_account.balance() == curr_ballance - 2.5 failed" on error
        BOOST_CHECK( m_account.balance() == curr_ballance - 2.5 );

        // reports 'error in "account_test::test_withdraw": exception std::runtime_error is expected' on error
        BOOST_CHECK_THROW( m_account.withdraw( m_account.balance() + 1 ), std::runtime_error );

    }
};

//____________________________________________________________________________//

struct account_test_suite : public test_suite {
    account_test_suite( double init_value ) : test_suite("account_test_suite") {
        // add member function test cases to a test suite
        boost::shared_ptr<account_test> instance( new account_test( init_value ) );

        test_case* init_test_case     = BOOST_CLASS_TEST_CASE( &account_test::test_init, instance );
        test_case* deposit_test_case  = BOOST_CLASS_TEST_CASE( &account_test::test_deposit, instance );
        test_case* withdraw_test_case = BOOST_CLASS_TEST_CASE( &account_test::test_withdraw, instance );

        deposit_test_case->depends_on( init_test_case );
        withdraw_test_case->depends_on( deposit_test_case );

        add( init_test_case, 1 );
        add( deposit_test_case, 1 );
        add( withdraw_test_case );
    }
};

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char * argv[] ) {
    framework::master_test_suite().p_name.value = "Unit test example 10";

    try {
        if( argc < 2 )
            throw std::logic_error( "Initial deposit expected" );

        framework::master_test_suite().add( new account_test_suite( boost::lexical_cast<double>( argv[1] ) ) );
    }
    catch( boost::bad_lexical_cast const& ) {
         throw std::logic_error( "Initial deposit value should match format of double" );
    }

    return 0;
}

//____________________________________________________________________________//

// EOF

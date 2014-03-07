//  Unit test for boost::any.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2013.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#include <cstdlib>
#include <string>
#include <utility>

#include "boost/any.hpp"
#include "../test.hpp"
#include <boost/move/move.hpp>

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES

int main() 
{
    return EXIT_SUCCESS;
}

#else 

namespace any_tests
{
    typedef test<const char *, void (*)()> test_case;
    typedef const test_case * test_case_iterator;

    extern const test_case_iterator begin, end;
}

int main()
{
    using namespace any_tests;
    tester<test_case_iterator> test_suite(begin, end);
    return test_suite() ? EXIT_SUCCESS : EXIT_FAILURE;
}

namespace any_tests // test suite
{
    void test_move_construction();
    void test_move_assignment();
    void test_copy_construction();
    void test_copy_assignment();
    
    void test_move_construction_from_value();
    void test_move_assignment_from_value();
    void test_copy_construction_from_value();
    void test_copy_assignment_from_value();
    void test_construction_from_const_any_rv();
    void test_cast_to_rv();
    

    const test_case test_cases[] =
    {
        { "move construction of any",             test_move_construction      },
        { "move assignment of any",               test_move_assignment        },
        { "copy construction of any",             test_copy_construction      },
        { "copy assignment of any",               test_copy_assignment        },

        { "move construction from value",         test_move_construction_from_value },
        { "move assignment from value",           test_move_assignment_from_value  },
        { "copy construction from value",         test_copy_construction_from_value },
        { "copy assignment from value",           test_copy_assignment_from_value },
        { "constructing from const any&&",        test_construction_from_const_any_rv },
        { "casting to rvalue reference",          test_cast_to_rv }
    };

    const test_case_iterator begin = test_cases;
    const test_case_iterator end =
        test_cases + (sizeof test_cases / sizeof *test_cases);

    
    class move_copy_conting_class {
    public:
        static unsigned int moves_count;
        static unsigned int copy_count;

        move_copy_conting_class(){}
        move_copy_conting_class(move_copy_conting_class&& /*param*/) {
            ++ moves_count;
        }

        move_copy_conting_class& operator=(move_copy_conting_class&& /*param*/) {
            ++ moves_count;
            return *this;
        }

        move_copy_conting_class(const move_copy_conting_class&) {
            ++ copy_count;
        }
        move_copy_conting_class& operator=(const move_copy_conting_class& /*param*/) {
            ++ copy_count;
            return *this;
        }
    };

    unsigned int move_copy_conting_class::moves_count = 0;
    unsigned int move_copy_conting_class::copy_count = 0;
}

namespace any_tests // test definitions
{
    using namespace boost;

    void test_move_construction()
    {
        any value0 = move_copy_conting_class();
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
        any value(boost::move(value0)); 

        check(value0.empty(), "moved away value is empty");
        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");
        check_equal(
            move_copy_conting_class::copy_count, 0u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
    }

    void test_move_assignment()
    {
        any value0 = move_copy_conting_class();
        any value = move_copy_conting_class();
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
        value = boost::move(value0); 

        check(value0.empty(), "moved away is empty");
        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");
        check_equal(
            move_copy_conting_class::copy_count, 0u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
    }

    void test_copy_construction()
    {
        any value0 = move_copy_conting_class();
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
        any value(value0); 

        check_false(value0.empty(), "copyed value is not empty");
        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");
        check_equal(
            move_copy_conting_class::copy_count, 1u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
    }

    void test_copy_assignment()
    {
        any value0 = move_copy_conting_class();
        any value = move_copy_conting_class();
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
        value = value0; 

        check_false(value0.empty(), "copyied value is not empty");
        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");
        check_equal(
            move_copy_conting_class::copy_count, 1u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
    }

     void test_move_construction_from_value()
    {
        move_copy_conting_class value0;
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        any value(boost::move(value0)); 
#else
        any value(value0); 
#endif

        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");
        
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        check_equal(
            move_copy_conting_class::copy_count, 0u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 1u, 
            "checking move counts");
#endif

     }

    void test_move_assignment_from_value()
    {
        move_copy_conting_class value0;
        any value;
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        value = boost::move(value0); 
#else
        value = value0;
#endif 

        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
        check_equal(
            move_copy_conting_class::copy_count, 0u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 1u, 
            "checking move counts");
#endif

    }

    void test_copy_construction_from_value()
    {
        move_copy_conting_class value0;
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
        any value(value0); 

        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");

        check_equal(
            move_copy_conting_class::copy_count, 1u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
     }

    void test_copy_assignment_from_value()
    {
        move_copy_conting_class value0;
        any value;
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;
        value = value0;

        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(move_copy_conting_class), "type");
        check_non_null(any_cast<move_copy_conting_class>(&value), "any_cast<move_copy_conting_class>");

        check_equal(
            move_copy_conting_class::copy_count, 1u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
    }

    const any helper_method() {
        return true;
    }

    const bool helper_method1() {
        return true;
    }

    void test_construction_from_const_any_rv()
    {
        any values[] = {helper_method(), helper_method1() };
        (void)values;
    }

    void test_cast_to_rv()
    {
        move_copy_conting_class value0;
        any value;
        value = value0;
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;

        move_copy_conting_class value1 = any_cast<move_copy_conting_class&&>(value);

        check_equal(
            move_copy_conting_class::copy_count, 0u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 1u, 
            "checking move counts");
        (void)value1;
/* Following code shall fail to compile
        const any cvalue = value0;
        move_copy_conting_class::copy_count = 0; 
        move_copy_conting_class::moves_count = 0;

        move_copy_conting_class value2 = any_cast<move_copy_conting_class&&>(cvalue);

        check_equal(
            move_copy_conting_class::copy_count, 1u, 
            "checking copy counts");
        check_equal(
            move_copy_conting_class::moves_count, 0u, 
            "checking move counts");
        (void)value2;
*/
    }
    
}

#endif


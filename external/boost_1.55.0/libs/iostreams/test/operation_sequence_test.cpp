/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Tests the facilities defined in the header
 * libs/iostreams/test/detail/operation_sequence.hpp
 *
 * File:        libs/iostreams/test/operation_sequence_test.cpp
 * Date:        Mon Dec 10 18:58:19 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#include <stdexcept>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/operation_sequence.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;

        // Infrastructure for checking that operations are
        // executed in the correct order

void operation_sequence_test()
{
    // Test creating a duplicate operation
    {
        operation_sequence seq;
        operation op = seq.new_operation(1);
        BOOST_CHECK_THROW(seq.new_operation(1), runtime_error);
    }

    // Test reusing an operation id after first operation is destroyed
    {
        operation_sequence seq;
        seq.new_operation(1);
        BOOST_CHECK_NO_THROW(seq.new_operation(1));
    }

    // Test creating operations with illegal error codes
    {
        operation_sequence seq;
        BOOST_CHECK_THROW(seq.new_operation(1, -100), runtime_error);
        BOOST_CHECK_THROW(
            seq.new_operation(1, BOOST_IOSTREAMS_TEST_MAX_OPERATION_ERROR + 1),
            runtime_error
        );
    }

    // Test two successful operations executed out of order
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op2 = seq.new_operation(2);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op2.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op1.execute());
        BOOST_CHECK(seq.is_failure());
    }

    // Test executing an operation twice without resetting the sequence
    {
        operation_sequence  seq;
        operation           op = seq.new_operation(1);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op.execute());
        BOOST_CHECK(seq.is_success());
        BOOST_CHECK_NO_THROW(op.execute());
        BOOST_CHECK(seq.is_failure());
    }

    // Test creating an operation after operation execution has commenced
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op2 = seq.new_operation(2);
        operation           op3;
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op1.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_THROW(op3 = seq.new_operation(3), runtime_error);
        BOOST_CHECK_NO_THROW(op2.execute());
        BOOST_CHECK(seq.is_success());
    }

    // Test three successful operations with consecutive ids, executed in order
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op2 = seq.new_operation(2);
        operation           op3 = seq.new_operation(3);

        // First pass
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op1.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op2.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op3.execute();
        BOOST_CHECK(seq.is_success());

        // Second pass
        seq.reset();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op1.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op2.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op3.execute();
        BOOST_CHECK(seq.is_success());
    }

    // Test three successful operations with non-consecutive ids, 
    // executed in order
    {
        operation_sequence  seq;
        operation           op2 = seq.new_operation(2);
        operation           op3 = seq.new_operation(101);
        operation           op1 = seq.new_operation(-43);

        // First pass
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op1.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op2.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op3.execute();
        BOOST_CHECK(seq.is_success());

        // Second pass
        seq.reset();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op1.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op2.execute();
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        op3.execute();
        BOOST_CHECK(seq.is_success());
    }

    // Test checking for success after one of three operations
    // has been destroyed
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op3 = seq.new_operation(3);

        {
            operation       op2 = seq.new_operation(2);
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op1.execute();
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op2.execute();
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op3.execute();
        }
        BOOST_CHECK(seq.is_success());
    }

    // Test executing an operation sequence twice, with one of the
    // operations replaced with a new operation with the same id
    // in the second pass
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op3 = seq.new_operation(3);

        // First pass
        {
            operation       op2 = seq.new_operation(2);
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op1.execute();
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op2.execute();
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op3.execute();
            BOOST_CHECK(seq.is_success());
        }

        // Second pass
        seq.reset();
        {
            operation       op2 = seq.new_operation(2);
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op1.execute();
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op2.execute();
            BOOST_CHECK(!seq.is_failure() && !seq.is_success());
            op3.execute();
            BOOST_CHECK(seq.is_success());
        }
    }

    // Test three operations executed in order, the first of which throws
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1, 1);
        operation           op2 = seq.new_operation(2);
        operation           op3 = seq.new_operation(3);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_THROW(op1.execute(), operation_error<1>);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op2.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op3.execute());
        BOOST_CHECK(seq.is_success());
    }

    // Test three operations executed in order, the second of which throws
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op2 = seq.new_operation(2, 2);
        operation           op3 = seq.new_operation(3);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op1.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_THROW(op2.execute(), operation_error<2>);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op3.execute());
        BOOST_CHECK(seq.is_success());
    }

    // Test three operations executed in order, the third of which throws
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1);
        operation           op2 = seq.new_operation(2);
        operation           op3 = seq.new_operation(3, 3);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op1.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op2.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_THROW(op3.execute(), operation_error<3>);
        BOOST_CHECK(seq.is_success());
    }

    // Test three operations executed in order, the first and 
    // third of which throw
    {
        operation_sequence  seq;
        operation           op1 = seq.new_operation(1, 1);
        operation           op2 = seq.new_operation(2);
        operation           op3 = seq.new_operation(3, 3);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_THROW(op1.execute(), operation_error<1>);
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_NO_THROW(op2.execute());
        BOOST_CHECK(!seq.is_failure() && !seq.is_success());
        BOOST_CHECK_THROW(op3.execute(), operation_error<3>);
        BOOST_CHECK(seq.is_success());
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("execute test");
    test->add(BOOST_TEST_CASE(&operation_sequence_test));
    return test;
}

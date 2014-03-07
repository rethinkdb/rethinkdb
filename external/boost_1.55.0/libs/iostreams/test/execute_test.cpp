/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Tests the function templates boost::iostreams::detail::execute_all and
 * boost::iostreams::detail::execute_foreach
 *
 * File:        libs/iostreams/test/execute_test.cpp
 * Date:        Thu Dec 06 13:21:54 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#include <boost/iostreams/detail/execute.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::detail;
using boost::unit_test::test_suite;

// Function object that sets a boolean flag and returns a value
// specified at construction
template<typename Result>
class operation {
public:
    typedef Result result_type;
    explicit operation(Result r, bool& executed) 
        : r_(r), executed_(executed) 
        { }
    Result operator()() const 
    { 
        executed_ = true;
        return r_; 
    }
private:
    operation& operator=(const operation&);
    Result  r_;
    bool&   executed_;  
};

// Specialization for void return
template<>
class operation<void> {
public:
    typedef void result_type;
    explicit operation(bool& executed) : executed_(executed) { }
    void operator()() const { executed_ = true; }
private:
    operation& operator=(const operation&);
    bool& executed_; 
};

// Simple exception class with error code built in to type
template<int Code>
struct error { };

// Function object that sets a boolean flag and throws an exception
template<int Code>
class thrower {
public:
    typedef void result_type;
    explicit thrower(bool& executed) : executed_(executed) { }
    void operator()() const 
    { 
        executed_ = true;
        throw error<Code>(); 
    }
private:
    thrower& operator=(const thrower&);
    bool& executed_; 
};

// Function object for use by foreach_test
class foreach_func {
public:
    typedef void result_type;
    explicit foreach_func(int& count) : count_(count) { }
    void operator()(int x) const
    {
        ++count_;
        switch (x) {
        case 0: throw error<0>();
        case 1: throw error<1>();
        case 2: throw error<2>();
        case 3: throw error<3>();
        case 4: throw error<4>();
        case 5: throw error<5>();
        case 6: throw error<6>();
        case 7: throw error<7>();
        case 8: throw error<8>();
        case 9: throw error<9>();
        default:
            break;
        }
    }
private:
    foreach_func& operator=(const foreach_func&);
    int&  count_; // Number of times operator() has been called
};

void success_test()
{
    // Test returning an int
    {
        bool executed = false;
        BOOST_CHECK(execute_all(operation<int>(9, executed)) == 9);
        BOOST_CHECK(executed);
    }

    // Test returning void
    {
        bool executed = false;
        execute_all(operation<void>(executed));
        BOOST_CHECK(executed);
    }

    // Test returning an int with one cleanup operation
    {
        bool executed = false, cleaned_up = false;
        BOOST_CHECK(
            execute_all(
                operation<int>(9, executed),
                operation<void>(cleaned_up)
            ) == 9
        );
        BOOST_CHECK(executed && cleaned_up);
    }

    // Test returning void with one cleanup operation
    {
        bool executed = false, cleaned_up = false;
        execute_all(
            operation<void>(executed),
            operation<void>(cleaned_up)
        );
        BOOST_CHECK(executed && cleaned_up);
    }

    // Test returning an int with two cleanup operations
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK(
            execute_all(
                operation<int>(9, executed),
                operation<void>(cleaned_up1),
                operation<void>(cleaned_up2)
            ) == 9
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test returning void with two cleanup operations
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        execute_all(
            operation<void>(executed),
            operation<void>(cleaned_up1),
            operation<void>(cleaned_up2)
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test returning an int with three cleanup operations
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK(
            execute_all(
                operation<int>(9, executed),
                operation<void>(cleaned_up1),
                operation<void>(cleaned_up2),
                operation<void>(cleaned_up3)
            ) == 9
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test returning void with three cleanup operations
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        execute_all(
            operation<void>(executed),
            operation<void>(cleaned_up1),
            operation<void>(cleaned_up2),
            operation<void>(cleaned_up3)
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }
}

void operation_throws_test()
{
    // Test primary operation throwing with no cleanup operations
    {
        bool executed = false;
        BOOST_CHECK_THROW(
            execute_all(thrower<0>(executed)),
            error<0>
        );
        BOOST_CHECK(executed);
    }

    // Test primary operation throwing with one cleanup operation
    {
        bool executed = false, cleaned_up = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                operation<void>(cleaned_up)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up);
    }

    // Test primary operation throwing with two cleanup operations
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                operation<void>(cleaned_up1),
                operation<void>(cleaned_up2)
            ), 
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test primary operation throwing with three cleanup operations
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                operation<void>(cleaned_up1),
                operation<void>(cleaned_up2),
                operation<void>(cleaned_up3)
            ), 
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }
}

void cleanup_throws_test()
{
    // Test single cleanup operation that throws
    {
        bool executed = false, cleaned_up = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up)
            ),
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up);
    }

    // Test fist of two cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up1),
                operation<void>(cleaned_up2)
            ),
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test second of two cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                operation<void>(cleaned_up1),
                thrower<2>(cleaned_up2)
            ),
            error<2>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test first of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up1),
                operation<void>(cleaned_up2),
                operation<void>(cleaned_up3)
            ), 
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test second of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                operation<void>(cleaned_up1),
                thrower<2>(cleaned_up2),
                operation<void>(cleaned_up3)
            ), 
            error<2>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test third of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                operation<void>(cleaned_up1),
                operation<void>(cleaned_up2),
                thrower<3>(cleaned_up3)
            ), 
            error<3>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }
}

void multiple_exceptions_test()
{
    // Test primary operation and cleanup operation throwing
    {
        bool executed = false, cleaned_up = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                thrower<1>(cleaned_up)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up);
    }

    // Test primary operation and first of two cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                thrower<1>(cleaned_up1),
                operation<void>(cleaned_up2)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test primary operation and second of two cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                operation<void>(cleaned_up1),
                thrower<2>(cleaned_up2)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test two cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, cleaned_up2 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up1),
                thrower<2>(cleaned_up2)
            ),
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2);
    }

    // Test primary operation and first of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                thrower<1>(cleaned_up1),
                operation<void>(cleaned_up2),
                operation<void>(cleaned_up3)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test primary operation and second of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                operation<void>(cleaned_up1),
                thrower<2>(cleaned_up2),
                operation<void>(cleaned_up3)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test primary operation and third of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                thrower<0>(executed),
                operation<void>(cleaned_up1),
                operation<void>(cleaned_up2),
                thrower<3>(cleaned_up3)
            ),
            error<0>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test first and second of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up1),
                thrower<2>(cleaned_up2),
                operation<void>(cleaned_up3)
            ),
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test first and third of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up1),
                operation<void>(cleaned_up2),
                thrower<3>(cleaned_up3)
            ),
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test second and third of three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                operation<void>(cleaned_up1),
                thrower<2>(cleaned_up2),
                thrower<3>(cleaned_up3)
            ),
            error<2>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }

    // Test three cleanup operations throwing
    {
        bool executed = false, cleaned_up1 = false, 
             cleaned_up2 = false, cleaned_up3 = false;
        BOOST_CHECK_THROW(
            execute_all(
                operation<void>(executed),
                thrower<1>(cleaned_up1),
                thrower<2>(cleaned_up2),
                thrower<3>(cleaned_up3)
            ),
            error<1>
        );
        BOOST_CHECK(executed && cleaned_up1 && cleaned_up2 && cleaned_up3);
    }
}

#define ARRAY_SIZE(ar) (sizeof(ar) / sizeof(ar[0]))

void foreach_test()
{
    // Test case where neither of two operations throws
    {
        int count = 0;
        int seq[] = {-1, -1};
        BOOST_CHECK_NO_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count))
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where first of two operations throws
    {
        int count = 0;
        int seq[] = {0, -1};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<0>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where second of two operations throws
    {
        int count = 0;
        int seq[] = {-1, 1};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<1>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where both of two operations throw
    {
        int count = 0;
        int seq[] = {0, 1};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<0>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where none of three operations throws
    {
        int count = 0;
        int seq[] = {-1, -1, -1};
        BOOST_CHECK_NO_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count))
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where first of three operations throw
    {
        int count = 0;
        int seq[] = {0, -1, -1};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<0>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where second of three operations throw
    {
        int count = 0;
        int seq[] = {-1, 1, -1};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<1>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where third of three operations throw
    {
        int count = 0;
        int seq[] = {-1, -1, 2};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<2>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where first and second of three operations throw
    {
        int count = 0;
        int seq[] = {0, 1, -1};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<0>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where first and third of three operations throw
    {
        int count = 0;
        int seq[] = {0, -1, 2};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<0>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where second and third of three operations throw
    {
        int count = 0;
        int seq[] = {-1, 1, 2};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<1>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }

    // Test case where three of three operations throw
    {
        int count = 0;
        int seq[] = {0, 1, 2};
        BOOST_CHECK_THROW(
            execute_foreach(seq, seq + ARRAY_SIZE(seq), foreach_func(count)),
            error<0>
        );
        BOOST_CHECK(count == ARRAY_SIZE(seq));
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("execute test");
    test->add(BOOST_TEST_CASE(&success_test));
    test->add(BOOST_TEST_CASE(&operation_throws_test));
    test->add(BOOST_TEST_CASE(&cleanup_throws_test));
    test->add(BOOST_TEST_CASE(&multiple_exceptions_test));
    test->add(BOOST_TEST_CASE(&foreach_test));
    return test;
}

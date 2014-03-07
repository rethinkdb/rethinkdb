// what:  simple unit test framework
// who:   developed by Kevlin Henney
// when:  November 2000
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.91
//
// ChangeLog:
//      20 Jan 2001 - Fixed a warning for MSVC (Dave Abrahams)

#ifndef TEST_INCLUDED
#define TEST_INCLUDED

#include <exception>
#include <iostream>
#include <strstream> // for out-of-the-box g++
#include <string>

namespace test // test tuple comprises name and nullary function (object)
{
    template<typename string_type, typename function_type>
    struct test
    {
        string_type   name;
        function_type action;

        static test make(string_type name, function_type action)
        {
            test result; // MSVC aggreggate initializer bugs
            result.name   = name;
            result.action = action;
            return result;
        }
    };
}

namespace test // failure exception used to indicate checked test failures
{
    class failure : public std::exception
    {
    public: // struction (default cases are OK)

        failure(const std::string & why)
          : reason(why)
        {
        }

        // std::~string has no exception-specification (could throw anything),
        // but we need to be compatible with std::~exception's empty one
        // see std::15.4p13 and std::15.4p3
        ~failure() throw()
        {
        }

    public: // usage

        virtual const char * what() const throw()
        {
            return reason.c_str();
        }

    private: // representation

        std::string reason;

    };
}

namespace test // not_implemented exception used to mark unimplemented tests
{
    class not_implemented : public std::exception
    {
    public: // usage (default ctor and dtor are OK)

        virtual const char * what() const throw()
        {
            return "not implemented";
        }

    };
}

namespace test // test utilities
{
    inline void check(bool condition, const std::string & description)
    {
        if(!condition)
        {
            throw failure(description);
        }
    }

    inline void check_true(bool value, const std::string & description)
    {
        check(value, "expected true: " + description);
    }

    inline void check_false(bool value, const std::string & description)
    {
        check(!value, "expected false: " + description);
    }

    template<typename lhs_type, typename rhs_type>
    void check_equal(
        const lhs_type & lhs, const rhs_type & rhs,
        const std::string & description)
    {
        check(lhs == rhs, "expected equal values: " + description);
    }

    template<typename lhs_type, typename rhs_type>
    void check_unequal(
        const lhs_type & lhs, const rhs_type & rhs,
        const std::string & description)
    {
        check(lhs != rhs, "expected unequal values: " + description);
    }

    inline void check_null(const void* ptr, const std::string & description)
    {
        check(!ptr, "expected null pointer: " + description);
    }

    inline void check_non_null(const void* ptr, const std::string & description)
    {
        check(ptr != 0, "expected non-null pointer: " + description);
    }
}

#define TEST_CHECK_THROW(expression, exception, description) \
    try \
    { \
        expression; \
        throw ::test::failure(description); \
    } \
    catch(exception &) \
    { \
    }

namespace test // memory tracking (enabled if test new and delete linked in)
{
    class allocations
    {
    public: // singleton access

        static allocations & instance()
        {
            static allocations singleton;
            return singleton;
        }

    public: // logging

        void clear()
        {
            alloc_count = dealloc_count = 0;
        }

        void allocation()
        {
            ++alloc_count;
        }

        void deallocation()
        {
            ++dealloc_count;
        }

    public: // reporting

        unsigned long allocated() const
        {
            return alloc_count;
        }

        unsigned long deallocated() const
        {
            return dealloc_count;
        }

        bool balanced() const
        {
            return alloc_count == dealloc_count;
        }

    private: // structors (default dtor is fine)
    
        allocations()
          : alloc_count(0), dealloc_count(0)
        {
        }

    private: // prevention

        allocations(const allocations &);
        allocations & operator=(const allocations &);

    private: // state

        unsigned long alloc_count, dealloc_count;

    };
}

namespace test // tester is the driver class for a sequence of tests
{
    template<typename test_iterator>
    class tester
    {
    public: // structors (default destructor is OK)

        tester(test_iterator first_test, test_iterator after_last_test)
          : begin(first_test), end(after_last_test)
        {
        }

    public: // usage

        bool operator()(); // returns true if all tests passed

    private: // representation

        test_iterator begin, end;

    private: // prevention

        tester(const tester &);
        tester &operator=(const tester &);

    };

    template<typename test_iterator>
    bool tester<test_iterator>::operator()()
    {
        using namespace std;

        unsigned long passed = 0, failed = 0, unimplemented = 0;

        for(test_iterator current = begin; current != end; ++current)
        {
            cerr << "[" << current->name << "] " << flush;
            string result = "passed"; // optimistic

            try
            {
                allocations::instance().clear();
                current->action();

                if(!allocations::instance().balanced())
                {
                    unsigned long allocated   = allocations::instance().allocated();
                    unsigned long deallocated = allocations::instance().deallocated();
                    ostrstream report;
                    report << "new/delete ("
                           << allocated << " allocated, "
                           << deallocated << " deallocated)"
                           << ends;
                    const char * text = report.str();
                    report.freeze(false);
                    throw failure(text);
                }

                ++passed;
            }
            catch(const failure & caught)
            {
                (result = "failed: ") += caught.what();
                ++failed;
            }
            catch(const not_implemented &)
            {
                result = "not implemented";
                ++unimplemented;
            }
            catch(const exception & caught)
            {
                (result = "exception: ") += caught.what();
                ++failed;
            }
            catch(...)
            {
                result = "failed with unknown exception";
                ++failed;
            }

            cerr << result << endl;
        }

        cerr << passed + failed << " tests: "
             << passed << " passed, "
             << failed << " failed";

        if(unimplemented)
        {
            cerr << " (" << unimplemented << " not implemented)";
        }

        cerr << endl;

        return failed == 0;
    }
}

#endif

// Copyright Kevlin Henney, 2000. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// what:  simple unit test framework
// who:   developed by Kevlin Henney
// when:  November 2000
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.91

#ifndef TEST_INCLUDED
#define TEST_INCLUDED

#include <boost/config.hpp>
#include <exception>
#include <iostream>
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream> // for out-of-the-box g++ pre-2.95.3
#else
#include <sstream>
#endif
#include <string>

namespace any_tests // test tuple comprises name and nullary function (object)
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

namespace any_tests // failure exception used to indicate checked test failures
{
    class failure : public std::exception
    {
    public: // struction (default cases are OK)

        failure(const std::string & why) throw()
          : reason(why)
        {
        }

              ~failure() throw() {}

    public: // usage

        virtual const char * what() const throw()
        {
            return reason.c_str();
        }

    private: // representation

        std::string reason;

    };
}

namespace any_tests // not_implemented exception used to mark unimplemented tests
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

namespace any_tests // test utilities
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

    inline void check_null(const void * ptr, const std::string & description)
    {
        check(!ptr, "expected null pointer: " + description);
    }

    inline void check_non_null(const void * ptr, const std::string & description)
    {
        check(ptr, "expected non-null pointer: " + description);
    }
}

#define TEST_CHECK_THROW(expression, exception, description) \
    try \
    { \
        expression; \
        throw ::any_tests::failure(description); \
    } \
    catch(exception &) \
    { \
    }

namespace any_tests // memory tracking (enabled if test new and delete linked in)
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

namespace any_tests // tester is the driver class for a sequence of tests
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
    
#if defined(__GNUC__) && defined(__SGI_STL_PORT) && (__GNUC__ < 3)
    // function scope using declarations don't work:
    using namespace std;
#endif

    template<typename test_iterator>
    bool tester<test_iterator>::operator()()
    {
        using std::cerr;
        using std::endl;
        using std::ends;
        using std::exception;
        using std::flush;
        using std::string;

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
#ifdef BOOST_NO_STRINGSTREAM
                    std::ostrstream report;
#else
                    std::ostringstream report;
#endif
                    report << "new/delete ("
                           << allocated << " allocated, "
                           << deallocated << " deallocated)"
                           << ends;
                    const string text = report.str();
#ifdef BOOST_NO_STRINGSTREAM
                    report.freeze(false);
#endif
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

        cerr << (passed + failed) << " tests: "
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

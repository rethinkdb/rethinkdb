// what:  unit tests for variant type boost::any
// who:   contributed by Kevlin Henney
// when:  July 2001, 2013
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

#include <cstdlib>
#include <string>
#include <utility>

#include "boost/any.hpp"
#include "test.hpp"

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
    void test_default_ctor();
    void test_converting_ctor();
    void test_copy_ctor();
    void test_copy_assign();
    void test_converting_assign();
    void test_bad_cast();
    void test_swap();
    void test_null_copying();
    void test_cast_to_reference();
    void test_with_array();
    void test_with_func();
    void test_clear();

    const test_case test_cases[] =
    {
        { "default construction",           test_default_ctor      },
        { "single argument construction",   test_converting_ctor   },
        { "copy construction",              test_copy_ctor         },
        { "copy assignment operator",       test_copy_assign       },
        { "converting assignment operator", test_converting_assign },
        { "failed custom keyword cast",     test_bad_cast          },
        { "swap member function",           test_swap              },
        { "copying operations on a null",   test_null_copying      },
        { "cast to reference types",        test_cast_to_reference },
        { "storing an array inside",        test_with_array        },
        { "implicit cast of returned value",test_with_func         },
        { "clear() methods",                test_clear             }
    };

    const test_case_iterator begin = test_cases;
    const test_case_iterator end =
        test_cases + (sizeof test_cases / sizeof *test_cases);

    

    struct copy_counter
    {

    public:

        copy_counter() {}
        copy_counter(const copy_counter&) { ++count; }
        copy_counter& operator=(const copy_counter&) { ++count; return *this; }
        static int get_count() { return count; }

    private:

        static int count;

    };

    int copy_counter::count = 0;
}

namespace any_tests // test definitions
{
    using namespace boost;

    void test_default_ctor()
    {
        const any value;

        check_true(value.empty(), "empty");
        check_null(any_cast<int>(&value), "any_cast<int>");
        check_equal(value.type(), typeid(void), "type");
    }

    void test_converting_ctor()
    {
        std::string text = "test message";
        any value = text;

        check_false(value.empty(), "empty");
        check_equal(value.type(), typeid(std::string), "type");
        check_null(any_cast<int>(&value), "any_cast<int>");
        check_non_null(any_cast<std::string>(&value), "any_cast<std::string>");
        check_equal(
            any_cast<std::string>(value), text,
            "comparing cast copy against original text");
        check_unequal(
            any_cast<std::string>(&value), &text,
            "comparing address in copy against original text");
    }

    void test_copy_ctor()
    {
        std::string text = "test message";
        any original = text, copy = original;

        check_false(copy.empty(), "empty");
        check_equal(original.type(), copy.type(), "type");
        check_equal(
            any_cast<std::string>(original), any_cast<std::string>(copy),
            "comparing cast copy against original");
        check_equal(
            text, any_cast<std::string>(copy),
            "comparing cast copy against original text");
        check_unequal(
            any_cast<std::string>(&original),
            any_cast<std::string>(&copy),
            "comparing address in copy against original");
    }

    void test_copy_assign()
    {
        std::string text = "test message";
        any original = text, copy;
        any * assign_result = &(copy = original);

        check_false(copy.empty(), "empty");
        check_equal(original.type(), copy.type(), "type");
        check_equal(
            any_cast<std::string>(original), any_cast<std::string>(copy),
            "comparing cast copy against cast original");
        check_equal(
            text, any_cast<std::string>(copy),
            "comparing cast copy against original text");
        check_unequal(
            any_cast<std::string>(&original),
            any_cast<std::string>(&copy),
            "comparing address in copy against original");
        check_equal(assign_result, &copy, "address of assignment result");
    }

    void test_converting_assign()
    {
        std::string text = "test message";
        any value;
        any * assign_result = &(value = text);

        check_false(value.empty(), "type");
        check_equal(value.type(), typeid(std::string), "type");
        check_null(any_cast<int>(&value), "any_cast<int>");
        check_non_null(any_cast<std::string>(&value), "any_cast<std::string>");
        check_equal(
            any_cast<std::string>(value), text,
            "comparing cast copy against original text");
        check_unequal(
            any_cast<std::string>(&value),
            &text,
            "comparing address in copy against original text");
        check_equal(assign_result, &value, "address of assignment result");
    }

    void test_bad_cast()
    {
        std::string text = "test message";
        any value = text;

        TEST_CHECK_THROW(
            any_cast<const char *>(value),
            bad_any_cast,
            "any_cast to incorrect type");
    }

    void test_swap()
    {
        std::string text = "test message";
        any original = text, swapped;
        std::string * original_ptr = any_cast<std::string>(&original);
        any * swap_result = &original.swap(swapped);

        check_true(original.empty(), "empty on original");
        check_false(swapped.empty(), "empty on swapped");
        check_equal(swapped.type(), typeid(std::string), "type");
        check_equal(
            text, any_cast<std::string>(swapped),
            "comparing swapped copy against original text");
        check_non_null(original_ptr, "address in pre-swapped original");
        check_equal(
            original_ptr,
            any_cast<std::string>(&swapped),
            "comparing address in swapped against original");
        check_equal(swap_result, &original, "address of swap result");

        any copy1 = copy_counter();
        any copy2 = copy_counter();
        int count = copy_counter::get_count();
        swap(copy1, copy2);
        check_equal(count, copy_counter::get_count(), "checking that free swap doesn't make any copies.");
    }

    void test_null_copying()
    {
        const any null;
        any copied = null, assigned;
        assigned = null;

        check_true(null.empty(), "empty on null");
        check_true(copied.empty(), "empty on copied");
        check_true(assigned.empty(), "empty on copied");
    }

    void test_cast_to_reference()
    {
        any a(137);
        const any b(a);

        int &                ra    = any_cast<int &>(a);
        int const &          ra_c  = any_cast<int const &>(a);
        int volatile &       ra_v  = any_cast<int volatile &>(a);
        int const volatile & ra_cv = any_cast<int const volatile&>(a);

        check_true(
            &ra == &ra_c && &ra == &ra_v && &ra == &ra_cv,
            "cv references to same obj");

        int const &          rb_c  = any_cast<int const &>(b);
        int const volatile & rb_cv = any_cast<int const volatile &>(b);

        check_true(&rb_c == &rb_cv, "cv references to copied const obj");
        check_true(&ra != &rb_c, "copies hold different objects");

        ++ra;
        int incremented = any_cast<int>(a);
        check_true(incremented == 138, "increment by reference changes value");

        TEST_CHECK_THROW(
            any_cast<char &>(a),
            bad_any_cast,
            "any_cast to incorrect reference type");

        TEST_CHECK_THROW(
            any_cast<const char &>(b),
            bad_any_cast,
            "any_cast to incorrect const reference type");
    }

    void test_with_array()
    {
        any value1("Char array");
        any value2;
        value2 = "Char array";

        check_false(value1.empty(), "type");
        check_false(value2.empty(), "type");

        check_equal(value1.type(), typeid(const char*), "type");
        check_equal(value2.type(), typeid(const char*), "type");
        
        check_non_null(any_cast<const char*>(&value1), "any_cast<const char*>");
        check_non_null(any_cast<const char*>(&value2), "any_cast<const char*>");
    }

    const std::string& returning_string1() 
    {
        static const std::string ret("foo"); 
        return ret;
    }

    std::string returning_string2() 
    {
        static const std::string ret("foo"); 
        return ret;
    }

    void test_with_func()
    {
        std::string s;
        s = any_cast<std::string>(returning_string1());
        s = any_cast<const std::string&>(returning_string1());

        s = any_cast<std::string>(returning_string2());
        s = any_cast<const std::string&>(returning_string2());

#if (!defined(_MSC_VER) || _MSC_VER != 1600) && !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) 
#if !defined(__INTEL_COMPILER) && !defined(__ICL)
        // Intel compiler thinks that it must choose the `any_cast(const any&)` function 
        // instead of the `any_cast(const any&&)`.
        // Bug was not reported because of missing premier support account + annoying 
        // registrations requirements.
        s = any_cast<std::string&&>(returning_string1());
#endif
        s = any_cast<std::string&&>(returning_string2());
#endif
    }

    
    void test_clear()
    {
        std::string text = "test message";
        any value = text;

        check_false(value.empty(), "empty");
        
        value.clear();
        check_true(value.empty(), "non-empty after clear");

        value.clear();
        check_true(value.empty(), "non-empty after second clear");

        value = text;
        check_false(value.empty(), "empty");
        
        value.clear();
        check_true(value.empty(), "non-empty after clear");
    }
}

// Copyright Kevlin Henney, 2000, 2001. All rights reserved.
// Copyright Antony Polukhin, 2013.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

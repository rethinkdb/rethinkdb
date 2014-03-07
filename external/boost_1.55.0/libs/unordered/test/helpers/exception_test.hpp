
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_EXCEPTION_TEST_HEADER)
#define BOOST_UNORDERED_EXCEPTION_TEST_HEADER

#include "./test.hpp"

#include <boost/preprocessor/seq/for_each_product.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/cat.hpp>

#   define UNORDERED_EXCEPTION_TEST_CASE(name, test_func, type)             \
        UNORDERED_AUTO_TEST(name)                                           \
        {                                                                   \
            test_func< type > fixture;                                      \
            ::test::lightweight::exception_safety(                          \
                fixture, BOOST_STRINGIZE(test_func<type>));                 \
        }                                                                   \

#   define UNORDERED_EXCEPTION_TEST_CASE_REPEAT(name, test_func, n, type)   \
        UNORDERED_AUTO_TEST(name)                                           \
        {                                                                   \
            for (unsigned i = 0; i < n; ++i) {                              \
                test_func< type > fixture;                                  \
                ::test::lightweight::exception_safety(                      \
                    fixture, BOOST_STRINGIZE(test_func<type>));             \
            }                                                               \
        }                                                                   \


#    define UNORDERED_EPOINT_IMPL ::test::lightweight::epoint

#define UNORDERED_EXCEPTION_TEST_POSTFIX RUN_TESTS()

#define EXCEPTION_TESTS(test_seq, param_seq)                                \
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(EXCEPTION_TESTS_OP,                       \
        (test_seq)((1))(param_seq))

#define EXCEPTION_TESTS_REPEAT(n, test_seq, param_seq)                      \
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(EXCEPTION_TESTS_OP,                       \
        (test_seq)((n))(param_seq))

#define EXCEPTION_TESTS_OP(r, product)                                      \
    UNORDERED_EXCEPTION_TEST_CASE_REPEAT(                                   \
        BOOST_PP_CAT(BOOST_PP_SEQ_ELEM(0, product),                         \
            BOOST_PP_CAT(_, BOOST_PP_SEQ_ELEM(2, product))                  \
        ),                                                                  \
        BOOST_PP_SEQ_ELEM(0, product),                                      \
        BOOST_PP_SEQ_ELEM(1, product),                                      \
        BOOST_PP_SEQ_ELEM(2, product)                                       \
    )                                                                       \

#define UNORDERED_SCOPE(scope_name)                                         \
    for(::test::scope_guard unordered_test_guard(                           \
            BOOST_STRINGIZE(scope_name));                                   \
        !unordered_test_guard.dismissed();                                  \
        unordered_test_guard.dismiss())                                     \

#define UNORDERED_EPOINT(name)                                              \
    if(::test::exceptions_enabled) {                                        \
        UNORDERED_EPOINT_IMPL(name);                                        \
    }                                                                       \

#define ENABLE_EXCEPTIONS                                                   \
    ::test::exceptions_enable BOOST_PP_CAT(                                 \
        ENABLE_EXCEPTIONS_, __LINE__)(true)                                 \

#define DISABLE_EXCEPTIONS                                                  \
    ::test::exceptions_enable BOOST_PP_CAT(                                 \
        ENABLE_EXCEPTIONS_, __LINE__)(false)                                \

namespace test {
    static char const* scope = "";
    bool exceptions_enabled = false;

    class scope_guard {
        scope_guard& operator=(scope_guard const&);
        scope_guard(scope_guard const&);

        char const* old_scope_;
        char const* scope_;
        bool dismissed_;
    public:
        scope_guard(char const* name)
            : old_scope_(scope),
            scope_(name),
            dismissed_(false)
        {
            scope = scope_;
        }

        ~scope_guard() {
            if(dismissed_) scope = old_scope_;
        }

        void dismiss() {
            dismissed_ = true;
        }

        bool dismissed() const {
            return dismissed_;
        }
    };

    class exceptions_enable
    {
        exceptions_enable& operator=(exceptions_enable const&);
        exceptions_enable(exceptions_enable const&);

        bool old_value_;
    public:
        exceptions_enable(bool enable)
            : old_value_(exceptions_enabled)
        {
            exceptions_enabled = enable;
        }

        ~exceptions_enable()
        {
            exceptions_enabled = old_value_;
        }
    };

    struct exception_base {
        struct data_type {};
        struct strong_type {
            template <class T> void store(T const&) {}
            template <class T> void test(T const&) const {}
        };
        data_type init() const { return data_type(); }
        void check BOOST_PREVENT_MACRO_SUBSTITUTION() const {}
    };

    template <class T, class P1, class P2, class T2>
    inline void call_ignore_extra_parameters(
            void (T::*fn)() const, T2 const& obj,
            P1&, P2&)
    {
        (obj.*fn)();
    }

    template <class T, class P1, class P2, class T2>
    inline void call_ignore_extra_parameters(
            void (T::*fn)(P1&) const, T2 const& obj,
            P1& p1, P2&)
    {
        (obj.*fn)(p1);
    }

    template <class T, class P1, class P2, class T2>
    inline void call_ignore_extra_parameters(
            void (T::*fn)(P1&, P2&) const, T2 const& obj,
            P1& p1, P2& p2)
    {
        (obj.*fn)(p1, p2);
    }

    template <class T>
    T const& constant(T const& x) {
        return x;
    }

    template <class Test>
    class test_runner
    {
        Test const& test_;

        test_runner(test_runner const&);
        test_runner& operator=(test_runner const&);
    public:
        test_runner(Test const& t) : test_(t) {}
        void operator()() const {
            DISABLE_EXCEPTIONS;
            test::scope = "";
            BOOST_DEDUCED_TYPENAME Test::data_type x(test_.init());
            BOOST_DEDUCED_TYPENAME Test::strong_type strong;
            strong.store(x);
            try {
                ENABLE_EXCEPTIONS;
                call_ignore_extra_parameters<
                    Test,
                    BOOST_DEDUCED_TYPENAME Test::data_type,
                    BOOST_DEDUCED_TYPENAME Test::strong_type
                >(&Test::run, test_, x, strong);
            }
            catch(...) {
                call_ignore_extra_parameters<
                    Test,
                    BOOST_DEDUCED_TYPENAME Test::data_type const,
                    BOOST_DEDUCED_TYPENAME Test::strong_type const
                >(&Test::check, test_, constant(x), constant(strong));
                throw;
            }
        }
    };

    // Quick exception testing based on lightweight test

    namespace lightweight {
        static int iteration;
        static int count;

        struct test_exception {
            char const* name;
            test_exception(char const* n) : name(n) {}
        };

        struct test_failure {
        };

        void epoint(char const* name) {
            ++count;
            if(count == iteration) {
                throw test_exception(name);
            }
        }

        template <class Test>
        void exception_safety(Test const& f, char const* /*name*/) {
            test_runner<Test> runner(f);

            iteration = 0;
            bool success = false;
            do {
                ++iteration;
                count = 0;

                try {
                    runner();
                    success = true;
                }
                catch(test_failure) {
                    BOOST_ERROR("test_failure caught.");
                    break;
                }
                catch(test_exception) {
                    continue;
                }
                catch(...) {
                    BOOST_ERROR("Unexpected exception.");
                    break;
                }
            } while(!success);
        }
    }
}

#endif

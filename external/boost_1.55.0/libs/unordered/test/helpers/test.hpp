
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_TEST_HEADER)
#define BOOST_UNORDERED_TEST_TEST_HEADER

#include <boost/detail/lightweight_test.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <iostream>

#define UNORDERED_AUTO_TEST(x)                                              \
    struct BOOST_PP_CAT(x, _type) : public ::test::registered_test_base {   \
        BOOST_PP_CAT(x, _type)()                                            \
            : ::test::registered_test_base(BOOST_PP_STRINGIZE(x))           \
        {                                                                   \
            ::test::test_list::add_test(this);                              \
        }                                                                   \
        void run();                                                         \
    };                                                                      \
    BOOST_PP_CAT(x, _type) x;                                               \
    void BOOST_PP_CAT(x, _type)::run()                                      \

#define RUN_TESTS() int main(int, char**)                                   \
    { ::test::test_list::run_tests(); return boost::report_errors(); }      \

namespace test {
    struct registered_test_base {
        registered_test_base* next;
        char const* name;
        explicit registered_test_base(char const* n) : name(n) {}
        virtual void run() = 0;
        virtual ~registered_test_base() {}
    };

    namespace test_list {
        static inline registered_test_base*& first() {
            static registered_test_base* ptr = 0;
            return ptr;
        }

        static inline registered_test_base*& last() {
            static registered_test_base* ptr = 0;
            return ptr;
        }

        static inline void add_test(registered_test_base* test) {
            if(last()) {
                last()->next = test;
            }
            else {
                first() = test;
            }

            last() = test;
        }

        static inline void run_tests() {
            for(registered_test_base* i = first(); i; i = i->next) {
                std::cout<<"Running "<<i->name<<"\n"<<std::flush;
                i->run();
                std::cerr<<std::flush;
                std::cout<<std::flush;
            }
        }
    }
}

#include <boost/preprocessor/seq/for_each_product.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/to_tuple.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/cat.hpp>

// Run test with every combination of the parameters (a sequence of sequences)
#define UNORDERED_TEST(name, parameters)                                    \
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(UNORDERED_TEST_OP,                        \
        ((name))((1)) parameters)                                           \

#define UNORDERED_TEST_REPEAT(name, n, parameters)                          \
    BOOST_PP_SEQ_FOR_EACH_PRODUCT(UNORDERED_TEST_OP,                        \
        ((name))((n)) parameters)                                           \

#define UNORDERED_TEST_OP(r, product)                                       \
    UNORDERED_TEST_OP2(                                                     \
        BOOST_PP_SEQ_ELEM(0, product),                                      \
        BOOST_PP_SEQ_ELEM(1, product),                                      \
        BOOST_PP_SEQ_TAIL(BOOST_PP_SEQ_TAIL(product)))                      \

#define UNORDERED_TEST_OP2(name, n, params)                                 \
    UNORDERED_AUTO_TEST(                                                    \
        BOOST_PP_SEQ_FOLD_LEFT(UNORDERED_TEST_OP_JOIN, name, params))       \
    {                                                                       \
        for (int i = 0; i < n; ++i)                                         \
            name BOOST_PP_SEQ_TO_TUPLE(params);                             \
    }                                                                       \

#define UNORDERED_TEST_OP_JOIN(s, state, elem)                              \
    BOOST_PP_CAT(state, BOOST_PP_CAT(_, elem))                              \


#endif

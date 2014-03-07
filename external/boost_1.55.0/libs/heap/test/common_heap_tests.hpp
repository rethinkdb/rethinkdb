/*=============================================================================
    Copyright (c) 2010 Tim Blechmann

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#ifndef COMMON_HEAP_TESTS_HPP_INCLUDED
#define COMMON_HEAP_TESTS_HPP_INCLUDED

#include <algorithm>
#include <vector>

#include <boost/concept/assert.hpp>
#include <boost/concept_archetype.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/heap/heap_concepts.hpp>


typedef boost::default_constructible_archetype<
        boost::less_than_comparable_archetype<
        boost::copy_constructible_archetype<
        boost::assignable_archetype<
        > > > > test_value_type;


typedef std::vector<int> test_data;
const int test_size = 32;

struct dummy_run
{
    static void run(void)
    {}
};


test_data make_test_data(int size, int offset = 0, int strive = 1)
{
    test_data ret;

    for (int i = 0; i != size; ++i)
        ret.push_back(i * strive + offset);
    return ret;
}


template <typename pri_queue, typename data_container>
void check_q(pri_queue & q, data_container const & expected)
{
    BOOST_REQUIRE_EQUAL(q.size(), expected.size());

    for (unsigned int i = 0; i != expected.size(); ++i)
    {
        BOOST_REQUIRE_EQUAL(q.size(), expected.size() - i);
        BOOST_REQUIRE_EQUAL(q.top(), expected[expected.size()-1-i]);
        q.pop();
    }

    BOOST_REQUIRE(q.empty());
}


template <typename pri_queue, typename data_container>
void fill_q(pri_queue & q, data_container const & data)
{
    for (unsigned int i = 0; i != data.size(); ++i)
        q.push(data[i]);
}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template <typename pri_queue, typename data_container>
void fill_emplace_q(pri_queue & q, data_container const & data)
{
    for (unsigned int i = 0; i != data.size(); ++i) {
        typename pri_queue::value_type value = data[i];
        q.emplace(std::move(value));
    }
}
#endif

template <typename pri_queue>
void pri_queue_test_sequential_push(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        fill_q(q, data);
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_sequential_reverse_push(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        std::reverse(data.begin(), data.end());
        fill_q(q, data);
        std::reverse(data.begin(), data.end());
        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_emplace(void)
{
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        std::reverse(data.begin(), data.end());
        fill_emplace_q(q, data);
        std::reverse(data.begin(), data.end());
        check_q(q, data);
    }
#endif
}


template <typename pri_queue>
void pri_queue_test_random_push(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);

        test_data shuffled (data);
        std::random_shuffle(shuffled.begin(), shuffled.end());

        fill_q(q, shuffled);

        check_q(q, data);
    }
}

template <typename pri_queue>
void pri_queue_test_copyconstructor(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        fill_q(q, data);

        pri_queue r(q);

        check_q(r, data);
    }
}

template <typename pri_queue>
void pri_queue_test_assignment(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        fill_q(q, data);

        pri_queue r;
        r = q;

        check_q(r, data);
    }
}

template <typename pri_queue>
void pri_queue_test_moveconstructor(void)
{
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    pri_queue q;
    test_data data = make_test_data(test_size);
    fill_q(q, data);

    pri_queue r(std::move(q));

    check_q(r, data);
    BOOST_REQUIRE(q.empty());
#endif
}

template <typename pri_queue>
void pri_queue_test_move_assignment(void)
{
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    pri_queue q;
    test_data data = make_test_data(test_size);
    fill_q(q, data);

    pri_queue r;
    r = std::move(q);

    check_q(r, data);
    BOOST_REQUIRE(q.empty());
#endif
}


template <typename pri_queue>
void pri_queue_test_swap(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        test_data shuffled (data);
        std::random_shuffle(shuffled.begin(), shuffled.end());
        fill_q(q, shuffled);

        pri_queue r;

        q.swap(r);
        check_q(r, data);
        BOOST_REQUIRE(q.empty());
    }
}


template <typename pri_queue>
void pri_queue_test_iterators(void)
{
    for (int i = 0; i != test_size; ++i) {
        test_data data = make_test_data(test_size);
        test_data shuffled (data);
        std::random_shuffle(shuffled.begin(), shuffled.end());
        pri_queue q;
        BOOST_REQUIRE(q.begin() == q.end());
        fill_q(q, shuffled);

        for (unsigned long i = 0; i != data.size(); ++i)
            BOOST_REQUIRE(std::find(q.begin(), q.end(), data[i]) != q.end());

        for (unsigned long i = 0; i != data.size(); ++i)
            BOOST_REQUIRE(std::find(q.begin(), q.end(), data[i] + data.size()) == q.end());

        test_data data_from_queue(q.begin(), q.end());
        std::sort(data_from_queue.begin(), data_from_queue.end());

        BOOST_REQUIRE(data == data_from_queue);

        for (unsigned long i = 0; i != data.size(); ++i) {
            BOOST_REQUIRE_EQUAL((long)std::distance(q.begin(), q.end()), (long)(data.size() - i));
            q.pop();
        }
    }
}

template <typename pri_queue>
void pri_queue_test_ordered_iterators(void)
{
    for (int i = 0; i != test_size; ++i) {
        test_data data = make_test_data(i);
        test_data shuffled (data);
        std::random_shuffle(shuffled.begin(), shuffled.end());
        pri_queue q;
        BOOST_REQUIRE(q.ordered_begin() == q.ordered_end());
        fill_q(q, shuffled);

        test_data data_from_queue(q.ordered_begin(), q.ordered_end());
        std::reverse(data_from_queue.begin(), data_from_queue.end());
        BOOST_REQUIRE(data == data_from_queue);

        for (unsigned long i = 0; i != data.size(); ++i)
            BOOST_REQUIRE(std::find(q.ordered_begin(), q.ordered_end(), data[i]) != q.ordered_end());

        for (unsigned long i = 0; i != data.size(); ++i)
            BOOST_REQUIRE(std::find(q.ordered_begin(), q.ordered_end(), data[i] + data.size()) == q.ordered_end());

        for (unsigned long i = 0; i != data.size(); ++i) {
            BOOST_REQUIRE_EQUAL((long)std::distance(q.begin(), q.end()), (long)(data.size() - i));
            q.pop();
        }
    }
}


template <typename pri_queue>
void pri_queue_test_equality(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q, r;
        test_data data = make_test_data(i);
        fill_q(q, data);
        std::reverse(data.begin(), data.end());
        fill_q(r, data);

        BOOST_REQUIRE(r == q);
    }
}

template <typename pri_queue>
void pri_queue_test_inequality(void)
{
    for (int i = 1; i != test_size; ++i)
    {
        pri_queue q, r;
        test_data data = make_test_data(i);
        fill_q(q, data);
        data[0] = data.back() + 1;
        fill_q(r, data);

        BOOST_REQUIRE(r != q);
    }
}

template <typename pri_queue>
void pri_queue_test_less(void)
{
    for (int i = 1; i != test_size; ++i)
    {
        pri_queue q, r;
        test_data data = make_test_data(i);
        test_data r_data(data);
        r_data.pop_back();

        fill_q(q, data);
        fill_q(r, r_data);

        BOOST_REQUIRE(r < q);
    }

    for (int i = 1; i != test_size; ++i)
    {
        pri_queue q, r;
        test_data data = make_test_data(i);
        test_data r_data(data);
        data.push_back(data.back() + 1);

        fill_q(q, data);
        fill_q(r, r_data);

        BOOST_REQUIRE(r < q);
    }

    for (int i = 1; i != test_size; ++i)
    {
        pri_queue q, r;
        test_data data = make_test_data(i);
        test_data r_data(data);

        data.back() += 1;

        fill_q(q, data);
        fill_q(r, r_data);

        BOOST_REQUIRE(r < q);
    }

    for (int i = 1; i != test_size; ++i)
    {
        pri_queue q, r;
        test_data data = make_test_data(i);
        test_data r_data(data);

        r_data.front() -= 1;

        fill_q(q, data);
        fill_q(r, r_data);

        BOOST_REQUIRE(r < q);
    }

}

template <typename pri_queue>
void pri_queue_test_clear(void)
{
    for (int i = 0; i != test_size; ++i)
    {
        pri_queue q;
        test_data data = make_test_data(i);
        fill_q(q, data);

        q.clear();
        BOOST_REQUIRE(q.size() == 0);
        BOOST_REQUIRE(q.empty());
    }
}


template <typename pri_queue>
void run_concept_check(void)
{
    BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<pri_queue>));
}

template <typename pri_queue>
void run_common_heap_tests(void)
{
    pri_queue_test_sequential_push<pri_queue>();
    pri_queue_test_sequential_reverse_push<pri_queue>();
    pri_queue_test_random_push<pri_queue>();
    pri_queue_test_equality<pri_queue>();
    pri_queue_test_inequality<pri_queue>();
    pri_queue_test_less<pri_queue>();
    pri_queue_test_clear<pri_queue>();

    pri_queue_test_emplace<pri_queue>();
}

template <typename pri_queue>
void run_iterator_heap_tests(void)
{
    pri_queue_test_iterators<pri_queue>();
}


template <typename pri_queue>
void run_copyable_heap_tests(void)
{
    pri_queue_test_copyconstructor <pri_queue>();
    pri_queue_test_assignment<pri_queue>();
    pri_queue_test_swap<pri_queue>();
}


template <typename pri_queue>
void run_moveable_heap_tests(void)
{
    pri_queue_test_moveconstructor <pri_queue>();
    pri_queue_test_move_assignment <pri_queue>();
}


template <typename pri_queue>
void run_reserve_heap_tests(void)
{
    test_data data = make_test_data(test_size);
    pri_queue q;
    q.reserve(6);
    fill_q(q, data);

    check_q(q, data);
}

template <typename pri_queue>
void run_leak_check_test(void)
{
    pri_queue q;
    q.push(boost::shared_ptr<int>(new int(0)));
}


struct less_with_T
{
    typedef int T;
    bool operator()(const int& a, const int& b) const
    {
        return a < b;
    }
};


#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

class thing {
public:
	thing( int a_, int b_, int c_ ) : a(a_), b(b_), c(c_) {}
public:
	int a;
	int b;
	int c;
};

class cmpthings {
public:
	bool operator() ( const thing& lhs, const thing& rhs ) const  {
		return lhs.a > rhs.a;
	}
	bool operator() ( const thing& lhs, const thing& rhs ) {
		return lhs.a > rhs.a;
	}
};

#define RUN_EMPLACE_TEST(HEAP_TYPE)                                     \
    do {                                                                \
        cmpthings ord;                                                  \
        boost::heap::HEAP_TYPE<thing, boost::heap::compare<cmpthings> > vpq(ord); \
        vpq.emplace(5, 6, 7);                                           \
        boost::heap::HEAP_TYPE<thing, boost::heap::compare<cmpthings>, boost::heap::stable<true> > vpq2(ord); \
        vpq2.emplace(5, 6, 7);                                          \
    } while(0);

#else
#define RUN_EMPLACE_TEST(HEAP_TYPE)
#endif


#endif // COMMON_HEAP_TESTS_HPP_INCLUDED

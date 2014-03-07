/*=============================================================================
    Copyright (c) 2004 Angus Leeming

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(CONTAINER_TESTS_HPP)
#define CONTAINER_TESTS_HPP

#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/stl/container/container.hpp>

#include <iostream>
#include <typeinfo>
#include <deque>
#include <list>
#include <map>
#include <vector>
#include <utility>

#ifdef BOOST_MSVC
#pragma warning(disable : 4800)
#endif

using std::cerr;
namespace phx = boost::phoenix;

std::deque<int> const build_deque();
std::list<int> const build_list();
std::map<int, int> const build_map();
std::multimap<int, int> const build_multimap();
std::vector<int> const build_vector();

inline bool
test(bool fail)
{
    BOOST_TEST(!fail);
    return fail;
}

template <typename Container>
void test_assign(Container c)
{
    using phx::arg_names::arg1;

    typename Container::size_type count = 2;
    typename Container::const_iterator first = c.begin();
    typename Container::const_iterator second = first;
    typename Container::value_type value = *first;

    phx::assign(arg1, count, value)(c);

    // iterators may be invalidated!
    first = c.begin();
    second = first;

    std::advance(second, 1);
    if (test(*first != *second)) {
        cerr << "Failed " << typeid(Container).name() << " test_assign 1\n";
        return;
    }
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    Container const const_c = c;
    phx::assign(const_c, count, value);
#endif
}

template <typename Container>
void test_assign2(Container c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::arg_names::arg3;

    Container c2 = c;
    typename Container::const_iterator first = c2.begin();
    typename Container::const_iterator last = c2.end();
    typename Container::size_type size = c2.size();

    c.clear();
    phx::assign(arg1, arg2, arg3)(c, first, last);
    if (test(c.size() != size)) {
        cerr << "Failed " << typeid(Container).name()
       << " test_assign2 1\n"
       << "size == " << c.size() << '\n';
        return;
    }

#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    Container const const_c = c;
    phx::assign(const_c, first, second);
#endif
}

template <typename Container>
void test_at(Container c)
{
    using phx::arg_names::arg1;
    using phx::at;

    typename Container::reference r1 = at(arg1, 0)(c);
    if (test(r1 != c.at(0))) {
        cerr << "Failed " << typeid(Container).name() << " test_at 1\n";
        return;
    }

    typename Container::const_reference r2 = at(arg1, 0)(c);
    if (test(r2 != c.at(0))) {
        cerr << "Failed " << typeid(Container).name() << " test_at 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::reference r3 = at(arg1, 0)(const_c);
#endif

    typename Container::const_reference r4 = at(arg1, 0)(const_c);
    if (test(r4 != c.at(0))) {
        cerr << "Failed " << typeid(Container).name() << " test_at 4\n";
        return;
    }
}

template <typename Container>
void test_back(Container c)
{
    using phx::arg_names::arg1;
    using phx::back;

    typename Container::reference r1 = back(arg1)(c);
    if (test(r1 != c.back())) {
        cerr << "Failed " << typeid(Container).name() << " test_back 1\n";
        return;
    }
    typename Container::const_reference r2 = back(arg1)(c);
    if (test(r2 != c.back())) {
        cerr << "Failed " << typeid(Container).name() << " test_back 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::reference r3 = back(arg1)(const_c);
#endif

    typename Container::const_reference r4 = back(arg1)(const_c);
    if (test(r4 != c.back())) {
        cerr << "Failed " << typeid(Container).name() << " test_back 4\n";
        return;
    }
}

template <typename Container>
void test_begin(Container c)
{
    using phx::arg_names::arg1;
    using phx::begin;

    typename Container::iterator it1 = begin(arg1)(c);
    if (test(it1 != c.begin())) {
        cerr << "Failed " << typeid(Container).name() << " test_begin 1\n";
        return;
    }
    typename Container::const_iterator it2 = begin(arg1)(c);
    if (test(it2 != c.begin())) {
        cerr << "Failed " << typeid(Container).name() << " test_begin 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::iterator it3 = begin(arg1)(const_c);
#endif

    typename Container::const_iterator it4 = begin(arg1)(const_c);
    if (test(it4 != const_c.begin())) {
        cerr << "Failed " << typeid(Container).name() << " test_begin 4\n";
        return;
    }
}

template <typename Container>
void test_capacity(Container c)
{
    using phx::arg_names::arg1;
    using phx::capacity;

    typename Container::size_type s1 = capacity(arg1)(c);
    if (test(s1 != c.capacity())) {
        cerr << "Failed " << typeid(Container).name() << " test_capacity 1\n";
        return;
    }

    Container const const_c = c;
    typename Container::size_type s2 = capacity(arg1)(const_c);
    if (test(s2 != const_c.capacity())) {
        cerr << "Failed " << typeid(Container).name() << " test_capacity 2\n";
        return;
    }
}

template <typename Container>
void test_clear(Container c)
{
    using phx::arg_names::arg1;
    using phx::clear;

    clear(arg1)(c);
    if (test(!c.empty())) {
        cerr << "Failed " << typeid(Container).name() << " test_clear 1\n";
        return;
    }

#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    Container const const_c = c;
    clear(arg1)(const_c);
#endif
}

template <typename Container>
void test_empty(Container c)
{
    using phx::arg_names::arg1;
    using phx::empty;

    typename Container::size_type s1 = empty(arg1)(c);
    if (test(bool(s1) != c.empty())) {
        cerr << "Failed " << typeid(Container).name() << " test_empty 1\n";
        return;
    }

    Container const const_c = c;
    typename Container::size_type s2 = empty(arg1)(const_c);
    if (test(bool(s2) != const_c.empty())) {
        cerr << "Failed " << typeid(Container).name() << " test_empty 2\n";
        return;
    }
}

template <typename Container>
void test_end(Container c)
{
    using phx::arg_names::arg1;
    using phx::end;

    typename Container::iterator it1 = end(arg1)(c);
    if (test(it1 != c.end())) {
        cerr << "Failed " << typeid(Container).name() << " test_end 1\n";
        return;
    }
    typename Container::const_iterator it2 = end(arg1)(c);
    if (test(it2 != c.end())) {
        cerr << "Failed " << typeid(Container).name() << " test_end 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::iterator it3 = end(arg1)(const_c);
#endif

    typename Container::const_iterator it4 = end(arg1)(const_c);
    if (test(it4 != const_c.end())) {
        cerr << "Failed " << typeid(Container).name() << " test_end 4\n";
        return;
    }
}

template <typename Container>
void test_erase(Container c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::arg_names::arg3;
    using phx::erase;

    Container const const_c = c;

    typename Container::size_type size = c.size();
    typename Container::iterator c_begin = c.begin();
    erase(arg1, arg2)(c, c_begin);
    if (test(c.size() + 1 != size)) {
        cerr << "Failed " << typeid(Container).name() << " test_erase 1\n";
        return;
    }

    c_begin = c.begin();
    typename Container::iterator c_end = c.end();
    erase(arg1, arg2, arg3)(c, c_begin, c_end);
    if (test(!c.empty())) {
        cerr << "Failed " << typeid(Container).name() << " test_erase 2\n";
        return;
    }

#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    erase(arg1, const_c.begin())(const_c);
    erase(arg1, const_c.begin(), const_c.end())(const_c);
#endif
}

template <typename Container>
void test_map_erase(Container c)
{
    test_erase(c);
    if (boost::report_errors() != 0)
      return;

    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::erase;

    typename Container::value_type const value = *c.begin();
    typename Container::key_type const key = value.first;
    typename Container::size_type const removed =
        erase(arg1, arg2)(c, key);
    if (test(removed != 1)) {
        cerr << "Failed " << typeid(Container).name() << " test_map_erase 1\n";
        return;
    }
}

template <typename Container>
void test_front(Container c)
{
    using phx::arg_names::arg1;
    using phx::front;

    typename Container::reference r1 = front(arg1)(c);
    if (test(r1 != c.front())) {
        cerr << "Failed " << typeid(Container).name() << " test_front 1\n";
        return;
    }
    typename Container::const_reference r2 = front(arg1)(c);
    if (test(r2 != c.front())) {
        cerr << "Failed " << typeid(Container).name() << " test_front 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::reference r3 = front(arg1)(const_c);
#endif

    typename Container::const_reference r4 = front(arg1)(const_c);
    if (test(r4 != c.front())) {
        cerr << "Failed " << typeid(Container).name() << " test_front 4\n";
        return;
    }
}

template <typename Container>
void test_get_allocator(Container c)
{
    using phx::arg_names::arg1;
    using phx::get_allocator;

    Container const const_c = c;

    typename Container::allocator_type a1 = get_allocator(arg1)(c);
    if (test(a1 != c.get_allocator())) {
        cerr << "Failed " << typeid(Container).name() << " test_get_allocator 1\n";
        return;
    }

    typename Container::allocator_type a2 = get_allocator(arg1)(const_c);
    if (test(a2 != const_c.get_allocator())) {
        cerr << "Failed " << typeid(Container).name() << " test_get_allocator 2\n";
        return;
    }
}

template <typename Container>
void test_insert(Container c)
{
    using phx::arg_names::arg1;
    using phx::insert;

    typename Container::value_type const value = *c.begin();
    typename Container::iterator it = insert(arg1, c.begin(), value)(c);
    if (test(it != c.begin() || *it != *(++it))) {
        cerr << "Failed " << typeid(Container).name() << " test_insert 1\n";
        return;
    }

    typename Container::size_type size = c.size();
    insert(arg1, c.begin(), 3, value)(c);
    if (test(c.size() != size + 3)) {
        cerr << "Failed " << typeid(Container).name() << " test_insert 2\n";
        return;
    }

    Container const const_c = c;
    size = c.size();
    insert(arg1, c.begin(), const_c.begin(), const_c.end())(c);
    if (test(c.size() != 2 * size)) {
        cerr << "Failed " << typeid(Container).name() << " test_insert 3\n";
        return;
    }
}

inline void test_map_insert(std::map<int, int> c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::arg_names::arg3;

    typedef std::map<int, int> Map;

    Map::value_type const value = *c.begin();
    Map::iterator c_begin = c.begin();
    // wrapper for
    // iterator insert(iterator where, const value_type& val);
    Map::iterator it =
        phx::insert(arg1, arg2, arg3)(c, c_begin, value);

    if (test(it != c.begin() /*|| *it != *(++it)*/)) {
        cerr << "Failed " << typeid(Map).name() << " test_map_insert 1\n";
        return;
    }

    // wrapper for
    // pair<iterator, bool> insert(const value_type& val);
    Map::value_type const value2(1400, 2200);
    std::pair<Map::iterator, bool> result =
      phx::insert(arg1, arg2)(c, value2);
    if (test(!result.second)) {
        cerr << "Failed " << typeid(Map).name() << " test_map_insert 2\n";
        return;
    }

    // wrapper for
    // template<class InIt>
    // void insert(InIt first, InIt last);
    Map const const_c = build_map();
    Map::size_type size = c.size();
    phx::insert(arg1, const_c.begin(), const_c.end())(c);
    if (test(c.size() != size + const_c.size())) {
        cerr << "Failed " << typeid(Map).name() << " test_map_insert 3\n";
        return;
    }
}

inline void test_multimap_insert(std::multimap<int, int> c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::arg_names::arg3;

    typedef std::multimap<int, int> Multimap;

    Multimap::value_type const value = *c.begin();
    Multimap::iterator c_begin = c.begin();
    std::size_t old_size = c.size();
    // wrapper for
    // iterator insert(iterator where, const value_type& val);
    Multimap::iterator it =
        phx::insert(arg1, arg2, arg3)(c, c_begin, value);

    if (test(*it != value || c.size() != old_size + 1)) {
        cerr << "Failed " << typeid(Multimap).name()
       << " test_multimap_insert 1\n";
        return;
    }

    // wrapper for
    // iterator insert(const value_type& val);
    Multimap::value_type const value2(1400, 2200);
    it = phx::insert(arg1, arg2)(c, value2);
    if (test(it == c.end())) {
        cerr << "Failed " << typeid(Multimap).name()
       << " test_multimap_insert 2\n";
        return;
    }

    // wrapper for
    // template<class InIt>
    // void insert(InIt first, InIt last);
    Multimap const const_c = build_multimap();
    Multimap::size_type size = c.size();
    phx::insert(arg1, const_c.begin(), const_c.end())(c);
    if (test(c.size() != size + const_c.size())) {
        cerr << "Failed " << typeid(Multimap).name()
       << " test_multimap_insert 3\n";
        return;
    }
}

template <typename Container>
void test_key_comp(Container c)
{
    using phx::arg_names::arg1;
    using phx::key_comp;

    typename Container::key_compare comp = key_comp(arg1)(c);

    Container const const_c = c;
    comp = key_comp(arg1)(const_c);
}

template <typename Container>
void test_max_size(Container c)
{
    using phx::arg_names::arg1;
    using phx::max_size;

    Container const const_c = c;

    typename Container::size_type s1 = max_size(arg1)(c);
    if (test(s1 != c.max_size())) {
        cerr << "Failed " << typeid(Container).name() << " test_max_size 1\n";
        return;
    }

    typename Container::size_type s2 = max_size(arg1)(const_c);
    if (test(s2 != const_c.max_size())) {
        cerr << "Failed " << typeid(Container).name() << " test_max_size 2\n";
        return;
    }
}

template <typename Container>
void test_pop_back(Container c)
{
    using phx::arg_names::arg1;
    using phx::pop_back;

    Container const const_c = c;

    typename Container::size_type size = c.size();

    pop_back(arg1)(c);
    if (test(c.size() + 1 != size)) {
        cerr << "Failed " << typeid(Container).name() << " test_pop_back 1\n";
        return;
    }

#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    pop_back(arg1)(const_c);
#endif
}

template <typename Container>
void test_pop_front(Container c)
{
    using phx::arg_names::arg1;
    using phx::pop_front;

    Container const const_c = c;

    typename Container::size_type size = c.size();

    pop_front(arg1)(c);
    if (test(c.size() + 1 != size)) {
        cerr << "Failed " << typeid(Container).name() << " test_pop_front 1\n";
        return;
    }
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    pop_front(arg1)(const_c);
#endif
}

template <typename Container>
void test_push_back(Container c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::push_back;

    Container const const_c = c;

    typename Container::value_type data = *c.begin();
    typename Container::size_type size = c.size();
    push_back(arg1, arg2)(c, data);
    if (test(c.size() != size + 1)) {
        cerr << "Failed " << typeid(Container).name() << " test_push_back 1\n";
        return;
    }
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    push_back(arg1, arg2)(const_c, data);
#endif
}

template <typename Container>
void test_push_front(Container c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::push_front;

    Container const const_c = c;

    typename Container::value_type data = *c.begin();
    typename Container::size_type size = c.size();
    push_front(arg1, arg2)(c, data);
    if (test(c.size() != size + 1)) {
        cerr << "Failed " << typeid(Container).name() << " test_push_front 1\n";
        return;
    }
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    push_front(arg1, arg2)(const_c, data);
#endif
}

template <typename Container>
void test_rbegin(Container c)
{
    using phx::arg_names::arg1;
    using phx::rbegin;

    typename Container::reverse_iterator it1 = rbegin(arg1)(c);
    typename Container::reverse_iterator it1_test = c.rbegin();
    if (test(it1 != it1_test)) {
        cerr << "Failed " << typeid(Container).name() << " test_rbegin 1\n";
        return;
    }
    typename Container::const_reverse_iterator it2 = rbegin(arg1)(c);
    typename Container::const_reverse_iterator it2_test = c.rbegin();
    if (test(it2 != it2_test)) {
        cerr << "Failed " << typeid(Container).name() << " test_rbegin 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::reverse_iterator it3 = rbegin(arg1)(const_c);
#endif

    typename Container::const_reverse_iterator it4 = rbegin(arg1)(const_c);
    it2_test = const_c.rbegin();
    if (test(it4 != it2_test)) {
        cerr << "Failed " << typeid(Container).name() << " test_rbegin 4\n";
        return;
    }
}

template <typename Container>
void test_rend(Container c)
{
    using phx::arg_names::arg1;
    using phx::rend;

    typename Container::reverse_iterator it1 = rend(arg1)(c);
    typename Container::reverse_iterator it1_test = c.rend();
    if (test(it1 != it1_test)) {
        cerr << "Failed " << typeid(Container).name() << " test_rend 1\n";
        return;
    }
    typename Container::const_reverse_iterator it2 = rend(arg1)(c);
    typename Container::const_reverse_iterator it2_test = c.rend();
    if (test(it2 != it2_test)) {
        cerr << "Failed " << typeid(Container).name() << " test_rend 2\n";
        return;
    }

    Container const const_c = c;
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    // Should not --- does not, Yay! --- compile.
    typename Container::reverse_iterator it3 = rend(arg1)(const_c);
#endif

    typename Container::const_reverse_iterator it4 = rend(arg1)(const_c);
    it2_test = const_c.rend();
    if (test(it4 != it2_test)) {
        cerr << "Failed " << typeid(Container).name() << " test_rend 4\n";
        return;
    }
}

template <typename Container>
void test_reserve(Container c)
{
    using phx::arg_names::arg1;
    using phx::reserve;

    Container const const_c = c;

    typename Container::size_type count = 2 * c.size();
    reserve(arg1, count)(c);
    if (test(c.capacity() < count)) {
        cerr << "Failed " << typeid(Container).name() << " test_reserve 1\n";
        return;
    }
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    reserve(arg1, count)(const_c)(const_c);
#endif
}

template <typename Container>
void test_resize(Container c)
{
    using phx::arg_names::arg1;
    using phx::resize;

    Container const const_c = c;

    typename Container::size_type new_size = 2 * c.size();
    resize(arg1, new_size)(c);
    if (test(c.size() != new_size)) {
        cerr << "Failed " << typeid(Container).name() << " test_resize 1\n";
        return;
    }

    new_size = 2 * c.size();
    typename Container::value_type value = *c.begin();
    resize(arg1, new_size, value)(c);
    if (test(c.size() != new_size)) {
        cerr << "Failed " << typeid(Container).name() << " test_resize 2\n";
        return;
    }
#if defined(BOOST_PHOENIX_COMPILE_FAIL_TEST)
    new_size = 2 * const_c.size();
    resize(arg1, new_size)(const_c);

    new_size = 2 * const_c.size();
    resize(arg1, new_size, value)(const_c);
#endif
}

template <typename Container>
void test_size(Container c)
{
    using phx::arg_names::arg1;
    using phx::size;

    Container const const_c = c;

    typename Container::size_type s1 = size(arg1)(c);
    if (test(s1 != c.size())) {
        cerr << "Failed " << typeid(Container).name() << " test_size 1\n";
        return;
    }

    typename Container::size_type s2 = size(arg1)(const_c);
    if (test(s2 != const_c.size())) {
        cerr << "Failed " << typeid(Container).name() << " test_size 2\n";
        return;
    }
}

template <typename Container>
void test_splice(Container c)
{
    using phx::arg_names::arg1;
    using phx::arg_names::arg2;
    using phx::arg_names::arg3;
    using phx::arg_names::arg4;
    using phx::arg_names::arg5;
    using phx::splice;

    typename Container::iterator c_end;
    typename Container::iterator c2_begin;
    typename Container::iterator c2_end;
    typename Container::size_type size = c.size();

    Container const copy = c;
    Container const copy2 = build_list();
    Container c2 = copy2;

    size = c.size();
    c_end = c.end();
    splice(arg1, arg2, arg3)(c, c_end, c2);
    if (test(c.size() != 2 * size)) {
        cerr << "Failed " << typeid(Container).name() << " test_splice 1\n";
        return;
    }

    c = copy;
    c_end = c.end();
    c2 = copy2;
    c2_begin = c2.begin();
    size = c.size() + 1;
    splice(arg1, arg2, arg3, arg4)(c, c_end, c2, c2_begin);
    if (test(c.size() != size)) {
        cerr << "Failed " << typeid(Container).name() << " test_splice 2\n";
        return;
    }

    c = copy;
    c_end = c.end();
    c2 = copy2;
    c2_begin = c2.begin();
    c2_end = c2.end();
    size = c.size() + c2.size();
    /*
    splice(arg1, arg2, arg3, arg4, arg5)(c, c_end, c2, c2_begin, c2_end);
    if (test(c.size() != size)) {
        cerr << "Failed " << typeid(Container).name() << " test_splice 3\n";
        return;
    }
    */
}

template <typename Container>
void test_value_comp(Container c)
{
    using phx::arg_names::arg1;
    using phx::value_comp;

    typename Container::value_compare comp = value_comp(arg1)(c);

    Container const const_c = c;
    comp = value_comp(arg1)(const_c);
}

#endif

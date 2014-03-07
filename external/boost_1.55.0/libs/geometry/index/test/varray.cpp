// Boost.Geometry.Index varray
// Unit Test

// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2012-2013 Andrew Hundt.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/impl/execution_monitor.ipp>

// TODO: Disable parts of the unit test that should not run when BOOST_NO_EXCEPTIONS
// if exceptions are enabled there must be a user defined throw_exception function
#ifdef BOOST_NO_EXCEPTIONS
namespace boost {
    void throw_exception(std::exception const & e){}; // user defined
} // namespace boost
#endif // BOOST_NO_EXCEPTIONS

#include <vector>
#include <list>

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
#include <boost/container/vector.hpp>
#include <boost/container/stable_vector.hpp>
using namespace boost::container;
#endif

#include "varray_test.hpp"

using namespace boost::geometry::index::detail;

template <typename T, size_t N>
void test_ctor_ndc()
{
    varray<T, N> s;
    BOOST_CHECK_EQUAL(s.size(), 0u);
    BOOST_CHECK(s.capacity() == N);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW( s.at(0), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
}

template <typename T, size_t N>
void test_ctor_nc(size_t n)
{
    varray<T, N> s(n);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
    if ( 1 < n )
    {
        T val10(10);
        s[0] = val10;
        BOOST_CHECK(T(10) == s[0]);
        BOOST_CHECK(T(10) == s.at(0));
        T val20(20);
        s.at(1) = val20;
        BOOST_CHECK(T(20) == s[1]);
        BOOST_CHECK(T(20) == s.at(1));
    }
}

template <typename T, size_t N>
void test_ctor_nd(size_t n, T const& v)
{
    varray<T, N> s(n, v);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
    if ( 1 < n )
    {
        BOOST_CHECK(v == s[0]);
        BOOST_CHECK(v == s.at(0));
        BOOST_CHECK(v == s[1]);
        BOOST_CHECK(v == s.at(1));
        s[0] = T(10);
        BOOST_CHECK(T(10) == s[0]);
        BOOST_CHECK(T(10) == s.at(0));
        s.at(1) = T(20);
        BOOST_CHECK(T(20) == s[1]);
        BOOST_CHECK(T(20) == s.at(1));
    }
}

template <typename T, size_t N>
void test_resize_nc(size_t n)
{
    varray<T, N> s;

    s.resize(n);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
    if ( 1 < n )
    {
        T val10(10);
        s[0] = val10;
        BOOST_CHECK(T(10) == s[0]);
        BOOST_CHECK(T(10) == s.at(0));
        T val20(20);
        s.at(1) = val20;
        BOOST_CHECK(T(20) == s[1]);
        BOOST_CHECK(T(20) == s.at(1));
    }
}

template <typename T, size_t N>
void test_resize_nd(size_t n, T const& v)
{
    varray<T, N> s;

    s.resize(n, v);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
    if ( 1 < n )
    {
        BOOST_CHECK(v == s[0]);
        BOOST_CHECK(v == s.at(0));
        BOOST_CHECK(v == s[1]);
        BOOST_CHECK(v == s.at(1));
        s[0] = T(10);
        BOOST_CHECK(T(10) == s[0]);
        BOOST_CHECK(T(10) == s.at(0));
        s.at(1) = T(20);
        BOOST_CHECK(T(20) == s[1]);
        BOOST_CHECK(T(20) == s.at(1));
    }
}

template <typename T, size_t N>
void test_push_back_nd()
{
    varray<T, N> s;

    BOOST_CHECK(s.size() == 0);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW( s.at(0), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS

    for ( size_t i = 0 ; i < N ; ++i )
    {
        T t(i);
        s.push_back(t);
        BOOST_CHECK(s.size() == i + 1);
#ifndef BOOST_NO_EXCEPTIONS
        BOOST_CHECK_THROW( s.at(i + 1), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
        BOOST_CHECK(T(i) == s.at(i));
        BOOST_CHECK(T(i) == s[i]);
        BOOST_CHECK(T(i) == s.back());
        BOOST_CHECK(T(0) == s.front());
        BOOST_CHECK(T(i) == *(s.data() + i));
    }
}

template <typename T, size_t N>
void test_pop_back_nd()
{
    varray<T, N> s;

    for ( size_t i = 0 ; i < N ; ++i )
    {
        T t(i);
        s.push_back(t);
    }

    for ( size_t i = N ; i > 1 ; --i )
    {
        s.pop_back();
        BOOST_CHECK(s.size() == i - 1);
#ifndef BOOST_NO_EXCEPTIONS
        BOOST_CHECK_THROW( s.at(i - 1), std::out_of_range );
#endif // BOOST_NO_EXCEPTIONS
        BOOST_CHECK(T(i - 2) == s.at(i - 2));
        BOOST_CHECK(T(i - 2) == s[i - 2]);
        BOOST_CHECK(T(i - 2) == s.back());
        BOOST_CHECK(T(0) == s.front());
    }
}

template <typename It1, typename It2>
void test_compare_ranges(It1 first1, It1 last1, It2 first2, It2 last2)
{
    BOOST_CHECK(std::distance(first1, last1) == std::distance(first2, last2));
    for ( ; first1 != last1 && first2 != last2 ; ++first1, ++first2 )
        BOOST_CHECK(*first1 == *first2);
}

template <typename T, size_t N, typename C>
void test_copy_and_assign(C const& c)
{
    {
        varray<T, N> s(c.begin(), c.end());
        BOOST_CHECK(s.size() == c.size());
        test_compare_ranges(s.begin(), s.end(), c.begin(), c.end());
    }
    {
        varray<T, N> s;
        BOOST_CHECK(0 == s.size());
        s.assign(c.begin(), c.end());
        BOOST_CHECK(s.size() == c.size());
        test_compare_ranges(s.begin(), s.end(), c.begin(), c.end());
    }
}

template <typename T, size_t N>
void test_copy_and_assign_nd(T const& val)
{
    varray<T, N> s;
    std::vector<T> v;
    std::list<T> l;

    for ( size_t i = 0 ; i < N ; ++i )
    {
        T t(i);
        s.push_back(t);
        v.push_back(t);
        l.push_back(t);
    }
    // copy ctor
    {
        varray<T, N> s1(s);
        BOOST_CHECK(s.size() == s1.size());
        test_compare_ranges(s.begin(), s.end(), s1.begin(), s1.end());
    }
    // copy assignment
    {
        varray<T, N> s1;
        BOOST_CHECK(0 == s1.size());
        s1 = s;
        BOOST_CHECK(s.size() == s1.size());
        test_compare_ranges(s.begin(), s.end(), s1.begin(), s1.end());
    }

    // ctor(Iter, Iter) and assign(Iter, Iter)
    test_copy_and_assign<T, N>(s);
    test_copy_and_assign<T, N>(v);
    test_copy_and_assign<T, N>(l);

    // assign(N, V)
    {
        varray<T, N> s1(s);
        test_compare_ranges(s.begin(), s.end(), s1.begin(), s1.end());
        std::vector<T> a(N, val);
        s1.assign(N, val);
        test_compare_ranges(a.begin(), a.end(), s1.begin(), s1.end());
    }

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    stable_vector<T> bsv(s.begin(), s.end());
    vector<T> bv(s.begin(), s.end());
    test_copy_and_assign<T, N>(bsv);
    test_copy_and_assign<T, N>(bv);
#endif
}

template <typename T, size_t N>
void test_iterators_nd()
{
    varray<T, N> s;
    std::vector<T> v;

    for ( size_t i = 0 ; i < N ; ++i )
    {
        s.push_back(T(i));
        v.push_back(T(i));
    }

    test_compare_ranges(s.begin(), s.end(), v.begin(), v.end());
    test_compare_ranges(s.rbegin(), s.rend(), v.rbegin(), v.rend());

    s.assign(v.rbegin(), v.rend());

    test_compare_ranges(s.begin(), s.end(), v.rbegin(), v.rend());
    test_compare_ranges(s.rbegin(), s.rend(), v.begin(), v.end());
}

template <typename T, size_t N>
void test_erase_nd()
{
    varray<T, N> s;
    typedef typename varray<T, N>::iterator It;

    for ( size_t i = 0 ; i < N ; ++i )
        s.push_back(T(i));

    // erase(pos)
    {
        for ( size_t i = 0 ; i < N ; ++i )
        {
            varray<T, N> s1(s);
            It it = s1.erase(s1.begin() + i);
            BOOST_CHECK(s1.begin() + i == it);
            BOOST_CHECK(s1.size() == N - 1);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = i+1 ; j < N ; ++j )
                BOOST_CHECK(s1[j-1] == T(j));
        }
    }
    // erase(first, last)
    {
        size_t n = N/3;
        for ( size_t i = 0 ; i <= N ; ++i )
        {
            varray<T, N> s1(s);
            size_t removed = i + n < N ? n : N - i;
            It it = s1.erase(s1.begin() + i, s1.begin() + i + removed);
            BOOST_CHECK(s1.begin() + i == it);
            BOOST_CHECK(s1.size() == N - removed);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = i+n ; j < N ; ++j )
                BOOST_CHECK(s1[j-n] == T(j));
        }
    }
}

template <typename T, size_t N, typename SV, typename C>
void test_insert(SV const& s, C const& c)
{
    size_t h = N/2;
    size_t n = size_t(h/1.5f);

    for ( size_t i = 0 ; i <= h ; ++i )
    {
        varray<T, N> s1(s);

        typename C::const_iterator it = c.begin();
        std::advance(it, n);
        typename varray<T, N>::iterator
            it1 = s1.insert(s1.begin() + i, c.begin(), it);

        BOOST_CHECK(s1.begin() + i == it1);
        BOOST_CHECK(s1.size() == h+n);
        for ( size_t j = 0 ; j < i ; ++j )
            BOOST_CHECK(s1[j] == T(j));
        for ( size_t j = 0 ; j < n ; ++j )
            BOOST_CHECK(s1[j+i] == T(100 + j));
        for ( size_t j = 0 ; j < h-i ; ++j )
            BOOST_CHECK(s1[j+i+n] == T(j+i));
    }
}

template <typename T, size_t N>
void test_insert_nd(T const& val)
{
    size_t h = N/2;

    varray<T, N> s, ss;
    std::vector<T> v;
    std::list<T> l;

    typedef typename varray<T, N>::iterator It;

    for ( size_t i = 0 ; i < h ; ++i )
    {
        s.push_back(T(i));
        ss.push_back(T(100 + i));
        v.push_back(T(100 + i));
        l.push_back(T(100 + i));
    }

    // insert(pos, val)
    {
        for ( size_t i = 0 ; i <= h ; ++i )
        {
            varray<T, N> s1(s);
            It it = s1.insert(s1.begin() + i, val);
            BOOST_CHECK(s1.begin() + i == it);
            BOOST_CHECK(s1.size() == h+1);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            BOOST_CHECK(s1[i] == val);
            for ( size_t j = 0 ; j < h-i ; ++j )
                BOOST_CHECK(s1[j+i+1] == T(j+i));
        }
    }
    // insert(pos, n, val)
    {
        size_t n = size_t(h/1.5f);
        for ( size_t i = 0 ; i <= h ; ++i )
        {
            varray<T, N> s1(s);
            It it = s1.insert(s1.begin() + i, n, val);
            BOOST_CHECK(s1.begin() + i == it);
            BOOST_CHECK(s1.size() == h+n);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = 0 ; j < n ; ++j )
                BOOST_CHECK(s1[j+i] == val);
            for ( size_t j = 0 ; j < h-i ; ++j )
                BOOST_CHECK(s1[j+i+n] == T(j+i));
        }
    }
    // insert(pos, first, last)
    test_insert<T, N>(s, ss);
    test_insert<T, N>(s, v);
    test_insert<T, N>(s, l);

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    stable_vector<T> bsv(ss.begin(), ss.end());
    vector<T> bv(ss.begin(), ss.end());
    test_insert<T, N>(s, bv);
    test_insert<T, N>(s, bsv);
#endif
}

template <typename T>
void test_capacity_0_nd()
{
    varray<T, 10> v(5u, T(0));

    //varray<T, 0, bad_alloc_strategy<T> > s;
    varray<T, 0> s;
    BOOST_CHECK(s.size() == 0);
    BOOST_CHECK(s.capacity() == 0);
#ifndef BOOST_NO_EXCEPTIONS
    BOOST_CHECK_THROW(s.at(0), std::out_of_range);
    //BOOST_CHECK_THROW(s.resize(5u, T(0)), std::bad_alloc);
    //BOOST_CHECK_THROW(s.push_back(T(0)), std::bad_alloc);
    //BOOST_CHECK_THROW(s.insert(s.end(), T(0)), std::bad_alloc);
    //BOOST_CHECK_THROW(s.insert(s.end(), 5u, T(0)), std::bad_alloc);
    //BOOST_CHECK_THROW(s.insert(s.end(), v.begin(), v.end()), std::bad_alloc);
    //BOOST_CHECK_THROW(s.assign(v.begin(), v.end()), std::bad_alloc);
    //BOOST_CHECK_THROW(s.assign(5u, T(0)), std::bad_alloc);
    //try{
    //    varray<T, 0, bad_alloc_strategy<T> > s2(v.begin(), v.end());
    //    BOOST_CHECK(false);
    //}catch(std::bad_alloc &){}
    //try{
    //    varray<T, 0, bad_alloc_strategy<T> > s1(5u, T(0));
    //    BOOST_CHECK(false);
    //}catch(std::bad_alloc &){}
#endif // BOOST_NO_EXCEPTIONS
}

template <typename T, size_t N>
void test_exceptions_nd()
{
    varray<T, N> v(N, T(0));
    //varray<T, N/2, bad_alloc_strategy<T> > s(N/2, T(0));
    varray<T, N/2> s(N/2, T(0));

#ifndef BOOST_NO_EXCEPTIONS
    /*BOOST_CHECK_THROW(s.resize(N, T(0)), std::bad_alloc);
    BOOST_CHECK_THROW(s.push_back(T(0)), std::bad_alloc);
    BOOST_CHECK_THROW(s.insert(s.end(), T(0)), std::bad_alloc);
    BOOST_CHECK_THROW(s.insert(s.end(), N, T(0)), std::bad_alloc);
    BOOST_CHECK_THROW(s.insert(s.end(), v.begin(), v.end()), std::bad_alloc);
    BOOST_CHECK_THROW(s.assign(v.begin(), v.end()), std::bad_alloc);
    BOOST_CHECK_THROW(s.assign(N, T(0)), std::bad_alloc);
    try{
        container_detail::varray<T, N/2, bad_alloc_strategy<T> > s2(v.begin(), v.end());
        BOOST_CHECK(false);
    }catch(std::bad_alloc &){}
    try{
        container_detail::varray<T, N/2, bad_alloc_strategy<T> > s1(N, T(0));
        BOOST_CHECK(false);
    }catch(std::bad_alloc &){}*/
#endif // BOOST_NO_EXCEPTIONS
}

template <typename T, size_t N>
void test_swap_and_move_nd()
{
    {
        varray<T, N> v1, v2, v3, v4;
        varray<T, N> s1, s2;
        //varray<T, N, bad_alloc_strategy<T> > s4;
        varray<T, N> s4;

        for (size_t i = 0 ; i < N ; ++i )
        {
            v1.push_back(T(i));
            v2.push_back(T(i));
            v3.push_back(T(i));
            v4.push_back(T(i));
        }
        for (size_t i = 0 ; i < N/2 ; ++i )
        {
            s1.push_back(T(100 + i));
            s2.push_back(T(100 + i));
            s4.push_back(T(100 + i));
        }

        s1.swap(v1);
        s2 = boost::move(v2);
        varray<T, N> s3(boost::move(v3));
        s4.swap(v4);

        BOOST_CHECK(v1.size() == N/2);
        BOOST_CHECK(s1.size() == N);
        BOOST_CHECK(v2.size() == N); // objects aren't destroyed
        BOOST_CHECK(s2.size() == N);
        BOOST_CHECK(v3.size() == N); // objects aren't destroyed
        BOOST_CHECK(s3.size() == N);
        BOOST_CHECK(v4.size() == N/2);
        BOOST_CHECK(s4.size() == N);
        for (size_t i = 0 ; i < N/2 ; ++i )
        {
            BOOST_CHECK(v1[i] == T(100 + i));
            BOOST_CHECK(v4[i] == T(100 + i));
        }
        for (size_t i = 0 ; i < N ; ++i )
        {
            BOOST_CHECK(s1[i] == T(i));
            BOOST_CHECK(s2[i] == T(i));
            BOOST_CHECK(s3[i] == T(i));
            BOOST_CHECK(s4[i] == T(i));
        }
    }
    {
        varray<T, N> v1, v2, v3;
        varray<T, N/2> s1, s2;

        for (size_t i = 0 ; i < N/2 ; ++i )
        {
            v1.push_back(T(i));
            v2.push_back(T(i));
            v3.push_back(T(i));
        }
        for (size_t i = 0 ; i < N/3 ; ++i )
        {
            s1.push_back(T(100 + i));
            s2.push_back(T(100 + i));
        }

        s1.swap(v1);
        s2 = boost::move(v2);
        varray<T, N/2> s3(boost::move(v3));

        BOOST_CHECK(v1.size() == N/3);
        BOOST_CHECK(s1.size() == N/2);
        BOOST_CHECK(v2.size() == N/2); // objects aren't destroyed
        BOOST_CHECK(s2.size() == N/2);
        BOOST_CHECK(v3.size() == N/2); // objects aren't destroyed
        BOOST_CHECK(s3.size() == N/2);
        for (size_t i = 0 ; i < N/3 ; ++i )
            BOOST_CHECK(v1[i] == T(100 + i));
        for (size_t i = 0 ; i < N/2 ; ++i )
        {
            BOOST_CHECK(s1[i] == T(i));
            BOOST_CHECK(s2[i] == T(i));
            BOOST_CHECK(s3[i] == T(i));
        }
    }
    {
        varray<T, N> v(N, T(0));
        //varray<T, N/2, bad_alloc_strategy<T> > s(N/2, T(1));
        varray<T, N/2> s(N/2, T(1));
#ifndef BOOST_NO_EXCEPTIONS
        //BOOST_CHECK_THROW(s.swap(v), std::bad_alloc);
        //v.resize(N, T(0));
        //BOOST_CHECK_THROW(s = boost::move(v), std::bad_alloc);
        //v.resize(N, T(0));
        //try {
        //    varray<T, N/2, bad_alloc_strategy<T> > s2(boost::move(v));
        //    BOOST_CHECK(false);
        //} catch (std::bad_alloc &) {}
#endif // BOOST_NO_EXCEPTIONS
    }
}

template <typename T, size_t N>
void test_emplace_0p()
{
    //emplace_back()
    {
        //varray<T, N, bad_alloc_strategy<T> > v;
        varray<T, N> v;

        for (int i = 0 ; i < int(N) ; ++i )
            v.emplace_back();
        BOOST_CHECK(v.size() == N);
#ifndef BOOST_NO_EXCEPTIONS
        //BOOST_CHECK_THROW(v.emplace_back(), std::bad_alloc);
#endif
    }
}

template <typename T, size_t N>
void test_emplace_2p()
{
    //emplace_back(pos, int, int)
    {
        //varray<T, N, bad_alloc_strategy<T> > v;
        varray<T, N> v;

        for (int i = 0 ; i < int(N) ; ++i )
            v.emplace_back(i, 100 + i);
        BOOST_CHECK(v.size() == N);
#ifndef BOOST_NO_EXCEPTIONS
        //BOOST_CHECK_THROW(v.emplace_back(N, 100 + N), std::bad_alloc);
#endif
        BOOST_CHECK(v.size() == N);
        for (int i = 0 ; i < int(N) ; ++i )
            BOOST_CHECK(v[i] == T(i, 100 + i));
    }

    // emplace(pos, int, int)
    {
        //typedef typename varray<T, N, bad_alloc_strategy<T> >::iterator It;
        typedef typename varray<T, N>::iterator It;

        int h = N / 2;

        //varray<T, N, bad_alloc_strategy<T> > v;
        varray<T, N> v;
        for ( int i = 0 ; i < h ; ++i )
            v.emplace_back(i, 100 + i);

        for ( int i = 0 ; i <= h ; ++i )
        {
            //varray<T, N, bad_alloc_strategy<T> > vv(v);
            varray<T, N> vv(v);
            It it = vv.emplace(vv.begin() + i, i+100, i+200);
            BOOST_CHECK(vv.begin() + i == it);
            BOOST_CHECK(vv.size() == size_t(h+1));
            for ( int j = 0 ; j < i ; ++j )
                BOOST_CHECK(vv[j] == T(j, j+100));
            BOOST_CHECK(vv[i] == T(i+100, i+200));
            for ( int j = 0 ; j < h-i ; ++j )
                BOOST_CHECK(vv[j+i+1] == T(j+i, j+i+100));
        }
    }
}

template <typename T, size_t N>
void test_sv_elem(T const& t)
{
    //typedef varray<T, N, bad_alloc_strategy<T> > V;
    typedef varray<T, N> V;

    //varray<V, N, bad_alloc_strategy<V> > v;
    varray<V, N> v;

    v.push_back(V(N/2, t));
    V vvv(N/2, t);
    v.push_back(boost::move(vvv));
    v.insert(v.begin(), V(N/2, t));
    v.insert(v.end(), V(N/2, t));
    v.emplace_back(N/2, t);
}

int test_main(int, char* [])
{
    BOOST_CHECK(counting_value::count() == 0);

    test_ctor_ndc<size_t, 10>();
    test_ctor_ndc<value_ndc, 10>();
    test_ctor_ndc<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_ctor_ndc<shptr_value, 10>();
    test_ctor_ndc<copy_movable, 10>();

    test_ctor_nc<size_t, 10>(5);
    test_ctor_nc<value_nc, 10>(5);
    test_ctor_nc<counting_value, 10>(5);
    BOOST_CHECK(counting_value::count() == 0);
    test_ctor_nc<shptr_value, 10>(5);
    test_ctor_nc<copy_movable, 10>(5);

    test_ctor_nd<size_t, 10>(5, 1);
    test_ctor_nd<value_nd, 10>(5, value_nd(1));
    test_ctor_nd<counting_value, 10>(5, counting_value(1));
    BOOST_CHECK(counting_value::count() == 0);
    test_ctor_nd<shptr_value, 10>(5, shptr_value(1));
    test_ctor_nd<copy_movable, 10>(5, produce());

    test_resize_nc<size_t, 10>(5);
    test_resize_nc<value_nc, 10>(5);
    test_resize_nc<counting_value, 10>(5);
    BOOST_CHECK(counting_value::count() == 0);
    test_resize_nc<shptr_value, 10>(5);
    test_resize_nc<copy_movable, 10>(5);

    test_resize_nd<size_t, 10>(5, 1);
    test_resize_nd<value_nd, 10>(5, value_nd(1));
    test_resize_nd<counting_value, 10>(5, counting_value(1));
    BOOST_CHECK(counting_value::count() == 0);
    test_resize_nd<shptr_value, 10>(5, shptr_value(1));
    test_resize_nd<copy_movable, 10>(5, produce());

    test_push_back_nd<size_t, 10>();
    test_push_back_nd<value_nd, 10>();
    test_push_back_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_push_back_nd<shptr_value, 10>();
    test_push_back_nd<copy_movable, 10>();

    test_pop_back_nd<size_t, 10>();
    test_pop_back_nd<value_nd, 10>();
    test_pop_back_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_pop_back_nd<shptr_value, 10>();
    test_pop_back_nd<copy_movable, 10>();

    test_copy_and_assign_nd<size_t, 10>(1);
    test_copy_and_assign_nd<value_nd, 10>(value_nd(1));
    test_copy_and_assign_nd<counting_value, 10>(counting_value(1));
    BOOST_CHECK(counting_value::count() == 0);
    test_copy_and_assign_nd<shptr_value, 10>(shptr_value(1));
    test_copy_and_assign_nd<copy_movable, 10>(produce());

    test_iterators_nd<size_t, 10>();
    test_iterators_nd<value_nd, 10>();
    test_iterators_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_iterators_nd<shptr_value, 10>();
    test_iterators_nd<copy_movable, 10>();

    test_erase_nd<size_t, 10>();
    test_erase_nd<value_nd, 10>();
    test_erase_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_erase_nd<shptr_value, 10>();
    test_erase_nd<copy_movable, 10>();

    test_insert_nd<size_t, 10>(50);
    test_insert_nd<value_nd, 10>(value_nd(50));
    test_insert_nd<counting_value, 10>(counting_value(50));
    BOOST_CHECK(counting_value::count() == 0);
    test_insert_nd<shptr_value, 10>(shptr_value(50));
    test_insert_nd<copy_movable, 10>(produce());

    test_capacity_0_nd<size_t>();
    test_capacity_0_nd<value_nd>();
    test_capacity_0_nd<counting_value>();
    BOOST_CHECK(counting_value::count() == 0);
    test_capacity_0_nd<shptr_value>();
    test_capacity_0_nd<copy_movable>();

    test_exceptions_nd<size_t, 10>();
    test_exceptions_nd<value_nd, 10>();
    test_exceptions_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_exceptions_nd<shptr_value, 10>();
    test_exceptions_nd<copy_movable, 10>();

    test_swap_and_move_nd<size_t, 10>();
    test_swap_and_move_nd<value_nd, 10>();
    test_swap_and_move_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);
    test_swap_and_move_nd<shptr_value, 10>();
    test_swap_and_move_nd<copy_movable, 10>();

    test_emplace_0p<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_emplace_2p<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_sv_elem<size_t, 10>(50);
    test_sv_elem<value_nd, 10>(value_nd(50));
    test_sv_elem<counting_value, 10>(counting_value(50));
    BOOST_CHECK(counting_value::count() == 0);
    test_sv_elem<shptr_value, 10>(shptr_value(50));
    test_sv_elem<copy_movable, 10>(copy_movable(50));

    return 0;
}

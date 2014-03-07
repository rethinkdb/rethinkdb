// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/included/test_exec_monitor.hpp>
#include <boost/test/impl/execution_monitor.ipp>

#include <boost/geometry/index/detail/varray.hpp>

using namespace boost::geometry::index::detail;

class value_ndc
{
public:
    explicit value_ndc(int a) : aa(a) {}
    ~value_ndc() {}
    bool operator==(value_ndc const& v) const { return aa == v.aa; }
private:
    value_ndc(value_ndc const&) {}
    value_ndc & operator=(value_ndc const&) { return *this; }
    int aa;
};

class value_nd
{
public:
    explicit value_nd(int a) : aa(a) {}
    ~value_nd() {}
    bool operator==(value_nd const& v) const { return aa == v.aa; }
private:
    int aa;
};

class value_nc
{
public:
    explicit value_nc(int a = 0) : aa(a) {}
    ~value_nc() {}
    bool operator==(value_nc const& v) const { return aa == v.aa; }
private:
    value_nc(value_nc const&) {}
    value_nc & operator=(value_ndc const&) { return *this; }
    int aa;
};

class counting_value
{
public:
    explicit counting_value(int a = 0) : aa(a) { ++c(); }
    counting_value(counting_value const& v) : aa(v.aa) { ++c(); }
    counting_value & operator=(counting_value const& v) { aa = v.aa; return *this; }
    ~counting_value() { --c(); }
    bool operator==(counting_value const& v) const { return aa == v.aa; }
    static size_t count() { return c(); }
private:
    static size_t & c() { static size_t co = 0; return co; }
    int aa;
};

template <typename T, size_t N>
void test_ctor_ndc()
{
    varray<T, N> s;
    BOOST_CHECK(s.size() == 0);
    BOOST_CHECK(s.capacity() == N);
    BOOST_CHECK_THROW( s.at(0), std::out_of_range );
}

template <typename T, size_t N>
void test_ctor_nc(size_t n)
{
    varray<T, N> s(n);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
    if ( !boost::has_trivial_constructor<T>::value )
    {
        for ( size_t i = 0 ; i < n ; ++i )
            BOOST_CHECK(T() == s[i]);
    }
}

template <typename T, size_t N>
void test_ctor_nd(size_t n, T const& v)
{
    varray<T, N> s(n, v);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
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
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );

    if ( !boost::has_trivial_constructor<T>::value )
    {
        for ( size_t i = 0 ; i < n ; ++i )
            BOOST_CHECK(T() == s[i]);
    }
}

template <typename T, size_t N>
void test_resize_nd(size_t n, T const& v)
{
    varray<T, N> s;

    s.resize(n, v);
    BOOST_CHECK(s.size() == n);
    BOOST_CHECK(s.capacity() == N);
    BOOST_CHECK_THROW( s.at(n), std::out_of_range );
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
    BOOST_CHECK_THROW( s.at(0), std::out_of_range );

    for ( size_t i = 0 ; i < N ; ++i )
    {
        s.push_back(T(i));
        BOOST_CHECK(s.size() == i + 1);
        BOOST_CHECK_THROW( s.at(i + 1), std::out_of_range );
        BOOST_CHECK(T(i) == s.at(i));
        BOOST_CHECK(T(i) == s[i]);
        BOOST_CHECK(T(i) == s.back());
        BOOST_CHECK(T(0) == s.front());
    }
}

template <typename T, size_t N>
void test_pop_back_nd()
{
    varray<T, N> s;

    for ( size_t i = 0 ; i < N ; ++i )
        s.push_back(T(i));    

    for ( size_t i = N ; i > 1 ; --i )
    {
        s.pop_back();
        BOOST_CHECK(s.size() == i - 1);
        BOOST_CHECK_THROW( s.at(i - 1), std::out_of_range );
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

template <typename T, size_t N>
void test_copy_and_assign_nd(T const& val)
{
    varray<T, N> s;
    std::vector<T> v;
    std::list<T> l;

    for ( size_t i = 0 ; i < N ; ++i )
    {
        s.push_back(T(i));
        v.push_back(T(i));
        l.push_back(T(i));
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
    // ctor(Iter, Iter)
    {
        varray<T, N> s1(s.begin(), s.end());
        BOOST_CHECK(s.size() == s1.size());
        test_compare_ranges(s.begin(), s.end(), s1.begin(), s1.end());
    }
    {
        varray<T, N> s1(v.begin(), v.end());
        BOOST_CHECK(v.size() == s1.size());
        test_compare_ranges(v.begin(), v.end(), s1.begin(), s1.end());
    }
    {
        varray<T, N> s1(l.begin(), l.end());
        BOOST_CHECK(l.size() == s1.size());
        test_compare_ranges(l.begin(), l.end(), s1.begin(), s1.end());
    }
    // assign(Iter, Iter)
    {
        varray<T, N> s1;
        BOOST_CHECK(0 == s1.size());
        s1.assign(s.begin(), s.end());
        BOOST_CHECK(s.size() == s1.size());
        test_compare_ranges(s.begin(), s.end(), s1.begin(), s1.end());
    }
    {
        varray<T, N> s1;
        BOOST_CHECK(0 == s1.size());
        s1.assign(v.begin(), v.end());
        BOOST_CHECK(v.size() == s1.size());
        test_compare_ranges(v.begin(), v.end(), s1.begin(), s1.end());
    }
    {
        varray<T, N> s1;
        BOOST_CHECK(0 == s1.size());
        s1.assign(l.begin(), l.end());
        BOOST_CHECK(l.size() == s1.size());
        test_compare_ranges(l.begin(), l.end(), s1.begin(), s1.end());
    }
    // assign(N, V)
    {
        varray<T, N> s1(s);
        test_compare_ranges(s.begin(), s.end(), s1.begin(), s1.end());
        std::vector<T> a(N, val);
        s1.assign(N, val);
        test_compare_ranges(a.begin(), a.end(), s1.begin(), s1.end());
    }
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
    
    for ( size_t i = 0 ; i < N ; ++i )
        s.push_back(T(i));

    // erase(pos)
    {
        for ( size_t i = 0 ; i < N ; ++i )
        {
            varray<T, N> s1(s);
            s1.erase(s1.begin() + i);
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
            s1.erase(s1.begin() + i, s1.begin() + i + removed);
            BOOST_CHECK(s1.size() == N - removed);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = i+n ; j < N ; ++j )
                BOOST_CHECK(s1[j-n] == T(j));
        }        
    }
}

template <typename T, size_t N>
void test_insert_nd(T const& val)
{
    size_t h = N/2;

    varray<T, N> s, ss;
    std::vector<T> v;
    std::list<T> l;

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
            s1.insert(s1.begin() + i, val);
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
            s1.insert(s1.begin() + i, n, val);
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
    {
        size_t n = size_t(h/1.5f);
        for ( size_t i = 0 ; i <= h ; ++i )
        {
            varray<T, N> s1(s);
            s1.insert(s1.begin() + i, ss.begin(), ss.begin() + n);
            BOOST_CHECK(s1.size() == h+n);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = 0 ; j < n ; ++j )
                BOOST_CHECK(s1[j+i] == T(100 + j));
            for ( size_t j = 0 ; j < h-i ; ++j )
                BOOST_CHECK(s1[j+i+n] == T(j+i));
        }        
    }
    {
        size_t n = size_t(h/1.5f);
        for ( size_t i = 0 ; i <= h ; ++i )
        {
            varray<T, N> s1(s);
            s1.insert(s1.begin() + i, v.begin(), v.begin() + n);
            BOOST_CHECK(s1.size() == h+n);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = 0 ; j < n ; ++j )
                BOOST_CHECK(s1[j+i] == T(100 + j));
            for ( size_t j = 0 ; j < h-i ; ++j )
                BOOST_CHECK(s1[j+i+n] == T(j+i));
        }        
    }
    {
        size_t n = size_t(h/1.5f);
        for ( size_t i = 0 ; i <= h ; ++i )
        {
            varray<T, N> s1(s);
            typename std::list<T>::iterator it = l.begin();
            std::advance(it, n);
            s1.insert(s1.begin() + i, l.begin(), it);
            BOOST_CHECK(s1.size() == h+n);
            for ( size_t j = 0 ; j < i ; ++j )
                BOOST_CHECK(s1[j] == T(j));
            for ( size_t j = 0 ; j < n ; ++j )
                BOOST_CHECK(s1[j+i] == T(100 + j));
            for ( size_t j = 0 ; j < h-i ; ++j )
                BOOST_CHECK(s1[j+i+n] == T(j+i));
        }        
    }
}

int test_main(int, char* [])
{
    BOOST_CHECK(counting_value::count() == 0);

    test_ctor_ndc<int, 10>();
    test_ctor_ndc<value_ndc, 10>();
    test_ctor_ndc<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_ctor_nc<int, 10>(5);
    test_ctor_nc<value_nc, 10>(5);
    test_ctor_nc<counting_value, 10>(5);
    BOOST_CHECK(counting_value::count() == 0);

    test_ctor_nd<int, 10>(5, 1);
    test_ctor_nd<value_nd, 10>(5, value_nd(1));
    test_ctor_nd<counting_value, 10>(5, counting_value(1));
    BOOST_CHECK(counting_value::count() == 0);

    test_resize_nc<int, 10>(5);
    test_resize_nc<value_nc, 10>(5);
    test_resize_nc<counting_value, 10>(5);
    BOOST_CHECK(counting_value::count() == 0);

    test_resize_nd<int, 10>(5, 1);
    test_resize_nd<value_nd, 10>(5, value_nd(1));
    test_resize_nd<counting_value, 10>(5, counting_value(1));
    BOOST_CHECK(counting_value::count() == 0);

    test_push_back_nd<int, 10>();
    test_push_back_nd<value_nd, 10>();
    test_push_back_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_pop_back_nd<int, 10>();
    test_pop_back_nd<value_nd, 10>();
    test_pop_back_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_copy_and_assign_nd<int, 10>(1);
    test_copy_and_assign_nd<value_nd, 10>(value_nd(1));
    test_copy_and_assign_nd<counting_value, 10>(counting_value(1));
    BOOST_CHECK(counting_value::count() == 0);

    test_iterators_nd<int, 10>();
    test_iterators_nd<value_nd, 10>();
    test_iterators_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_erase_nd<int, 10>();
    test_erase_nd<value_nd, 10>();
    test_erase_nd<counting_value, 10>();
    BOOST_CHECK(counting_value::count() == 0);

    test_insert_nd<int, 10>(50);
    test_insert_nd<value_nd, 10>(value_nd(50));
    test_insert_nd<counting_value, 10>(counting_value(50));
    BOOST_CHECK(counting_value::count() == 0);

    return 0;
}

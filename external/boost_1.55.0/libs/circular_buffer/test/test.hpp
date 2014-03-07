// Header file for the test of the circular buffer library.

// Copyright (c) 2003-2008 Jan Gaspar

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_CIRCULAR_BUFFER_TEST_HPP)
#define BOOST_CIRCULAR_BUFFER_TEST_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1200
    #pragma once
#endif

#define BOOST_CB_TEST

#include <boost/circular_buffer.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/iterator.hpp>
#include <iterator>
#include <numeric>
#include <vector>
#if !defined(BOOST_NO_EXCEPTIONS)
    #include <exception>
#endif

// Integer (substitute for int) - more appropriate for testing
class MyInteger {
private:
    int* m_pValue;
    static int ms_exception_trigger;
    void check_exception() {
        if (ms_exception_trigger > 0) {
            if (--ms_exception_trigger == 0) {
                delete m_pValue;
                m_pValue = 0;
#if !defined(BOOST_NO_EXCEPTIONS)
                throw std::exception();
#endif
            }
        }
    }
public:
    MyInteger() : m_pValue(new int(0)) { check_exception(); }
    MyInteger(int i) : m_pValue(new int(i)) { check_exception(); }
    MyInteger(const MyInteger& src) : m_pValue(new int(src)) { check_exception(); }
    ~MyInteger() { delete m_pValue; }
    MyInteger& operator = (const MyInteger& src) {
        if (this == &src)
            return *this;
        check_exception();
        delete m_pValue;
        m_pValue = new int(src);
        return *this;
    }
    operator int () const { return *m_pValue; }
    static void set_exception_trigger(int n) { ms_exception_trigger = n; }
};

// default constructible class
class MyDefaultConstructible
{
public:
    MyDefaultConstructible() : m_n(1) {}
    MyDefaultConstructible(int n) : m_n(n) {}
    int m_n;
};

// class counting instances of self
class InstanceCounter {
public:
    InstanceCounter() { increment(); }
    InstanceCounter(const InstanceCounter& y) { y.increment(); }
    ~InstanceCounter() { decrement(); }
    static int count() { return ms_count; }
private:
    void increment() const { ++ms_count; }
    void decrement() const { --ms_count; }
    static int ms_count;
};

// dummy class suitable for iterator referencing test
class Dummy {
public:
    enum DummyEnum {
        eVar,
        eFnc,
        eConst,
        eVirtual
    };
    Dummy() : m_n(eVar) {}
    DummyEnum fnc() { return eFnc; }
    DummyEnum const_fnc() const { return eConst; }
    virtual DummyEnum virtual_fnc() { return eVirtual; }
    DummyEnum m_n;
};

// simulator of an input iterator
struct MyInputIterator
: boost::iterator<std::input_iterator_tag, int, ptrdiff_t, int*, int&> {
    typedef std::vector<int>::iterator vector_iterator;
    typedef int value_type;
    typedef int* pointer;
    typedef int& reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    explicit MyInputIterator(const vector_iterator& it) : m_it(it) {}
    MyInputIterator& operator = (const MyInputIterator& it) {
        if (this == &it)
            return *this;
        m_it = it.m_it;
        return *this;
    }
    reference operator * () const { return *m_it; }
    pointer operator -> () const { return &(operator*()); }
    MyInputIterator& operator ++ () {
        ++m_it;
        return *this;
    }
    MyInputIterator operator ++ (int) {
        MyInputIterator tmp = *this;
        ++*this;
        return tmp;
    }
    bool operator == (const MyInputIterator& it) const { return m_it == it.m_it; }
    bool operator != (const MyInputIterator& it) const { return m_it != it.m_it; }
private:
    vector_iterator m_it;
};

#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_MSVC_STD_ITERATOR)

inline std::input_iterator_tag iterator_category(const MyInputIterator&) {
    return std::input_iterator_tag();
}
inline int* value_type(const MyInputIterator&) { return 0; }
inline ptrdiff_t* distance_type(const MyInputIterator&) { return 0; }

#endif // #if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_MSVC_STD_ITERATOR)

using boost::unit_test::test_suite;
using namespace boost;
using namespace std;

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_TEST_HPP)

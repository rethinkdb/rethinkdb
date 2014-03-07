// Boost.Geometry.Index varray
// Unit Test

// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2012-2013 Andrew Hundt.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_TEST_VARRAY_TEST_HPP
#define BOOST_GEOMETRY_INDEX_TEST_VARRAY_TEST_HPP

#include <boost/geometry/index/detail/varray.hpp>

#include <boost/shared_ptr.hpp>
#include "movable.hpp"

class value_ndc
{
public:
    explicit value_ndc(size_t a) : aa(a) {}
    ~value_ndc() {}
    bool operator==(value_ndc const& v) const { return aa == v.aa; }
    bool operator<(value_ndc const& v) const { return aa < v.aa; }
private:
    value_ndc(value_ndc const&) {}
    value_ndc & operator=(value_ndc const&) { return *this; }
    size_t aa;
};

class value_nd
{
public:
    explicit value_nd(size_t a) : aa(a) {}
    ~value_nd() {}
    bool operator==(value_nd const& v) const { return aa == v.aa; }
    bool operator<(value_nd const& v) const { return aa < v.aa; }
private:
    size_t aa;
};

class value_nc
{
public:
    explicit value_nc(size_t a = 0) : aa(a) {}
    ~value_nc() {}
    bool operator==(value_nc const& v) const { return aa == v.aa; }
    bool operator<(value_nc const& v) const { return aa < v.aa; }
private:
    value_nc(value_nc const&) {}
    value_nc & operator=(value_ndc const&) { return *this; }
    size_t aa;
};

class counting_value
{
    BOOST_COPYABLE_AND_MOVABLE(counting_value)

public:
    explicit counting_value(size_t a = 0, size_t b = 0) : aa(a), bb(b) { ++c(); }
    counting_value(counting_value const& v) : aa(v.aa), bb(v.bb) { ++c(); }
    counting_value(BOOST_RV_REF(counting_value) p) : aa(p.aa), bb(p.bb) { p.aa = 0; p.bb = 0; ++c(); }                      // Move constructor
    counting_value& operator=(BOOST_RV_REF(counting_value) p) { aa = p.aa; p.aa = 0; bb = p.bb; p.bb = 0; return *this; }   // Move assignment
    counting_value& operator=(BOOST_COPY_ASSIGN_REF(counting_value) p) { aa = p.aa; bb = p.bb; return *this; }              // Copy assignment
    ~counting_value() { --c(); }
    bool operator==(counting_value const& v) const { return aa == v.aa && bb == v.bb; }
    bool operator<(counting_value const& v) const { return aa < v.aa || ( aa == v.aa && bb < v.bb ); }
    static size_t count() { return c(); }

private:
    static size_t & c() { static size_t co = 0; return co; }
    size_t aa, bb;
};

namespace boost {

template <>
struct has_nothrow_move<counting_value>
{
    static const bool value = true;
};

}

class shptr_value
{
    typedef boost::shared_ptr<size_t> Ptr;
public:
    explicit shptr_value(size_t a = 0) : m_ptr(new size_t(a)) {}
    bool operator==(shptr_value const& v) const { return *m_ptr == *(v.m_ptr); }
    bool operator<(shptr_value const& v) const { return *m_ptr < *(v.m_ptr); }
private:
    boost::shared_ptr<size_t> m_ptr;
};

#endif // BOOST_GEOMETRY_INDEX_TEST_VARRAY_TEST_HPP

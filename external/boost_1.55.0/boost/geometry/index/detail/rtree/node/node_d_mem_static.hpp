// Boost.Geometry Index
//
// R-tree nodes based on runtime-polymorphism, storing static-size containers
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_STATIC_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_STATIC_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct dynamic_internal_node<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
    : public dynamic_node<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
{
    typedef typename Allocators::leaf_allocator_type::template rebind<
        rtree::ptr_pair<Box, typename Allocators::node_pointer>
    >::other elements_allocator_type;

    typedef detail::varray<
        rtree::ptr_pair<Box, typename Allocators::node_pointer>,
        Parameters::max_elements + 1
    > elements_type;

    template <typename Alloc>
    inline dynamic_internal_node(Alloc const&) {}

    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_static_tag, false> & v) { v(*this); }
    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_static_tag, true> & v) const { v(*this); }

    elements_type elements;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct dynamic_leaf<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
    : public dynamic_node<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
{
    typedef typename Allocators::leaf_allocator_type::template rebind<
        Value
    >::other elements_allocator_type;

    typedef detail::varray<
        Value,
        Parameters::max_elements + 1
    > elements_type;

    template <typename Alloc>
    inline dynamic_leaf(Alloc const&) {}

    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_static_tag, false> & v) { v(*this); }
    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_static_tag, true> & v) const { v(*this); }

    elements_type elements;
};

// nodes traits

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct node<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
{
    typedef dynamic_node<Value, Parameters, Box, Allocators, node_d_mem_static_tag> type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct internal_node<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
{
    typedef dynamic_internal_node<Value, Parameters, Box, Allocators, node_d_mem_static_tag> type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct leaf<Value, Parameters, Box, Allocators, node_d_mem_static_tag>
{
    typedef dynamic_leaf<Value, Parameters, Box, Allocators, node_d_mem_static_tag> type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators, bool IsVisitableConst>
struct visitor<Value, Parameters, Box, Allocators, node_d_mem_static_tag, IsVisitableConst>
{
    typedef dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_static_tag, IsVisitableConst> type;
};

// elements derived type
template <typename OldValue, size_t N, typename NewValue>
struct container_from_elements_type<detail::varray<OldValue, N>, NewValue>
{
    typedef detail::varray<NewValue, N> type;
};

// allocators

template <typename Allocator, typename Value, typename Parameters, typename Box>
class allocators<Allocator, Value, Parameters, Box, node_d_mem_static_tag>
    : public Allocator::template rebind<
        typename internal_node<
            Value, Parameters, Box,
            allocators<Allocator, Value, Parameters, Box, node_d_mem_static_tag>,
            node_d_mem_static_tag
        >::type
    >::other
    , public Allocator::template rebind<
        typename leaf<
            Value, Parameters, Box,
            allocators<Allocator, Value, Parameters, Box, node_d_mem_static_tag>,
            node_d_mem_static_tag
        >::type
    >::other
{
    typedef typename Allocator::template rebind<
        Value
    >::other value_allocator_type;

public:
    typedef Allocator allocator_type;

    typedef Value value_type;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef typename value_allocator_type::size_type size_type;
    typedef typename value_allocator_type::difference_type difference_type;
    typedef typename value_allocator_type::pointer pointer;
    typedef typename value_allocator_type::const_pointer const_pointer;

    typedef typename Allocator::template rebind<
        typename node<Value, Parameters, Box, allocators, node_d_mem_static_tag>::type
    >::other::pointer node_pointer;

//    typedef typename Allocator::template rebind<
//        typename internal_node<Value, Parameters, Box, allocators, node_d_mem_static_tag>::type
//    >::other::pointer internal_node_pointer;

    typedef typename Allocator::template rebind<
        typename internal_node<Value, Parameters, Box, allocators, node_d_mem_static_tag>::type
    >::other internal_node_allocator_type;

    typedef typename Allocator::template rebind<
        typename leaf<Value, Parameters, Box, allocators, node_d_mem_static_tag>::type
    >::other leaf_allocator_type;

    inline allocators()
        : internal_node_allocator_type()
        , leaf_allocator_type()
    {}

    template <typename Alloc>
    inline explicit allocators(Alloc const& alloc)
        : internal_node_allocator_type(alloc)
        , leaf_allocator_type(alloc)
    {}

    inline allocators(BOOST_FWD_REF(allocators) a)
        : internal_node_allocator_type(boost::move(a.internal_node_allocator()))
        , leaf_allocator_type(boost::move(a.leaf_allocator()))
    {}

    inline allocators & operator=(BOOST_FWD_REF(allocators) a)
    {
        internal_node_allocator() = ::boost::move(a.internal_node_allocator());
        leaf_allocator() = ::boost::move(a.leaf_allocator());
        return *this;
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    inline allocators & operator=(allocators const& a)
    {
        internal_node_allocator() = a.internal_node_allocator();
        leaf_allocator() = a.leaf_allocator();
        return *this;
    }
#endif

    void swap(allocators & a)
    {
        boost::swap(internal_node_allocator(), a.internal_node_allocator());
        boost::swap(leaf_allocator(), a.leaf_allocator());
    }

    bool operator==(allocators const& a) const { return leaf_allocator() == a.leaf_allocator(); }
    template <typename Alloc>
    bool operator==(Alloc const& a) const { return leaf_allocator() == leaf_allocator_type(a); }

    Allocator allocator() const { return Allocator(leaf_allocator()); }

    internal_node_allocator_type & internal_node_allocator() { return *this; }
    internal_node_allocator_type const& internal_node_allocator() const { return *this; }
    leaf_allocator_type & leaf_allocator() { return *this; }
    leaf_allocator_type const& leaf_allocator() const{ return *this; }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_STATIC_HPP

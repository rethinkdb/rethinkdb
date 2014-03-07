// Boost.Geometry Index
//
// R-tree nodes based on Boost.Variant, storing static-size containers
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_STATIC_VARIANT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_STATIC_VARIANT_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

// nodes default types

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct static_internal_node<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
{
    typedef typename Allocators::node_allocator_type::template rebind<
        rtree::ptr_pair<Box, typename Allocators::node_pointer>
    >::other elements_allocator_type;

    typedef detail::varray<
        rtree::ptr_pair<Box, typename Allocators::node_pointer>,
        Parameters::max_elements + 1
    > elements_type;

    template <typename Alloc>
    inline static_internal_node(Alloc const&) {}

    elements_type elements;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct static_leaf<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
{
    typedef typename Allocators::node_allocator_type::template rebind<
        Value
    >::other elements_allocator_type;

    typedef detail::varray<
        Value,
        Parameters::max_elements + 1
    > elements_type;

    template <typename Alloc>
    inline static_leaf(Alloc const&) {}

    elements_type elements;
};

// nodes traits

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct node<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
{
    typedef boost::variant<
        static_leaf<Value, Parameters, Box, Allocators, node_s_mem_static_tag>,
        static_internal_node<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
    > type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct internal_node<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
{
    typedef static_internal_node<Value, Parameters, Box, Allocators, node_s_mem_static_tag> type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct leaf<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
{
    typedef static_leaf<Value, Parameters, Box, Allocators, node_s_mem_static_tag> type;
};

// visitor traits

template <typename Value, typename Parameters, typename Box, typename Allocators, bool IsVisitableConst>
struct visitor<Value, Parameters, Box, Allocators, node_s_mem_static_tag, IsVisitableConst>
{
    typedef static_visitor<> type;
};

// allocators

template <typename Allocator, typename Value, typename Parameters, typename Box>
struct allocators<Allocator, Value, Parameters, Box, node_s_mem_static_tag>
    : public Allocator::template rebind<
        typename node<Value, Parameters, Box, allocators<Allocator, Value, Parameters, Box, node_s_mem_static_tag>, node_s_mem_static_tag>::type
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
        typename node<Value, Parameters, Box, allocators, node_s_mem_static_tag>::type
    >::other::pointer node_pointer;

//    typedef typename Allocator::template rebind<
//        typename internal_node<Value, Parameters, Box, allocators, node_s_mem_static_tag>::type
//    >::other::pointer internal_node_pointer;

    typedef typename Allocator::template rebind<
        typename node<Value, Parameters, Box, allocators, node_s_mem_static_tag>::type
    >::other node_allocator_type;

    inline allocators()
        : node_allocator_type()
    {}

    template <typename Alloc>
    inline explicit allocators(Alloc const& alloc)
        : node_allocator_type(alloc)
    {}

    inline allocators(BOOST_FWD_REF(allocators) a)
        : node_allocator_type(boost::move(a.node_allocator()))
    {}

    inline allocators & operator=(BOOST_FWD_REF(allocators) a)
    {
        node_allocator() = boost::move(a.node_allocator());
        return *this;
    }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    inline allocators & operator=(allocators const& a)
    {
        node_allocator() = a.node_allocator();
        return *this;
    }
#endif

    void swap(allocators & a)
    {
        boost::swap(node_allocator(), a.node_allocator());
    }

    bool operator==(allocators const& a) const { return node_allocator() == a.node_allocator(); }
    template <typename Alloc>
    bool operator==(Alloc const& a) const { return node_allocator() == node_allocator_type(a); }

    Allocator allocator() const { return Allocator(node_allocator()); }

    node_allocator_type & node_allocator() { return *this; }
    node_allocator_type const& node_allocator() const { return *this; }
};

// create_node

template <typename Allocators, typename Value, typename Parameters, typename Box>
struct create_node<
    Allocators,
    static_internal_node<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
>
{
    static inline typename Allocators::node_pointer
    apply(Allocators & allocators)
    {
        return create_static_node<
            typename Allocators::node_pointer,
            static_internal_node<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
        >::template apply(allocators.node_allocator());
    }
};

template <typename Allocators, typename Value, typename Parameters, typename Box>
struct create_node<
    Allocators,
    static_leaf<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
>
{
    static inline typename Allocators::node_pointer
    apply(Allocators & allocators)
    {
        return create_static_node<
            typename Allocators::node_pointer,
            static_leaf<Value, Parameters, Box, Allocators, node_s_mem_static_tag>
        >::template apply(allocators.node_allocator());
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_STATIC_VARIANT_HPP

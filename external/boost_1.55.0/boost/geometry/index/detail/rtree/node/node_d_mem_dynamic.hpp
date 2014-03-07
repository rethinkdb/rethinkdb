// Boost.Geometry Index
//
// R-tree nodes based on run-time polymorphism, storing std::vectors
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_HPP

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct dynamic_internal_node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
    : public dynamic_node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
{
    typedef typename Allocators::leaf_allocator_type::template rebind<
        rtree::ptr_pair<Box, typename Allocators::node_pointer>
    >::other elements_allocator_type;

    typedef boost::container::vector<
        rtree::ptr_pair<Box, typename Allocators::node_pointer>,
        elements_allocator_type
    > elements_type;

    template <typename Al>
    inline dynamic_internal_node(Al const& al)
        : elements(al)
    {}

    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag, false> & v) { v(*this); }
    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag, true> & v) const { v(*this); }

    elements_type elements;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct dynamic_leaf<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
    : public dynamic_node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
{
    typedef typename Allocators::leaf_allocator_type::template rebind<
        Value
    >::other elements_allocator_type;

    typedef boost::container::vector<
        Value,
        elements_allocator_type
    > elements_type;

    template <typename Al>
    inline dynamic_leaf(Al const& al)
        : elements(al)
    {}

    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag, false> & v) { v(*this); }
    void apply_visitor(dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag, true> & v) const { v(*this); }

    elements_type elements;
};

// nodes traits

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
{
    typedef dynamic_node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag> type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct internal_node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
{
    typedef dynamic_internal_node<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag> type;
};

template <typename Value, typename Parameters, typename Box, typename Allocators>
struct leaf<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag>
{
    typedef dynamic_leaf<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag> type;
};

// visitor traits

template <typename Value, typename Parameters, typename Box, typename Allocators, bool IsVisitableConst>
struct visitor<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag, IsVisitableConst>
{
    typedef dynamic_visitor<Value, Parameters, Box, Allocators, node_d_mem_dynamic_tag, IsVisitableConst> type;
};

// element's indexable type

template <typename Element, typename Translator>
struct element_indexable_type
{
    typedef typename indexable_type<Translator>::type type;
};

template <typename First, typename Pointer, typename Translator>
struct element_indexable_type<
    rtree::ptr_pair<First, Pointer>,
    Translator
>
{
    typedef First type;
};

// element's indexable getter

template <typename Element, typename Translator>
typename result_type<Translator>::type
element_indexable(Element const& el, Translator const& tr)
{
    return tr(el);
}

template <typename First, typename Pointer, typename Translator>
First const&
element_indexable(rtree::ptr_pair<First, Pointer> const& el, Translator const& /*tr*/)
{
    return el.first;
}

// nodes elements

template <typename Node>
struct elements_type
{
    typedef typename Node::elements_type type;
};

template <typename Node>
inline typename elements_type<Node>::type &
elements(Node & n)
{
    return n.elements;
}

template <typename Node>
inline typename elements_type<Node>::type const&
elements(Node const& n)
{
    return n.elements;
}

// elements derived type
template <typename Elements, typename NewValue>
struct container_from_elements_type
{
    typedef boost::container::vector<NewValue> type;
};

// allocators

template <typename Allocator, typename Value, typename Parameters, typename Box>
class allocators<Allocator, Value, Parameters, Box, node_d_mem_dynamic_tag>
    : public Allocator::template rebind<
        typename internal_node<Value, Parameters, Box, allocators<Allocator, Value, Parameters, Box, node_d_mem_dynamic_tag>, node_d_mem_dynamic_tag>::type
    >::other
    , public Allocator::template rebind<
        typename leaf<Value, Parameters, Box, allocators<Allocator, Value, Parameters, Box, node_d_mem_dynamic_tag>, node_d_mem_dynamic_tag>::type
    >::other
{
    typedef typename Allocator::template rebind<
        Value
    >::other value_allocator_type;

public:
    typedef Allocator allocator_type;
    
    typedef Value value_type;
    typedef typename value_allocator_type::reference reference;
    typedef typename value_allocator_type::const_reference const_reference;
    typedef typename value_allocator_type::size_type size_type;
    typedef typename value_allocator_type::difference_type difference_type;
    typedef typename value_allocator_type::pointer pointer;
    typedef typename value_allocator_type::const_pointer const_pointer;

    typedef typename Allocator::template rebind<
        typename node<Value, Parameters, Box, allocators, node_d_mem_dynamic_tag>::type
    >::other::pointer node_pointer;

//    typedef typename Allocator::template rebind<
//        typename internal_node<Value, Parameters, Box, allocators, node_d_mem_dynamic_tag>::type
//    >::other::pointer internal_node_pointer;

    typedef typename Allocator::template rebind<
        typename internal_node<Value, Parameters, Box, allocators, node_d_mem_dynamic_tag>::type
    >::other internal_node_allocator_type;

    typedef typename Allocator::template rebind<
        typename leaf<Value, Parameters, Box, allocators, node_d_mem_dynamic_tag>::type
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
    leaf_allocator_type const& leaf_allocator() const { return *this; }
};

// create_node_impl

template <typename BaseNodePtr, typename Node>
struct create_dynamic_node
{
    template <typename AllocNode>
    static inline BaseNodePtr apply(AllocNode & alloc_node)
    {
        typedef boost::container::allocator_traits<AllocNode> Al;
        typedef typename Al::pointer P;

        P p = Al::allocate(alloc_node, 1);

        if ( 0 == p )
            throw_runtime_error("boost::geometry::index::rtree node creation failed");

        auto_deallocator<AllocNode> deallocator(alloc_node, p);

        Al::construct(alloc_node, boost::addressof(*p), alloc_node);

        deallocator.release();
        return p;
    }
};

// destroy_node_impl

template <typename Node>
struct destroy_dynamic_node
{
    template <typename AllocNode, typename BaseNodePtr>
    static inline void apply(AllocNode & alloc_node, BaseNodePtr n)
    {
        typedef boost::container::allocator_traits<AllocNode> Al;
        typedef typename Al::pointer P;

        P p(&static_cast<Node&>(rtree::get<Node>(*n)));
        Al::destroy(alloc_node, boost::addressof(*p));
        Al::deallocate(alloc_node, p, 1);
    }
};

// create_node

template <typename Allocators, typename Value, typename Parameters, typename Box, typename Tag>
struct create_node<
    Allocators,
    dynamic_internal_node<Value, Parameters, Box, Allocators, Tag>
>
{
    static inline typename Allocators::node_pointer
    apply(Allocators & allocators)
    {
        return create_dynamic_node<
            typename Allocators::node_pointer,
            dynamic_internal_node<Value, Parameters, Box, Allocators, Tag>
        >::apply(allocators.internal_node_allocator());
    }
};

template <typename Allocators, typename Value, typename Parameters, typename Box, typename Tag>
struct create_node<
    Allocators,
    dynamic_leaf<Value, Parameters, Box, Allocators, Tag>
>
{
    static inline typename Allocators::node_pointer
    apply(Allocators & allocators)
    {
        return create_dynamic_node<
            typename Allocators::node_pointer,
            dynamic_leaf<Value, Parameters, Box, Allocators, Tag>
        >::apply(allocators.leaf_allocator());
    }
};

// destroy_node

template <typename Allocators, typename Value, typename Parameters, typename Box, typename Tag>
struct destroy_node<
    Allocators,
    dynamic_internal_node<Value, Parameters, Box, Allocators, Tag>
>
{
    static inline void apply(Allocators & allocators, typename Allocators::node_pointer n)
    {
        destroy_dynamic_node<
            dynamic_internal_node<Value, Parameters, Box, Allocators, Tag>
        >::apply(allocators.internal_node_allocator(), n);
    }
};

template <typename Allocators, typename Value, typename Parameters, typename Box, typename Tag>
struct destroy_node<
    Allocators,
    dynamic_leaf<Value, Parameters, Box, Allocators, Tag>
>
{
    static inline void apply(Allocators & allocators, typename Allocators::node_pointer n)
    {
        destroy_dynamic_node<
            dynamic_leaf<Value, Parameters, Box, Allocators, Tag>
        >::apply(allocators.leaf_allocator(), n);
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_DEFAULT_HPP

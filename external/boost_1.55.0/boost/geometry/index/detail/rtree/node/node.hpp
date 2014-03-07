// Boost.Geometry Index
//
// R-tree nodes
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_HPP

#include <boost/container/vector.hpp>
#include <boost/geometry/index/detail/varray.hpp>

#include <boost/geometry/index/detail/rtree/node/concept.hpp>
#include <boost/geometry/index/detail/rtree/node/pairs.hpp>
#include <boost/geometry/index/detail/rtree/node/auto_deallocator.hpp>

#include <boost/geometry/index/detail/rtree/node/dynamic_visitor.hpp>
#include <boost/geometry/index/detail/rtree/node/node_d_mem_dynamic.hpp>
#include <boost/geometry/index/detail/rtree/node/node_d_mem_static.hpp>

#include <boost/geometry/index/detail/rtree/node/static_visitor.hpp>
#include <boost/geometry/index/detail/rtree/node/node_s_mem_dynamic.hpp>
#include <boost/geometry/index/detail/rtree/node/node_s_mem_static.hpp>

#include <boost/geometry/index/detail/rtree/node/node_auto_ptr.hpp>

#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

#include <boost/geometry/index/detail/algorithms/bounds.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

// elements box

template <typename Box, typename FwdIter, typename Translator>
inline Box elements_box(FwdIter first, FwdIter last, Translator const& tr)
{
    Box result;

    if ( first == last )
    {
        geometry::assign_inverse(result);
        return result;
    }

    detail::bounds(element_indexable(*first, tr), result);
    ++first;

    for ( ; first != last ; ++first )
        geometry::expand(result, element_indexable(*first, tr));

    return result;
}

// destroys subtree if the element is internal node's element
template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
struct destroy_element
{
    typedef typename Options::parameters_type parameters_type;

    typedef typename rtree::internal_node<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    typedef rtree::node_auto_ptr<Value, Options, Translator, Box, Allocators> node_auto_ptr;

    inline static void apply(typename internal_node::elements_type::value_type & element, Allocators & allocators)
    {
         node_auto_ptr dummy(element.second, allocators);
         element.second = 0;
    }

    inline static void apply(typename leaf::elements_type::value_type &, Allocators &) {}
};

// destroys stored subtrees if internal node's elements are passed
template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
struct destroy_elements
{
    typedef typename Options::parameters_type parameters_type;

    typedef typename rtree::internal_node<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    typedef rtree::node_auto_ptr<Value, Options, Translator, Box, Allocators> node_auto_ptr;

    inline static void apply(typename internal_node::elements_type & elements, Allocators & allocators)
    {
        for ( size_t i = 0 ; i < elements.size() ; ++i )
        {
            node_auto_ptr dummy(elements[i].second, allocators);
            elements[i].second = 0;
        }
    }

    inline static void apply(typename leaf::elements_type &, Allocators &)
    {}

    inline static void apply(typename internal_node::elements_type::iterator first,
                             typename internal_node::elements_type::iterator last,
                             Allocators & allocators)
    {
        for ( ; first != last ; ++first )
        {
            node_auto_ptr dummy(first->second, allocators);
            first->second = 0;
        }
    }

    inline static void apply(typename leaf::elements_type::iterator /*first*/,
                             typename leaf::elements_type::iterator /*last*/,
                             Allocators & /*allocators*/)
    {}
};

// clears node, deletes all subtrees stored in node
template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
struct clear_node
{
    typedef typename Options::parameters_type parameters_type;

    typedef typename rtree::node<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    inline static void apply(node & node, Allocators & allocators)
    {
        rtree::visitors::is_leaf<Value, Options, Box, Allocators> ilv;
        rtree::apply_visitor(ilv, node);
        if ( ilv.result )
        {
            apply(rtree::get<leaf>(node), allocators);
        }
        else
        {
            apply(rtree::get<internal_node>(node), allocators);
        }
    }

    inline static void apply(internal_node & internal_node, Allocators & allocators)
    {
        destroy_elements<Value, Options, Translator, Box, Allocators>::apply(rtree::elements(internal_node), allocators);
        rtree::elements(internal_node).clear();
    }

    inline static void apply(leaf & leaf, Allocators &)
    {
        rtree::elements(leaf).clear();
    }
};

template <typename Container, typename Iterator>
void move_from_back(Container & container, Iterator it)
{
    BOOST_GEOMETRY_INDEX_ASSERT(!container.empty(), "cannot copy from empty container");
    Iterator back_it = container.end();
    --back_it;
    if ( it != back_it )
    {
        *it = boost::move(*back_it);                                                             // MAY THROW (copy)
    }
}

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_NODE_NODE_HPP

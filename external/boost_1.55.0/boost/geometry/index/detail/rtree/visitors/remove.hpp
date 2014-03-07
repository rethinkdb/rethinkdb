// Boost.Geometry Index
//
// R-tree removing visitor implementation
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_REMOVE_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_REMOVE_HPP

#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace visitors {

// Default remove algorithm
template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class remove
    : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, false>::type
{
    typedef typename Options::parameters_type parameters_type;

    typedef typename rtree::node<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    typedef rtree::node_auto_ptr<Value, Options, Translator, Box, Allocators> node_auto_ptr;
    typedef typename Allocators::node_pointer node_pointer;

    //typedef typename Allocators::internal_node_pointer internal_node_pointer;
    typedef internal_node * internal_node_pointer;

public:
    inline remove(node_pointer & root,
                  size_t & leafs_level,
                  Value const& value,
                  parameters_type const& parameters,
                  Translator const& translator,
                  Allocators & allocators)
        : m_value(value)
        , m_parameters(parameters)
        , m_translator(translator)
        , m_allocators(allocators)
        , m_root_node(root)
        , m_leafs_level(leafs_level)
        , m_is_value_removed(false)
        , m_parent(0)
        , m_current_child_index(0)
        , m_current_level(0)
        , m_is_underflow(false)
    {
        // TODO
        // assert - check if Value/Box is correct
    }

    inline void operator()(internal_node & n)
    {
        typedef typename rtree::elements_type<internal_node>::type children_type;
        children_type & children = rtree::elements(n);

        // traverse children which boxes intersects value's box
        size_t child_node_index = 0;
        for ( ; child_node_index < children.size() ; ++child_node_index )
        {
            if ( geometry::covered_by(m_translator(m_value), children[child_node_index].first) )
            {
                // next traversing step
                traverse_apply_visitor(n, child_node_index);                                                            // MAY THROW

                if ( m_is_value_removed )
                    break;
            }
        }

        // value was found and removed
        if ( m_is_value_removed )
        {
            typedef typename rtree::elements_type<internal_node>::type elements_type;
            typedef typename elements_type::iterator element_iterator;
            elements_type & elements = rtree::elements(n);

            // underflow occured - child node should be removed
            if ( m_is_underflow )
            {
                element_iterator underfl_el_it = elements.begin() + child_node_index;
                size_t relative_level = m_leafs_level - m_current_level;

                // move node to the container - store node's relative level as well and return new underflow state
                m_is_underflow = store_underflowed_node(elements, underfl_el_it, relative_level);                       // MAY THROW (E: alloc, copy)
            }

            // n is not root - adjust aabb
            if ( 0 != m_parent )
            {
                // underflow state should be ok here
                // note that there may be less than min_elems elements in root
                // so this condition should be checked only here
                BOOST_GEOMETRY_INDEX_ASSERT((elements.size() < m_parameters.get_min_elements()) == m_is_underflow, "unexpected state");

                rtree::elements(*m_parent)[m_current_child_index].first
                    = rtree::elements_box<Box>(elements.begin(), elements.end(), m_translator);
            }
            // n is root node
            else
            {
                BOOST_GEOMETRY_INDEX_ASSERT(&n == &rtree::get<internal_node>(*m_root_node), "node must be the root");

                // reinsert elements from removed nodes (underflows)
                reinsert_removed_nodes_elements();                                                                  // MAY THROW (V, E: alloc, copy, N: alloc)

                // shorten the tree
                if ( rtree::elements(n).size() == 1 )
                {
                    node_pointer root_to_destroy = m_root_node;
                    m_root_node = rtree::elements(n)[0].second;
                    --m_leafs_level;

                    rtree::destroy_node<Allocators, internal_node>::apply(m_allocators, root_to_destroy);
                }
            }
        }
    }

    inline void operator()(leaf & n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type & elements = rtree::elements(n);

        // find value and remove it
        for ( typename elements_type::iterator it = elements.begin() ; it != elements.end() ; ++it )
        {
            if ( m_translator.equals(*it, m_value) )
            {
                rtree::move_from_back(elements, it);                                                           // MAY THROW (V: copy)
                elements.pop_back();
                m_is_value_removed = true;
                break;
            }
        }

        // if value was removed
        if ( m_is_value_removed )
        {
            BOOST_ASSERT_MSG(0 < m_parameters.get_min_elements(), "min number of elements is too small");

            // calc underflow
            m_is_underflow = elements.size() < m_parameters.get_min_elements();

            // n is not root - adjust aabb
            if ( 0 != m_parent )
            {
                rtree::elements(*m_parent)[m_current_child_index].first
                    = rtree::elements_box<Box>(elements.begin(), elements.end(), m_translator);
            }
        }
    }

    bool is_value_removed() const
    {
        return m_is_value_removed;
    }

private:

    typedef std::vector< std::pair<size_t, node_pointer> > UnderflowNodes;

    void traverse_apply_visitor(internal_node &n, size_t choosen_node_index)
    {
        // save previous traverse inputs and set new ones
        internal_node_pointer parent_bckup = m_parent;
        size_t current_child_index_bckup = m_current_child_index;
        size_t current_level_bckup = m_current_level;

        m_parent = &n;
        m_current_child_index = choosen_node_index;
        ++m_current_level;

        // next traversing step
        rtree::apply_visitor(*this, *rtree::elements(n)[choosen_node_index].second);                    // MAY THROW (V, E: alloc, copy, N: alloc)

        // restore previous traverse inputs
        m_parent = parent_bckup;
        m_current_child_index = current_child_index_bckup;
        m_current_level = current_level_bckup;
    }

    bool store_underflowed_node(
            typename rtree::elements_type<internal_node>::type & elements,
            typename rtree::elements_type<internal_node>::type::iterator underfl_el_it,
            size_t relative_level)
    {
        // move node to the container - store node's relative level as well
        m_underflowed_nodes.push_back(std::make_pair(relative_level, underfl_el_it->second));           // MAY THROW (E: alloc, copy)

        BOOST_TRY
        {
            rtree::move_from_back(elements, underfl_el_it);                                             // MAY THROW (E: copy)
            elements.pop_back();
        }
        BOOST_CATCH(...)
        {
            m_underflowed_nodes.pop_back();
            BOOST_RETHROW                                                                                 // RETHROW
        }
        BOOST_CATCH_END

        // calc underflow
        return elements.size() < m_parameters.get_min_elements();
    }

    void reinsert_removed_nodes_elements()
    {
        typename UnderflowNodes::reverse_iterator it = m_underflowed_nodes.rbegin();

        BOOST_TRY
        {
            // reinsert elements from removed nodes
            // begin with levels closer to the root
            for ( ; it != m_underflowed_nodes.rend() ; ++it )
            {
                is_leaf<Value, Options, Box, Allocators> ilv;
                rtree::apply_visitor(ilv, *it->second);
                if ( ilv.result )
                {
                    reinsert_node_elements(rtree::get<leaf>(*it->second), it->first);                        // MAY THROW (V, E: alloc, copy, N: alloc)

                    rtree::destroy_node<Allocators, leaf>::apply(m_allocators, it->second);
                }
                else
                {
                    reinsert_node_elements(rtree::get<internal_node>(*it->second), it->first);               // MAY THROW (V, E: alloc, copy, N: alloc)

                    rtree::destroy_node<Allocators, internal_node>::apply(m_allocators, it->second);
                }
            }

            //m_underflowed_nodes.clear();
        }
        BOOST_CATCH(...)
        {
            // destroy current and remaining nodes
            for ( ; it != m_underflowed_nodes.rend() ; ++it )
            {
                node_auto_ptr dummy(it->second, m_allocators);
            }

            //m_underflowed_nodes.clear();

            BOOST_RETHROW                                                                                      // RETHROW
        }
        BOOST_CATCH_END
    }

    template <typename Node>
    void reinsert_node_elements(Node &n, size_t node_relative_level)
    {
        typedef typename rtree::elements_type<Node>::type elements_type;
        elements_type & elements = rtree::elements(n);

        typename elements_type::iterator it = elements.begin();
        BOOST_TRY
        {
            for ( ; it != elements.end() ; ++it )
            {
                visitors::insert<
                    typename elements_type::value_type,
                    Value, Options, Translator, Box, Allocators,
                    typename Options::insert_tag
                > insert_v(
                    m_root_node, m_leafs_level, *it,
                    m_parameters, m_translator, m_allocators,
                    node_relative_level - 1);

                rtree::apply_visitor(insert_v, *m_root_node);                                               // MAY THROW (V, E: alloc, copy, N: alloc)
            }
        }
        BOOST_CATCH(...)
        {
            ++it;
            rtree::destroy_elements<Value, Options, Translator, Box, Allocators>
                ::apply(it, elements.end(), m_allocators);
            elements.clear();
            BOOST_RETHROW                                                                                     // RETHROW
        }
        BOOST_CATCH_END
    }

    Value const& m_value;
    parameters_type const& m_parameters;
    Translator const& m_translator;
    Allocators & m_allocators;

    node_pointer & m_root_node;
    size_t & m_leafs_level;

    bool m_is_value_removed;
    UnderflowNodes m_underflowed_nodes;

    // traversing input parameters
    internal_node_pointer m_parent;
    size_t m_current_child_index;
    size_t m_current_level;

    // traversing output parameters
    bool m_is_underflow;
};

}}} // namespace detail::rtree::visitors

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_VISITORS_REMOVE_HPP

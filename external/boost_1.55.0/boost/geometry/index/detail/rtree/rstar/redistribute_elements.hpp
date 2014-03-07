// Boost.Geometry Index
//
// R-tree R*-tree split algorithm implementation
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP

#include <boost/geometry/index/detail/algorithms/intersection_content.hpp>
#include <boost/geometry/index/detail/algorithms/union_content.hpp>
#include <boost/geometry/index/detail/algorithms/margin.hpp>

#include <boost/geometry/index/detail/rtree/node/node.hpp>
#include <boost/geometry/index/detail/rtree/visitors/insert.hpp>
#include <boost/geometry/index/detail/rtree/visitors/is_leaf.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree {

namespace rstar {

template <typename Element, typename Translator, typename Tag, size_t Corner, size_t AxisIndex>
class element_axis_corner_less
{
    BOOST_MPL_ASSERT_MSG(false, NOT_IMPLEMENTED_FOR_THIS_TAG, (Tag));
};

template <typename Element, typename Translator, size_t Corner, size_t AxisIndex>
class element_axis_corner_less<Element, Translator, box_tag, Corner, AxisIndex>
{
public:
    element_axis_corner_less(Translator const& tr)
        : m_tr(tr)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        return geometry::get<Corner, AxisIndex>(rtree::element_indexable(e1, m_tr))
            < geometry::get<Corner, AxisIndex>(rtree::element_indexable(e2, m_tr));
    }

private:
    Translator const& m_tr;
};

template <typename Element, typename Translator, size_t Corner, size_t AxisIndex>
class element_axis_corner_less<Element, Translator, point_tag, Corner, AxisIndex>
{
public:
    element_axis_corner_less(Translator const& tr)
        : m_tr(tr)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        return geometry::get<AxisIndex>(rtree::element_indexable(e1, m_tr))
            < geometry::get<AxisIndex>(rtree::element_indexable(e2, m_tr));
    }

private:
    Translator const& m_tr;
};

template <typename Parameters, typename Box, size_t Corner, size_t AxisIndex>
struct choose_split_axis_and_index_for_corner
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_index,
                             margin_type & sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        typedef typename Elements::value_type element_type;
        typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
        typedef typename tag<indexable_type>::type indexable_tag;

        BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == parameters.get_max_elements() + 1, "wrong number of elements");

        // copy elements
        Elements elements_copy(elements);                                                                       // MAY THROW, STRONG (alloc, copy)
        
        // sort elements
        element_axis_corner_less<element_type, Translator, indexable_tag, Corner, AxisIndex> elements_less(translator);
        std::sort(elements_copy.begin(), elements_copy.end(), elements_less);                                   // MAY THROW, BASIC (copy)

        // init outputs
        choosen_index = parameters.get_min_elements();
        sum_of_margins = 0;
        smallest_overlap = (std::numeric_limits<content_type>::max)();
        smallest_content = (std::numeric_limits<content_type>::max)();

        // calculate sum of margins for all distributions
        size_t index_last = parameters.get_max_elements() - parameters.get_min_elements() + 2;
        for ( size_t i = parameters.get_min_elements() ; i < index_last ; ++i )
        {
            // TODO - awulkiew: may be optimized - box of group 1 may be initialized with
            // box of min_elems number of elements and expanded for each iteration by another element

            Box box1 = rtree::elements_box<Box>(elements_copy.begin(), elements_copy.begin() + i, translator);
            Box box2 = rtree::elements_box<Box>(elements_copy.begin() + i, elements_copy.end(), translator);
            
            sum_of_margins += index::detail::comparable_margin(box1) + index::detail::comparable_margin(box2);

            content_type ovl = index::detail::intersection_content(box1, box2);
            content_type con = index::detail::content(box1) + index::detail::content(box2);

            // TODO - shouldn't here be < instead of <= ?
            if ( ovl < smallest_overlap || (ovl == smallest_overlap && con <= smallest_content) )
            {
                choosen_index = i;
                smallest_overlap = ovl;
                smallest_content = con;
            }
        }

        ::boost::ignore_unused_variable_warning(parameters);
    }
};

template <typename Parameters, typename Box, size_t AxisIndex, typename ElementIndexableTag>
struct choose_split_axis_and_index_for_axis
{
    BOOST_MPL_ASSERT_MSG(false, NOT_IMPLEMENTED_FOR_THIS_TAG, (ElementIndexableTag));
};

template <typename Parameters, typename Box, size_t AxisIndex>
struct choose_split_axis_and_index_for_axis<Parameters, Box, AxisIndex, box_tag>
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        size_t index1 = 0;
        margin_type som1 = 0;
        content_type ovl1 = (std::numeric_limits<content_type>::max)();
        content_type con1 = (std::numeric_limits<content_type>::max)();

        choose_split_axis_and_index_for_corner<Parameters, Box, min_corner, AxisIndex>::
            apply(elements, index1,
                  som1, ovl1, con1,
                  parameters, translator);                                                                  // MAY THROW, STRONG

        size_t index2 = 0;
        margin_type som2 = 0;
        content_type ovl2 = (std::numeric_limits<content_type>::max)();
        content_type con2 = (std::numeric_limits<content_type>::max)();

        choose_split_axis_and_index_for_corner<Parameters, Box, max_corner, AxisIndex>::
            apply(elements, index2,
                  som2, ovl2, con2,
                  parameters, translator);                                                                  // MAY THROW, STRONG

        sum_of_margins = som1 + som2;

        if ( ovl1 < ovl2 || (ovl1 == ovl2 && con1 <= con2) )
        {
            choosen_corner = min_corner;
            choosen_index = index1;
            smallest_overlap = ovl1;
            smallest_content = con1;
        }
        else
        {
            choosen_corner = max_corner;
            choosen_index = index2;
            smallest_overlap = ovl2;
            smallest_content = con2;
        }
    }
};

template <typename Parameters, typename Box, size_t AxisIndex>
struct choose_split_axis_and_index_for_axis<Parameters, Box, AxisIndex, point_tag>
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        choose_split_axis_and_index_for_corner<Parameters, Box, min_corner, AxisIndex>::
            apply(elements, choosen_index,
                  sum_of_margins, smallest_overlap, smallest_content,
                  parameters, translator);                                                                  // MAY THROW, STRONG

        choosen_corner = min_corner;
    }
};

template <typename Parameters, typename Box, size_t Dimension>
struct choose_split_axis_and_index
{
    BOOST_STATIC_ASSERT(0 < Dimension);

    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_axis,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & smallest_sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        typedef typename rtree::element_indexable_type<typename Elements::value_type, Translator>::type element_indexable_type;

        choose_split_axis_and_index<Parameters, Box, Dimension - 1>::
            apply(elements, choosen_axis, choosen_corner, choosen_index,
                  smallest_sum_of_margins, smallest_overlap, smallest_content,
                  parameters, translator);                                                                  // MAY THROW, STRONG

        margin_type sum_of_margins = 0;

        size_t corner = min_corner;
        size_t index = 0;

        content_type overlap_val = (std::numeric_limits<content_type>::max)();
        content_type content_val = (std::numeric_limits<content_type>::max)();

        choose_split_axis_and_index_for_axis<
            Parameters,
            Box,
            Dimension - 1,
            typename tag<element_indexable_type>::type
        >::apply(elements, corner, index, sum_of_margins, overlap_val, content_val, parameters, translator); // MAY THROW, STRONG

        if ( sum_of_margins < smallest_sum_of_margins )
        {
            choosen_axis = Dimension - 1;
            choosen_corner = corner;
            choosen_index = index;
            smallest_sum_of_margins = sum_of_margins;
            smallest_overlap = overlap_val;
            smallest_content = content_val;
        }
    }
};

template <typename Parameters, typename Box>
struct choose_split_axis_and_index<Parameters, Box, 1>
{
    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Elements, typename Translator>
    static inline void apply(Elements const& elements,
                             size_t & choosen_axis,
                             size_t & choosen_corner,
                             size_t & choosen_index,
                             margin_type & smallest_sum_of_margins,
                             content_type & smallest_overlap,
                             content_type & smallest_content,
                             Parameters const& parameters,
                             Translator const& translator)
    {
        typedef typename rtree::element_indexable_type<typename Elements::value_type, Translator>::type element_indexable_type;

        choosen_axis = 0;

        choose_split_axis_and_index_for_axis<
            Parameters,
            Box,
            0,
            typename tag<element_indexable_type>::type
        >::apply(elements, choosen_corner, choosen_index, smallest_sum_of_margins, smallest_overlap, smallest_content, parameters, translator); // MAY THROW
    }
};

template <size_t Corner, size_t Dimension>
struct partial_sort
{
    BOOST_STATIC_ASSERT(0 < Dimension);

    template <typename Elements, typename Translator>
    static inline void apply(Elements & elements, const size_t axis, const size_t index, Translator const& tr)
    {
        if ( axis < Dimension - 1 )
        {
            partial_sort<Corner, Dimension - 1>::apply(elements, axis, index, tr);                          // MAY THROW, BASIC (copy)
        }
        else
        {
            BOOST_GEOMETRY_INDEX_ASSERT(axis == Dimension - 1, "unexpected axis value");

            typedef typename Elements::value_type element_type;
            typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
            typedef typename tag<indexable_type>::type indexable_tag;

            element_axis_corner_less<element_type, Translator, indexable_tag, Corner, Dimension - 1> less(tr);
            std::partial_sort(elements.begin(), elements.begin() + index, elements.end(), less);            // MAY THROW, BASIC (copy)
        }
    }
};

template <size_t Corner>
struct partial_sort<Corner, 1>
{
    template <typename Elements, typename Translator>
    static inline void apply(Elements & elements,
                             const size_t BOOST_GEOMETRY_INDEX_ASSERT_UNUSED_PARAM(axis),
                             const size_t index,
                             Translator const& tr)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(axis == 0, "unexpected axis value");

        typedef typename Elements::value_type element_type;
        typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
        typedef typename tag<indexable_type>::type indexable_tag;

        element_axis_corner_less<element_type, Translator, indexable_tag, Corner, 0> less(tr);
        std::partial_sort(elements.begin(), elements.begin() + index, elements.end(), less);                // MAY THROW, BASIC (copy)
    }
};

} // namespace rstar

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
struct redistribute_elements<Value, Options, Translator, Box, Allocators, rstar_tag>
{
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    typedef typename Options::parameters_type parameters_type;

    static const size_t dimension = geometry::dimension<Box>::value;

    typedef typename index::detail::default_margin_result<Box>::type margin_type;
    typedef typename index::detail::default_content_result<Box>::type content_type;

    template <typename Node>
    static inline void apply(
        Node & n,
        Node & second_node,
        Box & box1,
        Box & box2,
        parameters_type const& parameters,
        Translator const& translator,
        Allocators & allocators)
    {
        typedef typename rtree::elements_type<Node>::type elements_type;
        
        elements_type & elements1 = rtree::elements(n);
        elements_type & elements2 = rtree::elements(second_node);

        size_t split_axis = 0;
        size_t split_corner = 0;
        size_t split_index = parameters.get_min_elements();
        margin_type smallest_sum_of_margins = (std::numeric_limits<margin_type>::max)();
        content_type smallest_overlap = (std::numeric_limits<content_type>::max)();
        content_type smallest_content = (std::numeric_limits<content_type>::max)();

        rstar::choose_split_axis_and_index<
            typename Options::parameters_type,
            Box,
            dimension
        >::apply(elements1,
                 split_axis, split_corner, split_index,
                 smallest_sum_of_margins, smallest_overlap, smallest_content,
                 parameters, translator);                                                               // MAY THROW, STRONG

        // TODO: awulkiew - get rid of following static_casts?

        BOOST_GEOMETRY_INDEX_ASSERT(split_axis < dimension, "unexpected value");
        BOOST_GEOMETRY_INDEX_ASSERT(split_corner == static_cast<size_t>(min_corner) || split_corner == static_cast<size_t>(max_corner), "unexpected value");
        BOOST_GEOMETRY_INDEX_ASSERT(parameters.get_min_elements() <= split_index && split_index <= parameters.get_max_elements() - parameters.get_min_elements() + 1, "unexpected value");
        
        // copy original elements
        elements_type elements_copy(elements1);                                                         // MAY THROW, STRONG
        elements_type elements_backup(elements1);                                                       // MAY THROW, STRONG

        // TODO: awulkiew - check if std::partial_sort produces the same result as std::sort
        if ( split_corner == static_cast<size_t>(min_corner) )
            rstar::partial_sort<min_corner, dimension>
                ::apply(elements_copy, split_axis, split_index, translator);                            // MAY THROW, BASIC (copy)
        else
            rstar::partial_sort<max_corner, dimension>
                ::apply(elements_copy, split_axis, split_index, translator);                            // MAY THROW, BASIC (copy)

        BOOST_TRY
        {
            // copy elements to nodes
            elements1.assign(elements_copy.begin(), elements_copy.begin() + split_index);               // MAY THROW, BASIC
            elements2.assign(elements_copy.begin() + split_index, elements_copy.end());                 // MAY THROW, BASIC

            // calculate boxes
            box1 = rtree::elements_box<Box>(elements1.begin(), elements1.end(), translator);
            box2 = rtree::elements_box<Box>(elements2.begin(), elements2.end(), translator);
        }
        BOOST_CATCH(...)
        {
            //elements_copy.clear();
            elements1.clear();
            elements2.clear();

            rtree::destroy_elements<Value, Options, Translator, Box, Allocators>::apply(elements_backup, allocators);
            //elements_backup.clear();

            BOOST_RETHROW                                                                                 // RETHROW, BASIC
        }
        BOOST_CATCH_END
    }
};

}} // namespace detail::rtree

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP

#include <vector>
#include <boost/range/algorithm/copy.hpp>
#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

namespace boost { namespace geometry
{

namespace detail { namespace partition
{

typedef std::vector<std::size_t> index_vector_type;

template <int Dimension, typename Box>
inline void divide_box(Box const& box, Box& lower_box, Box& upper_box)
{
    typedef typename coordinate_type<Box>::type ctype;

    // Divide input box into two parts, e.g. left/right
    ctype two = 2;
    ctype mid = (geometry::get<min_corner, Dimension>(box)
            + geometry::get<max_corner, Dimension>(box)) / two;

    lower_box = box;
    upper_box = box;
    geometry::set<max_corner, Dimension>(lower_box, mid);
    geometry::set<min_corner, Dimension>(upper_box, mid);
}

// Divide collection into three subsets: lower, upper and oversized
// (not-fitting) 
// (lower == left or bottom, upper == right or top)
template <typename OverlapsPolicy, typename InputCollection, typename Box>
static inline void divide_into_subsets(Box const& lower_box,
        Box const& upper_box,
        InputCollection const& collection,
        index_vector_type const& input,
        index_vector_type& lower,
        index_vector_type& upper,
        index_vector_type& exceeding)
{
    typedef boost::range_iterator
        <
            index_vector_type const
        >::type index_iterator_type;

    for(index_iterator_type it = boost::begin(input);
        it != boost::end(input);
        ++it)
    {
        bool const lower_overlapping = OverlapsPolicy::apply(lower_box,
                    collection[*it]);
        bool const upper_overlapping = OverlapsPolicy::apply(upper_box,
                    collection[*it]);

        if (lower_overlapping && upper_overlapping)
        {
            exceeding.push_back(*it);
        }
        else if (lower_overlapping)
        {
            lower.push_back(*it);
        }
        else if (upper_overlapping)
        {
            upper.push_back(*it);
        }
        else
        {
            // Is nowhere! Should not occur!
            BOOST_ASSERT(true);
        }
    }
}

// Match collection with itself
template <typename InputCollection, typename Policy>
static inline void handle_one(InputCollection const& collection,
        index_vector_type const& input,
        Policy& policy)
{
    typedef boost::range_iterator<index_vector_type const>::type
                index_iterator_type;
    // Quadratic behaviour at lowest level (lowest quad, or all exceeding)
    for(index_iterator_type it1 = boost::begin(input);
        it1 != boost::end(input);
        ++it1)
    {
        index_iterator_type it2 = it1;
        for(++it2; it2 != boost::end(input); ++it2)
        {
            policy.apply(collection[*it1], collection[*it2]);
        }
    }
}

// Match collection 1 with collection 2
template <typename InputCollection, typename Policy>
static inline void handle_two(
        InputCollection const& collection1, index_vector_type const& input1,
        InputCollection const& collection2, index_vector_type const& input2,
        Policy& policy)
{
    typedef boost::range_iterator
        <
            index_vector_type const
        >::type index_iterator_type;

    for(index_iterator_type it1 = boost::begin(input1);
        it1 != boost::end(input1);
        ++it1)
    {
        for(index_iterator_type it2 = boost::begin(input2);
            it2 != boost::end(input2);
            ++it2)
        {
            policy.apply(collection1[*it1], collection2[*it2]);
        }
    }
}

template
<
    int Dimension,
    typename Box,
    typename OverlapsPolicy,
    typename VisitBoxPolicy
>
class partition_one_collection
{
    typedef std::vector<std::size_t> index_vector_type;
    typedef typename coordinate_type<Box>::type ctype;
    typedef partition_one_collection
            <
                1 - Dimension,
                Box,
                OverlapsPolicy,
                VisitBoxPolicy
            > sub_divide;

    template <typename InputCollection, typename Policy>
    static inline void next_level(Box const& box,
            InputCollection const& collection,
            index_vector_type const& input,
            int level, std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        if (boost::size(input) > 0)
        {
            if (std::size_t(boost::size(input)) > min_elements && level < 100)
            {
                sub_divide::apply(box, collection, input, level + 1,
                            min_elements, policy, box_policy);
            }
            else
            {
                handle_one(collection, input, policy);
            }
        }
    }

public :
    template <typename InputCollection, typename Policy>
    static inline void apply(Box const& box,
            InputCollection const& collection,
            index_vector_type const& input,
            int level,
            std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        index_vector_type lower, upper, exceeding;
        divide_into_subsets<OverlapsPolicy>(lower_box, upper_box, collection,
                    input, lower, upper, exceeding);

        if (boost::size(exceeding) > 0)
        {
            // All what is not fitting a partition should be combined
            // with each other, and with all which is fitting.
            handle_one(collection, exceeding, policy);
            handle_two(collection, exceeding, collection, lower, policy);
            handle_two(collection, exceeding, collection, upper, policy);
        }

        // Recursively call operation both parts
        next_level(lower_box, collection, lower, level, min_elements,
                        policy, box_policy);
        next_level(upper_box, collection, upper, level, min_elements,
                        policy, box_policy);
    }
};

template
<
    int Dimension,
    typename Box,
    typename OverlapsPolicy,
    typename VisitBoxPolicy
>
class partition_two_collections
{
    typedef std::vector<std::size_t> index_vector_type;
    typedef typename coordinate_type<Box>::type ctype;
    typedef partition_two_collections
            <
                1 - Dimension,
                Box,
                OverlapsPolicy,
                VisitBoxPolicy
            > sub_divide;

    template <typename InputCollection, typename Policy>
    static inline void next_level(Box const& box,
            InputCollection const& collection1,
            index_vector_type const& input1,
            InputCollection const& collection2,
            index_vector_type const& input2,
            int level, std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        if (boost::size(input1) > 0 && boost::size(input2) > 0)
        {
            if (std::size_t(boost::size(input1)) > min_elements
                && std::size_t(boost::size(input2)) > min_elements
                && level < 100)
            {
                sub_divide::apply(box, collection1, input1, collection2,
                                input2, level + 1, min_elements,
                                policy, box_policy);
            }
            else
            {
                box_policy.apply(box, level + 1);
                handle_two(collection1, input1, collection2, input2, policy);
            }
        }
    }

public :
    template <typename InputCollection, typename Policy>
    static inline void apply(Box const& box,
            InputCollection const& collection1, index_vector_type const& input1,
            InputCollection const& collection2, index_vector_type const& input2,
            int level,
            std::size_t min_elements,
            Policy& policy, VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        index_vector_type lower1, upper1, exceeding1;
        index_vector_type lower2, upper2, exceeding2;
        divide_into_subsets<OverlapsPolicy>(lower_box, upper_box, collection1,
                    input1, lower1, upper1, exceeding1);
        divide_into_subsets<OverlapsPolicy>(lower_box, upper_box, collection2,
                    input2, lower2, upper2, exceeding2);

        if (boost::size(exceeding1) > 0)
        {
            // All exceeding from 1 with 2:
            handle_two(collection1, exceeding1, collection2, exceeding2,
                        policy);

            // All exceeding from 1 with lower and upper of 2:
            handle_two(collection1, exceeding1, collection2, lower2, policy);
            handle_two(collection1, exceeding1, collection2, upper2, policy);
        }
        if (boost::size(exceeding2) > 0)
        {
            // All exceeding from 2 with lower and upper of 1:
            handle_two(collection1, lower1, collection2, exceeding2, policy);
            handle_two(collection1, upper1, collection2, exceeding2, policy);
        }

        next_level(lower_box, collection1, lower1, collection2, lower2, level,
                        min_elements, policy, box_policy);
        next_level(upper_box, collection1, upper1, collection2, upper2, level,
                        min_elements, policy, box_policy);
    }
};

}} // namespace detail::partition

struct visit_no_policy
{
    template <typename Box>
    static inline void apply(Box const&, int )
    {}
};

template
<
    typename Box,
    typename ExpandPolicy,
    typename OverlapsPolicy,
    typename VisitBoxPolicy = visit_no_policy
>
class partition
{
    typedef std::vector<std::size_t> index_vector_type;

    template <typename InputCollection>
    static inline void expand_to_collection(InputCollection const& collection,
                Box& total, index_vector_type& index_vector)
    {
        std::size_t index = 0;
        for(typename boost::range_iterator<InputCollection const>::type it
            = boost::begin(collection);
            it != boost::end(collection);
            ++it, ++index)
        {
            ExpandPolicy::apply(total, *it);
            index_vector.push_back(index);
        }
    }

public :
    template <typename InputCollection, typename VisitPolicy>
    static inline void apply(InputCollection const& collection,
            VisitPolicy& visitor,
            std::size_t min_elements = 16,
            VisitBoxPolicy box_visitor = visit_no_policy()
            )
    {
        if (std::size_t(boost::size(collection)) > min_elements)
        {
            index_vector_type index_vector;
            Box total;
            assign_inverse(total);
            expand_to_collection(collection, total, index_vector);

            detail::partition::partition_one_collection
                <
                    0, Box,
                    OverlapsPolicy,
                    VisitBoxPolicy
                >::apply(total, collection, index_vector, 0, min_elements,
                                visitor, box_visitor);
        }
        else
        {
            typedef typename boost::range_iterator
                <
                    InputCollection const
                >::type iterator_type;
            for(iterator_type it1 = boost::begin(collection);
                it1 != boost::end(collection);
                ++it1)
            {
                iterator_type it2 = it1;
                for(++it2; it2 != boost::end(collection); ++it2)
                {
                    visitor.apply(*it1, *it2);
                }
            }
        }
    }

    template <typename InputCollection, typename VisitPolicy>
    static inline void apply(InputCollection const& collection1,
                InputCollection const& collection2,
                VisitPolicy& visitor,
                std::size_t min_elements = 16,
                VisitBoxPolicy box_visitor = visit_no_policy()
                )
    {
        if (std::size_t(boost::size(collection1)) > min_elements
            && std::size_t(boost::size(collection2)) > min_elements)
        {
            index_vector_type index_vector1, index_vector2;
            Box total;
            assign_inverse(total);
            expand_to_collection(collection1, total, index_vector1);
            expand_to_collection(collection2, total, index_vector2);

            detail::partition::partition_two_collections
                <
                    0, Box, OverlapsPolicy, VisitBoxPolicy
                >::apply(total,
                    collection1, index_vector1,
                    collection2, index_vector2,
                    0, min_elements, visitor, box_visitor);
        }
        else
        {
            typedef typename boost::range_iterator
                <
                    InputCollection const
                >::type iterator_type;
            for(iterator_type it1 = boost::begin(collection1);
                it1 != boost::end(collection1);
                ++it1)
            {
                for(iterator_type it2 = boost::begin(collection2);
                    it2 != boost::end(collection2);
                    ++it2)
                {
                    visitor.apply(*it1, *it2);
                }
            }
        }
    }

};

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_CONVEX_GRAHAM_ANDREW_HPP
#define BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_CONVEX_GRAHAM_ANDREW_HPP


#include <cstddef>
#include <algorithm>
#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/strategies/convex_hull.hpp>

#include <boost/geometry/views/detail/range_type.hpp>

#include <boost/geometry/policies/compare.hpp>

#include <boost/geometry/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/views/reversible_view.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace convex_hull
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template
<
    typename InputRange,
    typename RangeIterator,
    typename StrategyLess,
    typename StrategyGreater
>
struct get_extremes
{
    typedef typename point_type<InputRange>::type point_type;

    point_type left, right;

    bool first;

    StrategyLess less;
    StrategyGreater greater;

    inline get_extremes()
        : first(true)
    {}

    inline void apply(InputRange const& range)
    {
        if (boost::size(range) == 0)
        {
            return;
        }

        // First iterate through this range
        // (this two-stage approach avoids many point copies,
        //  because iterators are kept in memory. Because iterators are
        //  not persistent (in MSVC) this approach is not applicable
        //  for more ranges together)

        RangeIterator left_it = boost::begin(range);
        RangeIterator right_it = boost::begin(range);

        for (RangeIterator it = boost::begin(range) + 1;
            it != boost::end(range);
            ++it)
        {
            if (less(*it, *left_it))
            {
                left_it = it;
            }

            if (greater(*it, *right_it))
            {
                right_it = it;
            }
        }

        // Then compare with earlier
        if (first)
        {
            // First time, assign left/right
            left = *left_it;
            right = *right_it;
            first = false;
        }
        else
        {
            // Next time, check if this range was left/right from
            // the extremes already collected
            if (less(*left_it, left))
            {
                left = *left_it;
            }

            if (greater(*right_it, right))
            {
                right = *right_it;
            }
        }
    }
};


template
<
    typename InputRange,
    typename RangeIterator,
    typename Container,
    typename SideStrategy
>
struct assign_range
{
    Container lower_points, upper_points;

    typedef typename point_type<InputRange>::type point_type;

    point_type const& most_left;
    point_type const& most_right;

    inline assign_range(point_type const& left, point_type const& right)
        : most_left(left)
        , most_right(right)
    {}

    inline void apply(InputRange const& range)
    {
        typedef SideStrategy side;

        // Put points in one of the two output sequences
        for (RangeIterator it = boost::begin(range);
            it != boost::end(range);
            ++it)
        {
            // check if it is lying most_left or most_right from the line

            int dir = side::apply(most_left, most_right, *it);
            switch(dir)
            {
                case 1 : // left side
                    upper_points.push_back(*it);
                    break;
                case -1 : // right side
                    lower_points.push_back(*it);
                    break;

                // 0: on line most_left-most_right,
                //    or most_left, or most_right,
                //    -> all never part of hull
            }
        }
    }
};

template <typename Range>
static inline void sort(Range& range)
{
    typedef typename boost::range_value<Range>::type point_type;
    typedef geometry::less<point_type> comparator;

    std::sort(boost::begin(range), boost::end(range), comparator());
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Graham scan strategy to calculate convex hull
\ingroup strategies
\note Completely reworked version inspired on the sources listed below
\see http://www.ddj.com/architect/201806315
\see http://marknelson.us/2007/08/22/convex
 */
template <typename InputGeometry, typename OutputPoint>
class graham_andrew
{
public :
    typedef OutputPoint point_type;
    typedef InputGeometry geometry_type;

private:

    typedef typename cs_tag<point_type>::type cs_tag;

    typedef typename std::vector<point_type> container_type;
    typedef typename std::vector<point_type>::const_iterator iterator;
    typedef typename std::vector<point_type>::const_reverse_iterator rev_iterator;


    class partitions
    {
        friend class graham_andrew;

        container_type m_lower_hull;
        container_type m_upper_hull;
        container_type m_copied_input;
    };


public:
    typedef partitions state_type;


    inline void apply(InputGeometry const& geometry, partitions& state) const
    {
        // First pass.
        // Get min/max (in most cases left / right) points
        // This makes use of the geometry::less/greater predicates

        // For the left boundary it is important that multiple points
        // are sorted from bottom to top. Therefore the less predicate
        // does not take the x-only template parameter (this fixes ticket #6019.
        // For the right boundary it is not necessary (though also not harmful), 
        // because points are sorted from bottom to top in a later stage.
        // For symmetry and to get often more balanced lower/upper halves
        // we keep it.

        typedef typename geometry::detail::range_type<InputGeometry>::type range_type;

        typedef typename boost::range_iterator
            <
                range_type const
            >::type range_iterator;

        detail::get_extremes
            <
                range_type,
                range_iterator,
                geometry::less<point_type>,
                geometry::greater<point_type>
            > extremes;
        geometry::detail::for_each_range(geometry, extremes);

        // Bounding left/right points
        // Second pass, now that extremes are found, assign all points
        // in either lower, either upper
        detail::assign_range
            <
                range_type,
                range_iterator,
                container_type,
                typename strategy::side::services::default_strategy<cs_tag>::type
            > assigner(extremes.left, extremes.right);

        geometry::detail::for_each_range(geometry, assigner);


        // Sort both collections, first on x(, then on y)
        detail::sort(assigner.lower_points);
        detail::sort(assigner.upper_points);

        //std::cout << boost::size(assigner.lower_points) << std::endl;
        //std::cout << boost::size(assigner.upper_points) << std::endl;

        // And decide which point should be in the final hull
        build_half_hull<-1>(assigner.lower_points, state.m_lower_hull,
                extremes.left, extremes.right);
        build_half_hull<1>(assigner.upper_points, state.m_upper_hull,
                extremes.left, extremes.right);
    }


    template <typename OutputIterator>
    inline void result(partitions const& state,
                    OutputIterator out, bool clockwise)  const
    {
        if (clockwise)
        {
            output_range<iterate_forward>(state.m_upper_hull, out, false);
            output_range<iterate_reverse>(state.m_lower_hull, out, true);
        }
        else
        {
            output_range<iterate_forward>(state.m_lower_hull, out, false);
            output_range<iterate_reverse>(state.m_upper_hull, out, true);
        }
    }


private:

    template <int Factor>
    static inline void build_half_hull(container_type const& input,
            container_type& output,
            point_type const& left, point_type const& right)
    {
        output.push_back(left);
        for(iterator it = input.begin(); it != input.end(); ++it)
        {
            add_to_hull<Factor>(*it, output);
        }
        add_to_hull<Factor>(right, output);
    }


    template <int Factor>
    static inline void add_to_hull(point_type const& p, container_type& output)
    {
        typedef typename strategy::side::services::default_strategy<cs_tag>::type side;

        output.push_back(p);
        register std::size_t output_size = output.size();
        while (output_size >= 3)
        {
            rev_iterator rit = output.rbegin();
            point_type const& last = *rit++;
            point_type const& last2 = *rit++;

            if (Factor * side::apply(*rit, last, last2) <= 0)
            {
                // Remove last two points from stack, and add last again
                // This is much faster then erasing the one but last.
                output.pop_back();
                output.pop_back();
                output.push_back(last);
                output_size--;
            }
            else
            {
                return;
            }
        }
    }


    template <iterate_direction Direction, typename OutputIterator>
    static inline void output_range(container_type const& range,
        OutputIterator out, bool skip_first)
    {
        typedef typename reversible_view<container_type const, Direction>::type view_type;
        view_type view(range);
        bool first = true;
        for (typename boost::range_iterator<view_type const>::type it = boost::begin(view);
            it != boost::end(view); ++it)
        {
            if (first && skip_first)
            {
                first = false;
            }
            else
            {
                *out = *it;
                ++out;
            }
        }
    }

};

}} // namespace strategy::convex_hull


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
template <typename InputGeometry, typename OutputPoint>
struct strategy_convex_hull<InputGeometry, OutputPoint, cartesian_tag>
{
    typedef strategy::convex_hull::graham_andrew<InputGeometry, OutputPoint> type;
};
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_AGNOSTIC_CONVEX_GRAHAM_ANDREW_HPP

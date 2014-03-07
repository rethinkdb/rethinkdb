// Boost.Geometry Index
// Unit Test

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_TEST_RTREE_HPP
#define BOOST_GEOMETRY_INDEX_TEST_RTREE_HPP

#include <boost/foreach.hpp>
#include <vector>
#include <algorithm>

#include <geometry_index_test_common.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/index/detail/rtree/utilities/are_levels_ok.hpp>
#include <boost/geometry/index/detail/rtree/utilities/are_boxes_ok.hpp>

//#include <boost/geometry/geometries/ring.hpp>
//#include <boost/geometry/geometries/polygon.hpp>

namespace generate {

// Set point's coordinates

template <typename Point>
struct outside_point
{};

template <typename T, typename C>
struct outside_point< bg::model::point<T, 2, C> >
{
    typedef bg::model::point<T, 2, C> P;
    static P apply()
    {
        return P(13, 26);
    }
};

template <typename T, typename C>
struct outside_point< bg::model::point<T, 3, C> >
{
    typedef bg::model::point<T, 3, C> P;
    static P apply()
    {
        return P(13, 26, 13);
    }
};

// Default value generation

template <typename Value>
struct value_default
{
    static Value apply(){ return Value(); }
};

// Values, input and rtree generation

template <typename Value>
struct value
{};

template <typename T, typename C>
struct value< bg::model::point<T, 2, C> >
{
    typedef bg::model::point<T, 2, C> P;
    static P apply(int x, int y)
    {
        return P(x, y);
    }
};

template <typename T, typename C>
struct value< bg::model::box< bg::model::point<T, 2, C> > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    static B apply(int x, int y)
    {
        return B(P(x, y), P(x + 2, y + 3));
    }
};

template <typename T, typename C>
struct value< std::pair<bg::model::point<T, 2, C>, int> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef std::pair<P, int> R;
    static R apply(int x, int y)
    {
        return std::make_pair(P(x, y), x + y * 100);
    }
};

template <typename T, typename C>
struct value< std::pair<bg::model::box< bg::model::point<T, 2, C> >, int> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    typedef std::pair<B, int> R;
    static R apply(int x, int y)
    {
        return std::make_pair(B(P(x, y), P(x + 2, y + 3)), x + y * 100);
    }
};

template <typename T, typename C>
struct value< boost::tuple<bg::model::point<T, 2, C>, int, int> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef boost::tuple<P, int, int> R;
    static R apply(int x, int y)
    {
        return boost::make_tuple(P(x, y), x + y * 100, 0);
    }
};

template <typename T, typename C>
struct value< boost::tuple<bg::model::box< bg::model::point<T, 2, C> >, int, int> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    typedef boost::tuple<B, int, int> R;
    static R apply(int x, int y)
    {
        return boost::make_tuple(B(P(x, y), P(x + 2, y + 3)), x + y * 100, 0);
    }
};

template <typename T, typename C>
struct value< bg::model::point<T, 3, C> >
{
    typedef bg::model::point<T, 3, C> P;
    static P apply(int x, int y, int z)
    {
        return P(x, y, z);
    }
};

template <typename T, typename C>
struct value< bg::model::box< bg::model::point<T, 3, C> > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    static B apply(int x, int y, int z)
    {
        return B(P(x, y, z), P(x + 2, y + 3, z + 4));
    }
};

template <typename T, typename C>
struct value< std::pair<bg::model::point<T, 3, C>, int> >
{
    typedef bg::model::point<T, 3, C> P;
    typedef std::pair<P, int> R;
    static R apply(int x, int y, int z)
    {
        return std::make_pair(P(x, y, z), x + y * 100 + z * 10000);
    }
};

template <typename T, typename C>
struct value< std::pair<bg::model::box< bg::model::point<T, 3, C> >, int> >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    typedef std::pair<B, int> R;
    static R apply(int x, int y, int z)
    {
        return std::make_pair(B(P(x, y, z), P(x + 2, y + 3, z + 4)), x + y * 100 + z * 10000);
    }
};

template <typename T, typename C>
struct value< boost::tuple<bg::model::point<T, 3, C>, int, int> >
{
    typedef bg::model::point<T, 3, C> P;
    typedef boost::tuple<P, int, int> R;
    static R apply(int x, int y, int z)
    {
        return boost::make_tuple(P(x, y, z), x + y * 100 + z * 10000, 0);
    }
};

template <typename T, typename C>
struct value< boost::tuple<bg::model::box< bg::model::point<T, 3, C> >, int, int> >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    typedef boost::tuple<B, int, int> R;
    static R apply(int x, int y, int z)
    {
        return boost::make_tuple(B(P(x, y, z), P(x + 2, y + 3, z + 4)), x + y * 100 + z * 10000, 0);
    }
};

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

template <typename T, typename C>
struct value< std::tuple<bg::model::point<T, 2, C>, int, int> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef std::tuple<P, int, int> R;
    static R apply(int x, int y)
    {
        return std::make_tuple(P(x, y), x + y * 100, 0);
    }
};

template <typename T, typename C>
struct value< std::tuple<bg::model::box< bg::model::point<T, 2, C> >, int, int> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    typedef std::tuple<B, int, int> R;
    static R apply(int x, int y)
    {
        return std::make_tuple(B(P(x, y), P(x + 2, y + 3)), x + y * 100, 0);
    }
};

template <typename T, typename C>
struct value< std::tuple<bg::model::point<T, 3, C>, int, int> >
{
    typedef bg::model::point<T, 3, C> P;
    typedef std::tuple<P, int, int> R;
    static R apply(int x, int y, int z)
    {
        return std::make_tuple(P(x, y, z), x + y * 100 + z * 10000, 0);
    }
};

template <typename T, typename C>
struct value< std::tuple<bg::model::box< bg::model::point<T, 3, C> >, int, int> >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    typedef std::tuple<B, int, int> R;
    static R apply(int x, int y, int z)
    {
        return std::make_tuple(B(P(x, y, z), P(x + 2, y + 3, z + 4)), x + y * 100 + z * 10000, 0);
    }
};

#endif // #if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

} // namespace generate

// shared_ptr value

template <typename Indexable>
struct test_object
{
    test_object(Indexable const& indexable_) : indexable(indexable_) {}
    Indexable indexable;
};

namespace boost { namespace geometry { namespace index {

template <typename Indexable>
struct indexable< boost::shared_ptr< test_object<Indexable> > >
{
    typedef boost::shared_ptr< test_object<Indexable> > value_type;
    typedef Indexable const& result_type;

    result_type operator()(value_type const& value) const
    {
        return value->indexable;
    }
};

}}}

namespace generate {

template <typename T, typename C>
struct value< boost::shared_ptr<test_object<bg::model::point<T, 2, C> > > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef test_object<P> O;
    typedef boost::shared_ptr<O> R;

    static R apply(int x, int y)
    {
        return R(new O(P(x, y)));
    }
};

template <typename T, typename C>
struct value< boost::shared_ptr<test_object<bg::model::point<T, 3, C> > > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef test_object<P> O;
    typedef boost::shared_ptr<O> R;

    static R apply(int x, int y, int z)
    {   
        return R(new O(P(x, y, z)));
    }
};

template <typename T, typename C>
struct value< boost::shared_ptr<test_object<bg::model::box<bg::model::point<T, 2, C> > > > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    typedef test_object<B> O;
    typedef boost::shared_ptr<O> R;

    static R apply(int x, int y)
    {
        return R(new O(B(P(x, y), P(x + 2, y + 3))));
    }
};

template <typename T, typename C>
struct value< boost::shared_ptr<test_object<bg::model::box<bg::model::point<T, 3, C> > > > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    typedef test_object<B> O;
    typedef boost::shared_ptr<O> R;

    static R apply(int x, int y, int z)
    {   
        return R(new O(B(P(x, y, z), P(x + 2, y + 3, z + 4))));
    }
};

} //namespace generate

// counting value

template <typename Indexable>
struct counting_value
{
    counting_value() { counter()++; }
    counting_value(Indexable const& i) : indexable(i) { counter()++; }
    counting_value(counting_value const& c) : indexable(c.indexable) { counter()++; }
    ~counting_value() { counter()--; }

    static size_t & counter() { static size_t c = 0; return c; }
    Indexable indexable;
};

namespace boost { namespace geometry { namespace index {

template <typename Indexable>
struct indexable< counting_value<Indexable> >
{
    typedef counting_value<Indexable> value_type;
    typedef Indexable const& result_type;
    result_type operator()(value_type const& value) const
    {
        return value.indexable;
    }
};

template <typename Indexable>
struct equal_to< counting_value<Indexable> >
{
    typedef counting_value<Indexable> value_type;
    typedef bool result_type;
    bool operator()(value_type const& v1, value_type const& v2) const
    {
        return boost::geometry::equals(v1.indexable, v2.indexable);
    }
};

}}}

namespace generate {

template <typename T, typename C>
struct value< counting_value<bg::model::point<T, 2, C> > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef counting_value<P> R;
    static R apply(int x, int y) { return R(P(x, y)); }
};

template <typename T, typename C>
struct value< counting_value<bg::model::point<T, 3, C> > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef counting_value<P> R;
    static R apply(int x, int y, int z) { return R(P(x, y, z)); }
};

template <typename T, typename C>
struct value< counting_value<bg::model::box<bg::model::point<T, 2, C> > > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    typedef counting_value<B> R;
    static R apply(int x, int y) { return R(B(P(x, y), P(x+2, y+3))); }
};

template <typename T, typename C>
struct value< counting_value<bg::model::box<bg::model::point<T, 3, C> > > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    typedef counting_value<B> R;
    static R apply(int x, int y, int z) { return R(B(P(x, y, z), P(x+2, y+3, z+4))); }
};

} // namespace generate

// value without default constructor

template <typename Indexable>
struct value_no_dctor
{
    value_no_dctor(Indexable const& i) : indexable(i) {}
    Indexable indexable;
};

namespace boost { namespace geometry { namespace index {

template <typename Indexable>
struct indexable< value_no_dctor<Indexable> >
{
    typedef value_no_dctor<Indexable> value_type;
    typedef Indexable const& result_type;
    result_type operator()(value_type const& value) const
    {
        return value.indexable;
    }
};

template <typename Indexable>
struct equal_to< value_no_dctor<Indexable> >
{
    typedef value_no_dctor<Indexable> value_type;
    typedef bool result_type;
    bool operator()(value_type const& v1, value_type const& v2) const
    {
        return boost::geometry::equals(v1.indexable, v2.indexable);
    }
};

}}}

namespace generate {

template <typename Indexable>
struct value_default< value_no_dctor<Indexable> >
{
    static value_no_dctor<Indexable> apply() { return value_no_dctor<Indexable>(Indexable()); }
};

template <typename T, typename C>
struct value< value_no_dctor<bg::model::point<T, 2, C> > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef value_no_dctor<P> R;
    static R apply(int x, int y) { return R(P(x, y)); }
};

template <typename T, typename C>
struct value< value_no_dctor<bg::model::point<T, 3, C> > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef value_no_dctor<P> R;
    static R apply(int x, int y, int z) { return R(P(x, y, z)); }
};

template <typename T, typename C>
struct value< value_no_dctor<bg::model::box<bg::model::point<T, 2, C> > > >
{
    typedef bg::model::point<T, 2, C> P;
    typedef bg::model::box<P> B;
    typedef value_no_dctor<B> R;
    static R apply(int x, int y) { return R(B(P(x, y), P(x+2, y+3))); }
};

template <typename T, typename C>
struct value< value_no_dctor<bg::model::box<bg::model::point<T, 3, C> > > >
{
    typedef bg::model::point<T, 3, C> P;
    typedef bg::model::box<P> B;
    typedef value_no_dctor<B> R;
    static R apply(int x, int y, int z) { return R(B(P(x, y, z), P(x+2, y+3, z+4))); }
};

// generate input

template <size_t Dimension>
struct input
{};

template <>
struct input<2>
{
    template <typename Value, typename Box>
    static void apply(std::vector<Value> & input, Box & qbox, int size = 1)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < size, "the value must be greather than 0");

        for ( int i = 0 ; i < 12 * size ; i += 3 )
        {
            for ( int j = 1 ; j < 25 * size ; j += 4 )
            {
                input.push_back( generate::value<Value>::apply(i, j) );
            }
        }

        typedef typename bg::traits::point_type<Box>::type P;

        qbox = Box(P(3, 0), P(10, 9));
    }
};

template <>
struct input<3>
{
    template <typename Value, typename Box>
    static void apply(std::vector<Value> & input, Box & qbox, int size = 1)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < size, "the value must be greather than 0");

        for ( int i = 0 ; i < 12 * size ; i += 3 )
        {
            for ( int j = 1 ; j < 25 * size ; j += 4 )
            {
                for ( int k = 2 ; k < 12 * size ; k += 5 )
                {
                    input.push_back( generate::value<Value>::apply(i, j, k) );
                }
            }
        }

        typedef typename bg::traits::point_type<Box>::type P;

        qbox = Box(P(3, 0, 3), P(10, 9, 11));
    }
};

// generate_value_outside

template <typename Value, size_t Dimension>
struct value_outside_impl
{};

template <typename Value>
struct value_outside_impl<Value, 2>
{
    static Value apply()
    {
        //TODO - for size > 1 in generate_input<> this won't be outside
        return generate::value<Value>::apply(13, 26);
    }
};

template <typename Value>
struct value_outside_impl<Value, 3>
{
    static Value apply()
    {
        //TODO - for size > 1 in generate_input<> this won't be outside
        return generate::value<Value>::apply(13, 26, 13);
    }
};

template <typename Rtree>
inline typename Rtree::value_type
value_outside()
{
    typedef typename Rtree::value_type V;
    typedef typename Rtree::indexable_type I;

    return value_outside_impl<V, bg::dimension<I>::value>::apply();
}

template<typename Rtree, typename Elements, typename Box>
void rtree(Rtree & tree, Elements & input, Box & qbox)
{
    typedef typename Rtree::indexable_type I;

    generate::input<
        bg::dimension<I>::value
    >::apply(input, qbox);

    tree.insert(input.begin(), input.end());
}

} // namespace generate

namespace basictest {

// low level test functions

template <typename Rtree, typename Iter, typename Value>
Iter find(Rtree const& rtree, Iter first, Iter last, Value const& value)
{
    for ( ; first != last ; ++first )
        if ( rtree.value_eq()(value, *first) )
            return first;
    return first;
}

template <typename Rtree, typename Value>
void compare_outputs(Rtree const& rtree, std::vector<Value> const& output, std::vector<Value> const& expected_output)
{
    bool are_sizes_ok = (expected_output.size() == output.size());
    BOOST_CHECK( are_sizes_ok );
    if ( are_sizes_ok )
    {
        BOOST_FOREACH(Value const& v, expected_output)
        {
            BOOST_CHECK(find(rtree, output.begin(), output.end(), v) != output.end() );
        }
    }
}

template <typename Rtree, typename Range1, typename Range2>
void exactly_the_same_outputs(Rtree const& rtree, Range1 const& output, Range2 const& expected_output)
{
    size_t s1 = std::distance(output.begin(), output.end());
    size_t s2 = std::distance(expected_output.begin(), expected_output.end());
    BOOST_CHECK(s1 == s2);

    if ( s1 == s2 )
    {
        typename Range1::const_iterator it1 = output.begin();
        typename Range2::const_iterator it2 = expected_output.begin();
        for ( ; it1 != output.end() && it2 != expected_output.end() ; ++it1, ++it2 )
        {
            if ( !rtree.value_eq()(*it1, *it2) )
            {
                BOOST_CHECK(false && "rtree.translator().equals(*it1, *it2)");
                break;
            }
        }
    }
}

// alternative version of std::copy taking iterators of differnet types
template <typename First, typename Last, typename Out>
void copy_alt(First first, Last last, Out out)
{
    for ( ; first != last ; ++first, ++out )
        *out = *first;
}

// spatial query

template <typename Rtree, typename Value, typename Predicates>
void spatial_query(Rtree & rtree, Predicates const& pred, std::vector<Value> const& expected_output)
{
    BOOST_CHECK( bgi::detail::rtree::utilities::are_levels_ok(rtree) );
    if ( !rtree.empty() )
        BOOST_CHECK( bgi::detail::rtree::utilities::are_boxes_ok(rtree) );

    std::vector<Value> output;
    size_t n = rtree.query(pred, std::back_inserter(output));

    BOOST_CHECK( expected_output.size() == n );
    compare_outputs(rtree, output, expected_output);

    std::vector<Value> output2;
    size_t n2 = query(rtree, pred, std::back_inserter(output2));
    
    BOOST_CHECK( n == n2 );
    exactly_the_same_outputs(rtree, output, output2);

    exactly_the_same_outputs(rtree, output, rtree | bgi::adaptors::queried(pred));

    std::vector<Value> output3;
    std::copy(rtree.qbegin(pred), rtree.qend(), std::back_inserter(output3));

    compare_outputs(rtree, output3, expected_output);

    std::vector<Value> output4;
    std::copy(qbegin(rtree, pred), qend(rtree), std::back_inserter(output4));

    exactly_the_same_outputs(rtree, output3, output4);

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
    {
        std::vector<Value> output4;
        std::copy(rtree.qbegin_(pred), rtree.qend_(pred), std::back_inserter(output4));
        compare_outputs(rtree, output4, expected_output);
        output4.clear();
        copy_alt(rtree.qbegin_(pred), rtree.qend_(), std::back_inserter(output4));
        compare_outputs(rtree, output4, expected_output);
    }
#endif
}

// rtree specific queries tests

template <typename Rtree, typename Value, typename Box>
void intersects(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    std::vector<Value> expected_output;

    BOOST_FOREACH(Value const& v, input)
        if ( bg::intersects(tree.indexable_get()(v), qbox) )
            expected_output.push_back(v);

    //spatial_query(tree, qbox, expected_output);
    spatial_query(tree, bgi::intersects(qbox), expected_output);
    spatial_query(tree, !bgi::disjoint(qbox), expected_output);

    /*typedef bg::traits::point_type<Box>::type P;
    bg::model::ring<P> qring;
    bg::convert(qbox, qring);
    spatial_query(tree, bgi::intersects(qring), expected_output);
    spatial_query(tree, !bgi::disjoint(qring), expected_output);
    bg::model::polygon<P> qpoly;
    bg::convert(qbox, qpoly);
    spatial_query(tree, bgi::intersects(qpoly), expected_output);
    spatial_query(tree, !bgi::disjoint(qpoly), expected_output);*/
}

template <typename Rtree, typename Value, typename Box>
void disjoint(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    std::vector<Value> expected_output;

    BOOST_FOREACH(Value const& v, input)
        if ( bg::disjoint(tree.indexable_get()(v), qbox) )
            expected_output.push_back(v);

    spatial_query(tree, bgi::disjoint(qbox), expected_output);
    spatial_query(tree, !bgi::intersects(qbox), expected_output);

    /*typedef bg::traits::point_type<Box>::type P;
    bg::model::ring<P> qring;
    bg::convert(qbox, qring);
    spatial_query(tree, bgi::disjoint(qring), expected_output);
    bg::model::polygon<P> qpoly;
    bg::convert(qbox, qpoly);
    spatial_query(tree, bgi::disjoint(qpoly), expected_output);*/
}

template <typename Tag>
struct contains_impl
{
    template <typename Rtree, typename Value, typename Box>
    static void apply(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
    {
        std::vector<Value> expected_output;

        BOOST_FOREACH(Value const& v, input)
            if ( bg::within(qbox, tree.indexable_get()(v)) )
                expected_output.push_back(v);

        spatial_query(tree, bgi::contains(qbox), expected_output);

        /*typedef bg::traits::point_type<Box>::type P;
        bg::model::ring<P> qring;
        bg::convert(qbox, qring);
        spatial_query(tree, bgi::contains(qring), expected_output);
        bg::model::polygon<P> qpoly;
        bg::convert(qbox, qpoly);
        spatial_query(tree, bgi::contains(qpoly), expected_output);*/
    }
};

template <>
struct contains_impl<bg::point_tag>
{
    template <typename Rtree, typename Value, typename Box>
    static void apply(Rtree const& /*tree*/, std::vector<Value> const& /*input*/, Box const& /*qbox*/)
    {}
};

template <typename Rtree, typename Value, typename Box>
void contains(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    contains_impl<
        typename bg::tag<
            typename Rtree::indexable_type
        >::type
    >::apply(tree, input, qbox);
}

template <typename Rtree, typename Value, typename Box>
void covered_by(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    std::vector<Value> expected_output;

    BOOST_FOREACH(Value const& v, input)
        if ( bg::covered_by(tree.indexable_get()(v), qbox) )
            expected_output.push_back(v);

    spatial_query(tree, bgi::covered_by(qbox), expected_output);

    /*typedef bg::traits::point_type<Box>::type P;
    bg::model::ring<P> qring;
    bg::convert(qbox, qring);
    spatial_query(tree, bgi::covered_by(qring), expected_output);
    bg::model::polygon<P> qpoly;
    bg::convert(qbox, qpoly);
    spatial_query(tree, bgi::covered_by(qpoly), expected_output);*/
}

template <typename Tag>
struct covers_impl
{
    template <typename Rtree, typename Value, typename Box>
    static void apply(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
    {
        std::vector<Value> expected_output;

        BOOST_FOREACH(Value const& v, input)
            if ( bg::covered_by(qbox, tree.indexable_get()(v)) )
                expected_output.push_back(v);

        spatial_query(tree, bgi::covers(qbox), expected_output);

        /*typedef bg::traits::point_type<Box>::type P;
        bg::model::ring<P> qring;
        bg::convert(qbox, qring);
        spatial_query(tree, bgi::covers(qring), expected_output);
        bg::model::polygon<P> qpoly;
        bg::convert(qbox, qpoly);
        spatial_query(tree, bgi::covers(qpoly), expected_output);*/
    }
};

template <>
struct covers_impl<bg::point_tag>
{
    template <typename Rtree, typename Value, typename Box>
    static void apply(Rtree const& /*tree*/, std::vector<Value> const& /*input*/, Box const& /*qbox*/)
    {}
};

template <typename Rtree, typename Value, typename Box>
void covers(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    covers_impl<
        typename bg::tag<
            typename Rtree::indexable_type
        >::type
    >::apply(tree, input, qbox);
}

template <typename Tag>
struct overlaps_impl
{
    template <typename Rtree, typename Value, typename Box>
    static void apply(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
    {
        std::vector<Value> expected_output;

        BOOST_FOREACH(Value const& v, input)
            if ( bg::overlaps(tree.indexable_get()(v), qbox) )
                expected_output.push_back(v);

        spatial_query(tree, bgi::overlaps(qbox), expected_output);

        /*typedef bg::traits::point_type<Box>::type P;
        bg::model::ring<P> qring;
        bg::convert(qbox, qring);
        spatial_query(tree, bgi::overlaps(qring), expected_output);
        bg::model::polygon<P> qpoly;
        bg::convert(qbox, qpoly);
        spatial_query(tree, bgi::overlaps(qpoly), expected_output);*/
    }
};

template <>
struct overlaps_impl<bg::point_tag>
{
    template <typename Rtree, typename Value, typename Box>
    static void apply(Rtree const& /*tree*/, std::vector<Value> const& /*input*/, Box const& /*qbox*/)
    {}
};

template <typename Rtree, typename Value, typename Box>
void overlaps(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    overlaps_impl<
        typename bg::tag<
            typename Rtree::indexable_type
        >::type
    >::apply(tree, input, qbox);
}

//template <typename Tag, size_t Dimension>
//struct touches_impl
//{
//    template <typename Rtree, typename Value, typename Box>
//    static void apply(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
//    {}
//};
//
//template <>
//struct touches_impl<bg::box_tag, 2>
//{
//    template <typename Rtree, typename Value, typename Box>
//    static void apply(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
//    {
//        std::vector<Value> expected_output;
//
//        BOOST_FOREACH(Value const& v, input)
//            if ( bg::touches(tree.translator()(v), qbox) )
//                expected_output.push_back(v);
//
//        spatial_query(tree, bgi::touches(qbox), expected_output);
//    }
//};
//
//template <typename Rtree, typename Value, typename Box>
//void touches(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
//{
//    touches_impl<
//        bgi::traits::tag<typename Rtree::indexable_type>::type,
//        bgi::traits::dimension<typename Rtree::indexable_type>::value
//    >::apply(tree, input, qbox);
//}

template <typename Rtree, typename Value, typename Box>
void within(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    std::vector<Value> expected_output;

    BOOST_FOREACH(Value const& v, input)
        if ( bg::within(tree.indexable_get()(v), qbox) )
            expected_output.push_back(v);

    spatial_query(tree, bgi::within(qbox), expected_output);

    /*typedef bg::traits::point_type<Box>::type P;
    bg::model::ring<P> qring;
    bg::convert(qbox, qring);
    spatial_query(tree, bgi::within(qring), expected_output);
    bg::model::polygon<P> qpoly;
    bg::convert(qbox, qpoly);
    spatial_query(tree, bgi::within(qpoly), expected_output);*/
}

// rtree nearest queries

template <typename Rtree, typename Point>
struct NearestKLess
{
    typedef typename bg::default_distance_result<Point, typename Rtree::indexable_type>::type D;

    template <typename Value>
    bool operator()(std::pair<D, Value> const& p1, std::pair<D, Value> const& p2) const
    {
        return p1.first < p2.first;
    }
};

template <typename Rtree, typename Point>
struct NearestKTransform
{
    typedef typename bg::default_distance_result<Point, typename Rtree::indexable_type>::type D;

    template <typename Value>
    Value const& operator()(std::pair<D, Value> const& p) const
    {
        return p.second;
    }
};

template <typename Rtree, typename Value, typename Point, typename Distance>
void compare_nearest_outputs(Rtree const& rtree, std::vector<Value> const& output, std::vector<Value> const& expected_output, Point const& pt, Distance greatest_distance)
{
    // check output
    bool are_sizes_ok = (expected_output.size() == output.size());
    BOOST_CHECK( are_sizes_ok );
    if ( are_sizes_ok )
    {
        BOOST_FOREACH(Value const& v, output)
        {
            // TODO - perform explicit check here?
            // should all objects which are closest be checked and should exactly the same be found?

            if ( find(rtree, expected_output.begin(), expected_output.end(), v) == expected_output.end() )
            {
                Distance d = bgi::detail::comparable_distance_near(pt, rtree.indexable_get()(v));
                BOOST_CHECK(d == greatest_distance);
            }
        }
    }
}

template <typename Rtree, typename Value, typename Point>
void nearest_query_k(Rtree const& rtree, std::vector<Value> const& input, Point const& pt, unsigned int k)
{
    // TODO: Nearest object may not be the same as found by the rtree if distances are equal
    // All objects with the same closest distance should be picked

    typedef typename bg::default_distance_result<Point, typename Rtree::indexable_type>::type D;

    std::vector< std::pair<D, Value> > test_output;

    // calculate test output - k closest values pairs
    BOOST_FOREACH(Value const& v, input)
    {
        D d = bgi::detail::comparable_distance_near(pt, rtree.indexable_get()(v));

        if ( test_output.size() < k )
            test_output.push_back(std::make_pair(d, v));
        else
        {
            std::sort(test_output.begin(), test_output.end(), NearestKLess<Rtree, Point>());
            if ( d < test_output.back().first )
                test_output.back() = std::make_pair(d, v);
        }
    }

    // caluclate biggest distance
    std::sort(test_output.begin(), test_output.end(), NearestKLess<Rtree, Point>());
    D greatest_distance = 0;
    if ( !test_output.empty() )
        greatest_distance = test_output.back().first;
    
    // transform test output to vector of values
    std::vector<Value> expected_output(test_output.size(), generate::value_default<Value>::apply());
    std::transform(test_output.begin(), test_output.end(), expected_output.begin(), NearestKTransform<Rtree, Point>());

    // calculate output using rtree
    std::vector<Value> output;
    rtree.query(bgi::nearest(pt, k), std::back_inserter(output));

    // check output
    compare_nearest_outputs(rtree, output, expected_output, pt, greatest_distance);

    exactly_the_same_outputs(rtree, output, rtree | bgi::adaptors::queried(bgi::nearest(pt, k)));

    std::vector<Value> output2(k, generate::value_default<Value>::apply());
    typename Rtree::size_type found_count = rtree.query(bgi::nearest(pt, k), output2.begin());
    output2.resize(found_count, generate::value_default<Value>::apply());

    exactly_the_same_outputs(rtree, output, output2);

    std::vector<Value> output3;
    std::copy(rtree.qbegin(bgi::nearest(pt, k)), rtree.qend(), std::back_inserter(output3));

    compare_nearest_outputs(rtree, output3, expected_output, pt, greatest_distance);

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
    {
        std::vector<Value> output4;
        std::copy(rtree.qbegin_(bgi::nearest(pt, k)), rtree.qend_(bgi::nearest(pt, k)), std::back_inserter(output4));
        compare_nearest_outputs(rtree, output4, expected_output, pt, greatest_distance);
        output4.clear();
        copy_alt(rtree.qbegin_(bgi::nearest(pt, k)), rtree.qend_(), std::back_inserter(output4));
        compare_nearest_outputs(rtree, output4, expected_output, pt, greatest_distance);
    }
#endif
}

// rtree nearest not found

struct AlwaysFalse
{
    template <typename Value>
    bool operator()(Value const& ) const { return false; }
};

template <typename Rtree, typename Point>
void nearest_query_not_found(Rtree const& rtree, Point const& pt)
{
    typedef typename Rtree::value_type Value;

    std::vector<Value> output_v;
    size_t n_res = rtree.query(bgi::nearest(pt, 5) && bgi::satisfies(AlwaysFalse()), std::back_inserter(output_v));
    BOOST_CHECK(output_v.size() == n_res);
    BOOST_CHECK(n_res < 5);
}

template <typename Value>
bool satisfies_fun(Value const& ) { return true; }

struct satisfies_obj
{
    template <typename Value>
    bool operator()(Value const& ) const { return true; }
};

template <typename Rtree, typename Value>
void satisfies(Rtree const& rtree, std::vector<Value> const& input)
{
    std::vector<Value> result;    
    rtree.query(bgi::satisfies(satisfies_obj()), std::back_inserter(result));
    BOOST_CHECK(result.size() == input.size());
    result.clear();
    rtree.query(!bgi::satisfies(satisfies_obj()), std::back_inserter(result));
    BOOST_CHECK(result.size() == 0);

    result.clear();
    rtree.query(bgi::satisfies(satisfies_fun<Value>), std::back_inserter(result));
    BOOST_CHECK(result.size() == input.size());
    result.clear();
    rtree.query(!bgi::satisfies(satisfies_fun<Value>), std::back_inserter(result));
    BOOST_CHECK(result.size() == 0);

#ifndef BOOST_NO_CXX11_LAMBDAS
    result.clear();
    rtree.query(bgi::satisfies([](Value const&){ return true; }), std::back_inserter(result));
    BOOST_CHECK(result.size() == input.size());
    result.clear();
    rtree.query(!bgi::satisfies([](Value const&){ return true; }), std::back_inserter(result));
    BOOST_CHECK(result.size() == 0);
#endif
}

// rtree copying and moving

template <typename Rtree, typename Box>
void copy_swap_move(Rtree const& tree, Box const& qbox)
{
    typedef typename Rtree::value_type Value;
    typedef typename Rtree::parameters_type Params;

    size_t s = tree.size();
    Params params = tree.parameters();

    std::vector<Value> expected_output;
    tree.query(bgi::intersects(qbox), std::back_inserter(expected_output));

    // copy constructor
    Rtree t1(tree);

    BOOST_CHECK(tree.empty() == t1.empty());
    BOOST_CHECK(tree.size() == t1.size());
    BOOST_CHECK(t1.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t1.parameters().get_min_elements() == params.get_min_elements());

    std::vector<Value> output;
    t1.query(bgi::intersects(qbox), std::back_inserter(output));
    exactly_the_same_outputs(t1, output, expected_output);

    // copying assignment operator
    t1 = tree;

    BOOST_CHECK(tree.empty() == t1.empty());
    BOOST_CHECK(tree.size() == t1.size());
    BOOST_CHECK(t1.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t1.parameters().get_min_elements() == params.get_min_elements());

    output.clear();
    t1.query(bgi::intersects(qbox), std::back_inserter(output));
    exactly_the_same_outputs(t1, output, expected_output);
    
    Rtree t2(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
    t2.swap(t1);
    BOOST_CHECK(tree.empty() == t2.empty());
    BOOST_CHECK(tree.size() == t2.size());
    BOOST_CHECK(true == t1.empty());
    BOOST_CHECK(0 == t1.size());
    // those fails e.g. on darwin 4.2.1 because it can't copy base obejcts properly
    BOOST_CHECK(t1.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t1.parameters().get_min_elements() == params.get_min_elements());
    BOOST_CHECK(t2.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t2.parameters().get_min_elements() == params.get_min_elements());

    output.clear();
    t1.query(bgi::intersects(qbox), std::back_inserter(output));
    BOOST_CHECK(output.empty());

    output.clear();
    t2.query(bgi::intersects(qbox), std::back_inserter(output));
    exactly_the_same_outputs(t2, output, expected_output);
    t2.swap(t1);
    // those fails e.g. on darwin 4.2.1 because it can't copy base obejcts properly
    BOOST_CHECK(t1.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t1.parameters().get_min_elements() == params.get_min_elements());
    BOOST_CHECK(t2.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t2.parameters().get_min_elements() == params.get_min_elements());

    // moving constructor
    Rtree t3(boost::move(t1), tree.get_allocator());

    BOOST_CHECK(t3.size() == s);
    BOOST_CHECK(t1.size() == 0);
    BOOST_CHECK(t3.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t3.parameters().get_min_elements() == params.get_min_elements());

    output.clear();
    t3.query(bgi::intersects(qbox), std::back_inserter(output));
    exactly_the_same_outputs(t3, output, expected_output);

    // moving assignment operator
    t1 = boost::move(t3);

    BOOST_CHECK(t1.size() == s);
    BOOST_CHECK(t3.size() == 0);
    BOOST_CHECK(t1.parameters().get_max_elements() == params.get_max_elements());
    BOOST_CHECK(t1.parameters().get_min_elements() == params.get_min_elements());

    output.clear();
    t1.query(bgi::intersects(qbox), std::back_inserter(output));
    exactly_the_same_outputs(t1, output, expected_output);

    //TODO - test SWAP

    ::boost::ignore_unused_variable_warning(params);
}

template <typename I, typename O>
inline void my_copy(I first, I last, O out)
{
    for ( ; first != last ; ++first, ++out )
        *out = *first;
}

// rtree creation and insertion

template <typename Rtree, typename Value, typename Box>
void create_insert(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    std::vector<Value> expected_output;
    tree.query(bgi::intersects(qbox), std::back_inserter(expected_output));

    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        BOOST_FOREACH(Value const& v, input)
            t.insert(v);
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        //std::copy(input.begin(), input.end(), bgi::inserter(t));
        my_copy(input.begin(), input.end(), bgi::inserter(t)); // to suppress MSVC warnings
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
    {
        Rtree t(input.begin(), input.end(), tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        compare_outputs(t, output, expected_output);
    }
    {
        Rtree t(input, tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        compare_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        t.insert(input.begin(), input.end());
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        t.insert(input);
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }

    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        BOOST_FOREACH(Value const& v, input)
            bgi::insert(t, v);
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        bgi::query(t, bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        bgi::insert(t, input.begin(), input.end());
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        bgi::query(t, bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree.parameters(), tree.indexable_get(), tree.value_eq(), tree.get_allocator());
        bgi::insert(t, input);
        BOOST_CHECK(tree.size() == t.size());
        std::vector<Value> output;
        bgi::query(t, bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
}

// rtree removing

template <typename Rtree, typename Box>
void remove(Rtree const& tree, Box const& qbox)
{
    typedef typename Rtree::value_type Value;

    std::vector<Value> values_to_remove;
    tree.query(bgi::intersects(qbox), std::back_inserter(values_to_remove));
    BOOST_CHECK(0 < values_to_remove.size());

    std::vector<Value> expected_output;
    tree.query(bgi::disjoint(qbox), std::back_inserter(expected_output));
    size_t expected_removed_count = values_to_remove.size();
    size_t expected_size_after_remove = tree.size() - values_to_remove.size();

    // Add value which is not stored in the Rtree
    Value outsider = generate::value_outside<Rtree>();
    values_to_remove.push_back(outsider);
    
    {
        Rtree t(tree);
        size_t r = 0;
        BOOST_FOREACH(Value const& v, values_to_remove)
            r += t.remove(v);
        BOOST_CHECK( r == expected_removed_count );
        std::vector<Value> output;
        t.query(bgi::disjoint(qbox), std::back_inserter(output));
        BOOST_CHECK( t.size() == expected_size_after_remove );
        BOOST_CHECK( output.size() == tree.size() - expected_removed_count );
        compare_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree);
        size_t r = t.remove(values_to_remove.begin(), values_to_remove.end());
        BOOST_CHECK( r == expected_removed_count );
        std::vector<Value> output;
        t.query(bgi::disjoint(qbox), std::back_inserter(output));
        BOOST_CHECK( t.size() == expected_size_after_remove );
        BOOST_CHECK( output.size() == tree.size() - expected_removed_count );
        compare_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree);
        size_t r = t.remove(values_to_remove);
        BOOST_CHECK( r == expected_removed_count );
        std::vector<Value> output;
        t.query(bgi::disjoint(qbox), std::back_inserter(output));
        BOOST_CHECK( t.size() == expected_size_after_remove );
        BOOST_CHECK( output.size() == tree.size() - expected_removed_count );
        compare_outputs(t, output, expected_output);
    }

    {
        Rtree t(tree);
        size_t r = 0;
        BOOST_FOREACH(Value const& v, values_to_remove)
            r += bgi::remove(t, v);
        BOOST_CHECK( r == expected_removed_count );
        std::vector<Value> output;
        bgi::query(t, bgi::disjoint(qbox), std::back_inserter(output));
        BOOST_CHECK( t.size() == expected_size_after_remove );
        BOOST_CHECK( output.size() == tree.size() - expected_removed_count );
        compare_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree);
        size_t r = bgi::remove(t, values_to_remove.begin(), values_to_remove.end());
        BOOST_CHECK( r == expected_removed_count );
        std::vector<Value> output;
        bgi::query(t, bgi::disjoint(qbox), std::back_inserter(output));
        BOOST_CHECK( t.size() == expected_size_after_remove );
        BOOST_CHECK( output.size() == tree.size() - expected_removed_count );
        compare_outputs(t, output, expected_output);
    }
    {
        Rtree t(tree);
        size_t r = bgi::remove(t, values_to_remove);
        BOOST_CHECK( r == expected_removed_count );
        std::vector<Value> output;
        bgi::query(t, bgi::disjoint(qbox), std::back_inserter(output));
        BOOST_CHECK( t.size() == expected_size_after_remove );
        BOOST_CHECK( output.size() == tree.size() - expected_removed_count );
        compare_outputs(t, output, expected_output);
    }
}

template <typename Rtree, typename Value, typename Box>
void clear(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    std::vector<Value> values_to_remove;
    tree.query(bgi::intersects(qbox), std::back_inserter(values_to_remove));
    BOOST_CHECK(0 < values_to_remove.size());

    //clear
    {
        Rtree t(tree);

        std::vector<Value> expected_output;
        t.query(bgi::intersects(qbox), std::back_inserter(expected_output));
        size_t s = t.size();
        t.clear();
        BOOST_CHECK(t.empty());
        BOOST_CHECK(t.size() == 0);
        t.insert(input);
        BOOST_CHECK(t.size() == s);
        std::vector<Value> output;
        t.query(bgi::intersects(qbox), std::back_inserter(output));
        exactly_the_same_outputs(t, output, expected_output);
    }
}

// rtree queries

template <typename Rtree, typename Value, typename Box>
void queries(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    basictest::intersects(tree, input, qbox);
    basictest::disjoint(tree, input, qbox);
    basictest::covered_by(tree, input, qbox);
    basictest::overlaps(tree, input, qbox);
    //basictest::touches(tree, input, qbox);
    basictest::within(tree, input, qbox);
    basictest::contains(tree, input, qbox);
    basictest::covers(tree, input, qbox);

    typedef typename bg::point_type<Box>::type P;
    P pt;
    bg::centroid(qbox, pt);

    basictest::nearest_query_k(tree, input, pt, 10);
    basictest::nearest_query_not_found(tree, generate::outside_point<P>::apply());

    basictest::satisfies(tree, input);
}

// rtree creation and modification

template <typename Rtree, typename Value, typename Box>
void modifiers(Rtree const& tree, std::vector<Value> const& input, Box const& qbox)
{
    basictest::copy_swap_move(tree, qbox);
    basictest::create_insert(tree, input, qbox);
    basictest::remove(tree, qbox);
    basictest::clear(tree, input, qbox);
}

} // namespace basictest

template <typename Value, typename Parameters, typename Allocator>
void test_rtree_queries(Parameters const& parameters, Allocator const& allocator)
{
    typedef bgi::indexable<Value> I;
    typedef bgi::equal_to<Value> E;
    typedef typename Allocator::template rebind<Value>::other A;
    typedef bgi::rtree<Value, Parameters, I, E, A> Tree;
    typedef typename Tree::bounds_type B;

    Tree tree(parameters, I(), E(), allocator);
    std::vector<Value> input;
    B qbox;

    generate::rtree(tree, input, qbox);

    basictest::queries(tree, input, qbox);

    Tree empty_tree(parameters, I(), E(), allocator);
    std::vector<Value> empty_input;

    basictest::queries(empty_tree, empty_input, qbox);
}

template <typename Value, typename Parameters, typename Allocator>
void test_rtree_modifiers(Parameters const& parameters, Allocator const& allocator)
{
    typedef bgi::indexable<Value> I;
    typedef bgi::equal_to<Value> E;
    typedef typename Allocator::template rebind<Value>::other A;
    typedef bgi::rtree<Value, Parameters, I, E, A> Tree;
    typedef typename Tree::bounds_type B;

    Tree tree(parameters, I(), E(), allocator);
    std::vector<Value> input;
    B qbox;

    generate::rtree(tree, input, qbox);

    basictest::modifiers(tree, input, qbox);

    Tree empty_tree(parameters, I(), E(), allocator);
    std::vector<Value> empty_input;

    basictest::copy_swap_move(empty_tree, qbox);
}

// run all tests for a single Algorithm and single rtree
// defined by Value

template <typename Value, typename Parameters, typename Allocator>
void test_rtree_by_value(Parameters const& parameters, Allocator const& allocator)
{
    test_rtree_queries<Value>(parameters, allocator);
    test_rtree_modifiers<Value>(parameters, allocator);
}

// rtree inserting and removing of counting_value

template <typename Indexable, typename Parameters, typename Allocator>
void test_count_rtree_values(Parameters const& parameters, Allocator const& allocator)
{
    typedef counting_value<Indexable> Value;

    typedef bgi::indexable<Value> I;
    typedef bgi::equal_to<Value> E;
    typedef typename Allocator::template rebind<Value>::other A;
    typedef bgi::rtree<Value, Parameters, I, E, A> Tree;
    typedef typename Tree::bounds_type B;

    Tree t(parameters, I(), E(), allocator);
    std::vector<Value> input;
    B qbox;

    generate::rtree(t, input, qbox);

    size_t rest_count = input.size();

    BOOST_CHECK(t.size() + rest_count == Value::counter());

    std::vector<Value> values_to_remove;
    t.query(bgi::intersects(qbox), std::back_inserter(values_to_remove));

    rest_count += values_to_remove.size();

    BOOST_CHECK(t.size() + rest_count == Value::counter());

    size_t values_count = Value::counter();

    BOOST_FOREACH(Value const& v, values_to_remove)
    {
        size_t r = t.remove(v);
        --values_count;

        BOOST_CHECK(1 == r);
        BOOST_CHECK(Value::counter() == values_count);
        BOOST_CHECK(t.size() + rest_count == values_count);
    }
}

// rtree count

template <typename Indexable, typename Parameters, typename Allocator>
void test_rtree_count(Parameters const& parameters, Allocator const& allocator)
{
    typedef std::pair<Indexable, int> Value;

    typedef bgi::indexable<Value> I;
    typedef bgi::equal_to<Value> E;
    typedef typename Allocator::template rebind<Value>::other A;
    typedef bgi::rtree<Value, Parameters, I, E, A> Tree;
    typedef typename Tree::bounds_type B;

    Tree t(parameters, I(), E(), allocator);
    std::vector<Value> input;
    B qbox;

    generate::rtree(t, input, qbox);
    
    BOOST_CHECK(t.count(input[0]) == 1);
    BOOST_CHECK(t.count(input[0].first) == 1);
        
    t.insert(input[0]);
    
    BOOST_CHECK(t.count(input[0]) == 2);
    BOOST_CHECK(t.count(input[0].first) == 2);

    t.insert(std::make_pair(input[0].first, -1));

    BOOST_CHECK(t.count(input[0]) == 2);
    BOOST_CHECK(t.count(input[0].first) == 3);
}

// test rtree box

template <typename Value, typename Parameters, typename Allocator>
void test_rtree_bounds(Parameters const& parameters, Allocator const& allocator)
{
    typedef bgi::indexable<Value> I;
    typedef bgi::equal_to<Value> E;
    typedef typename Allocator::template rebind<Value>::other A;
    typedef bgi::rtree<Value, Parameters, I, E, A> Tree;
    typedef typename Tree::bounds_type B;
    typedef typename bg::traits::point_type<B>::type P;

    B b;
    bg::assign_inverse(b);

    Tree t(parameters, I(), E(), allocator);
    std::vector<Value> input;
    B qbox;

    BOOST_CHECK(bg::equals(t.bounds(), b));

    generate::rtree(t, input, qbox);

    BOOST_FOREACH(Value const& v, input)
        bg::expand(b, t.indexable_get()(v));

    BOOST_CHECK(bg::equals(t.bounds(), b));
    BOOST_CHECK(bg::equals(t.bounds(), bgi::bounds(t)));

    size_t s = input.size();
    while ( s/2 < input.size() && !input.empty() )
    {
        t.remove(input.back());
        input.pop_back();
    }

    bg::assign_inverse(b);
    BOOST_FOREACH(Value const& v, input)
        bg::expand(b, t.indexable_get()(v));

    BOOST_CHECK(bg::equals(t.bounds(), b));

    Tree t2(t);
    BOOST_CHECK(bg::equals(t2.bounds(), b));
    t2.clear();
    t2 = t;
    BOOST_CHECK(bg::equals(t2.bounds(), b));
    t2.clear();
    t2 = boost::move(t);
    BOOST_CHECK(bg::equals(t2.bounds(), b));

    t.clear();

    bg::assign_inverse(b);
    BOOST_CHECK(bg::equals(t.bounds(), b));
}

template <typename Indexable, typename Parameters, typename Allocator>
void test_rtree_additional(Parameters const& parameters, Allocator const& allocator)
{
    test_count_rtree_values<Indexable>(parameters, allocator);
    test_rtree_count<Indexable>(parameters, allocator);
    test_rtree_bounds<Indexable>(parameters, allocator);
}

// run all tests for one Algorithm for some number of rtrees
// defined by some number of Values constructed from given Point

template<typename Point, typename Parameters, typename Allocator>
void test_rtree_for_point(Parameters const& parameters, Allocator const& allocator)
{
    typedef std::pair<Point, int> PairP;
    typedef boost::tuple<Point, int, int> TupleP;
    typedef boost::shared_ptr< test_object<Point> > SharedPtrP;
    typedef value_no_dctor<Point> VNoDCtor;

    test_rtree_by_value<Point, Parameters>(parameters, allocator);
    test_rtree_by_value<PairP, Parameters>(parameters, allocator);
    test_rtree_by_value<TupleP, Parameters>(parameters, allocator);

    test_rtree_by_value<SharedPtrP, Parameters>(parameters, allocator);
    test_rtree_by_value<VNoDCtor, Parameters>(parameters, allocator);

    test_rtree_additional<Point>(parameters, allocator);

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    typedef std::tuple<Point, int, int> StdTupleP;
    test_rtree_by_value<StdTupleP, Parameters>(parameters, allocator);
#endif
}

template<typename Point, typename Parameters, typename Allocator>
void test_rtree_for_box(Parameters const& parameters, Allocator const& allocator)
{
    typedef bg::model::box<Point> Box;
    typedef std::pair<Box, int> PairB;
    typedef boost::tuple<Box, int, int> TupleB;
    typedef value_no_dctor<Box> VNoDCtor;

    test_rtree_by_value<Box, Parameters>(parameters, allocator);
    test_rtree_by_value<PairB, Parameters>(parameters, allocator);
    test_rtree_by_value<TupleB, Parameters>(parameters, allocator);

    test_rtree_by_value<VNoDCtor, Parameters>(parameters, allocator);

    test_rtree_additional<Box>(parameters, allocator);

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    typedef std::tuple<Box, int, int> StdTupleB;
    test_rtree_by_value<StdTupleB, Parameters>(parameters, allocator);
#endif
}

template<typename Point, typename Parameters>
void test_rtree_for_point(Parameters const& parameters)
{
    test_rtree_for_point<Point>(parameters, std::allocator<int>());
}

template<typename Point, typename Parameters>
void test_rtree_for_box(Parameters const& parameters)
{
    test_rtree_for_box<Point>(parameters, std::allocator<int>());
}

namespace testset {

template<typename Indexable, typename Parameters, typename Allocator>
void modifiers(Parameters const& parameters, Allocator const& allocator)
{
    typedef std::pair<Indexable, int> Pair;
    typedef boost::tuple<Indexable, int, int> Tuple;
    typedef boost::shared_ptr< test_object<Indexable> > SharedPtr;
    typedef value_no_dctor<Indexable> VNoDCtor;

    test_rtree_modifiers<Indexable>(parameters, allocator);
    test_rtree_modifiers<Pair>(parameters, allocator);
    test_rtree_modifiers<Tuple>(parameters, allocator);

    test_rtree_modifiers<SharedPtr>(parameters, allocator);
    test_rtree_modifiers<VNoDCtor>(parameters, allocator);

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    typedef std::tuple<Indexable, int, int> StdTuple;
    test_rtree_modifiers<StdTuple>(parameters, allocator);
#endif
}

template<typename Indexable, typename Parameters, typename Allocator>
void queries(Parameters const& parameters, Allocator const& allocator)
{
    typedef std::pair<Indexable, int> Pair;
    typedef boost::tuple<Indexable, int, int> Tuple;
    typedef boost::shared_ptr< test_object<Indexable> > SharedPtr;
    typedef value_no_dctor<Indexable> VNoDCtor;

    test_rtree_queries<Indexable>(parameters, allocator);
    test_rtree_queries<Pair>(parameters, allocator);
    test_rtree_queries<Tuple>(parameters, allocator);

    test_rtree_queries<SharedPtr>(parameters, allocator);
    test_rtree_queries<VNoDCtor>(parameters, allocator);

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    typedef std::tuple<Indexable, int, int> StdTuple;
    test_rtree_queries<StdTuple>(parameters, allocator);
#endif
}

template<typename Indexable, typename Parameters, typename Allocator>
void additional(Parameters const& parameters, Allocator const& allocator)
{
    test_rtree_additional<Indexable, Parameters>(parameters, allocator);
}

} // namespace testset

#endif // BOOST_GEOMETRY_INDEX_TEST_RTREE_HPP

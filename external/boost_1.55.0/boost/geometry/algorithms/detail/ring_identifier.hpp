// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_RING_IDENTIFIER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_RING_IDENTIFIER_HPP


namespace boost { namespace geometry
{


// Ring Identifier. It is currently: source,multi,ring
struct ring_identifier
{

    inline ring_identifier()
        : source_index(-1)
        , multi_index(-1)
        , ring_index(-1)
    {}

    inline ring_identifier(int src, int mul, int rin)
        : source_index(src)
        , multi_index(mul)
        , ring_index(rin)
    {}

    inline bool operator<(ring_identifier const& other) const
    {
        return source_index != other.source_index ? source_index < other.source_index
            : multi_index !=other.multi_index ? multi_index < other.multi_index
            : ring_index < other.ring_index
            ;
    }

    inline bool operator==(ring_identifier const& other) const
    {
        return source_index == other.source_index
            && ring_index == other.ring_index
            && multi_index == other.multi_index
            ;
    }

#if defined(BOOST_GEOMETRY_DEBUG_IDENTIFIER)
    friend std::ostream& operator<<(std::ostream &os, ring_identifier const& ring_id)
    {
        os << "(s:" << ring_id.source_index;
        if (ring_id.ring_index >= 0) os << ", r:" << ring_id.ring_index;
        if (ring_id.multi_index >= 0) os << ", m:" << ring_id.multi_index;
        os << ")";
        return os;
    }
#endif


    int source_index;
    int multi_index;
    int ring_index;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_RING_IDENTIFIER_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_VISIT_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_VISIT_INFO_HPP


#ifdef BOOST_GEOMETRY_USE_MSM
#  include <boost/geometry/algorithms/detail/overlay/msm_state.hpp>
#endif


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


#if ! defined(BOOST_GEOMETRY_USE_MSM)

class visit_info
{
private :
    static const int NONE = 0;
    static const int STARTED = 1;
    static const int VISITED = 2;
    static const int FINISHED = 3;
    static const int REJECTED = 4;

    int m_visit_code;
    bool m_rejected;

public:
    inline visit_info()
        : m_visit_code(0)
        , m_rejected(false)
    {}

    inline void set_visited() { m_visit_code = VISITED; }
    inline void set_started() { m_visit_code = STARTED; }
    inline void set_finished() { m_visit_code = FINISHED; }
    inline void set_rejected()
    {
        m_visit_code = REJECTED;
        m_rejected = true;
    }

    inline bool none() const { return m_visit_code == NONE; }
    inline bool visited() const { return m_visit_code == VISITED; }
    inline bool started() const { return m_visit_code == STARTED; }
    inline bool finished() const { return m_visit_code == FINISHED; }
    inline bool rejected() const { return m_rejected; }

    inline void clear()
    {
        if (! rejected())
        {
            m_visit_code = NONE;
        }
    }



#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
    friend std::ostream& operator<<(std::ostream &os, visit_info const& v)
    {
        if (v.m_visit_code != 0)
        {
            os << " VIS: " << int(v.m_visit_code);
        }
        return os;
    }
#endif

};


#else


class visit_info
{

private :

#ifndef USE_MSM_MINI
    mutable
#endif
        traverse_state state;

public :
    inline visit_info()
    {
        state.start();
    }

    inline void set_none() { state.process_event(none()); } // Not Yet Implemented!
    inline void set_visited() { state.process_event(visit()); }
    inline void set_started() { state.process_event(starting()); }
    inline void set_finished() { state.process_event(finish()); }

#ifdef USE_MSM_MINI
    inline bool none() const { return state.flag_none(); }
    inline bool visited() const { return state.flag_visited(); }
    inline bool started() const { return state.flag_started(); }
#else
    inline bool none() const { return state.is_flag_active<is_init>(); }
    inline bool visited() const { return state.is_flag_active<is_visited>(); }
    inline bool started() const { return state.is_flag_active<is_started>(); }
#endif

#ifdef BOOST_GEOMETRY_DEBUG_INTERSECTION
    friend std::ostream& operator<<(std::ostream &os, visit_info const& v)
    {
        return os;
    }
#endif
};
#endif


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_VISIT_INFO_HPP

// Boost.Geometry (aka GGL, Generic Geometry Library)
// Tool reporting Implementation Support Status in QBK or plain text format

// Copyright (c) 2011-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_SUPPORT_STATUS_QBK_OUTPUTTER_HPP
#define BOOST_GEOMETRY_SUPPORT_STATUS_QBK_OUTPUTTER_HPP


template <typename Tag> struct qbk_geometry_name                            { static inline std::string name() { return "Range"; } }; // TODO / make general
template <> struct qbk_geometry_name<boost::geometry::point_tag>            { static inline std::string name() { return "Point"; } };
template <> struct qbk_geometry_name<boost::geometry::segment_tag>          { static inline std::string name() { return "Segment"; } };
template <> struct qbk_geometry_name<boost::geometry::box_tag>              { static inline std::string name() { return "Box"; } };
template <> struct qbk_geometry_name<boost::geometry::linestring_tag>       { static inline std::string name() { return "Linestring"; } };
template <> struct qbk_geometry_name<boost::geometry::ring_tag>             { static inline std::string name() { return "Ring"; } };
template <> struct qbk_geometry_name<boost::geometry::polygon_tag>          { static inline std::string name() { return "Polygon"; } };
template <> struct qbk_geometry_name<boost::geometry::multi_point_tag>      { static inline std::string name() { return "MultiPoint"; } };
template <> struct qbk_geometry_name<boost::geometry::multi_linestring_tag> { static inline std::string name() { return "MultiLinestring"; } };
template <> struct qbk_geometry_name<boost::geometry::multi_polygon_tag>    { static inline std::string name() { return "MultiPolygon"; } };


struct qbk_table_row_header
{
    std::ofstream& m_out;

    qbk_table_row_header(std::ofstream& out)
        : m_out(out)
    {}

    template <typename G>
    void operator()(G)
    {
        m_out
            << "[" 
            << qbk_geometry_name
                <
                    typename boost::geometry::tag<G>::type
                >::name() 
            << "]";
    }
};

struct qbk_outputter
{
    std::ofstream m_out;

    std::string filename(std::string const& name)
    {
        std::ostringstream out;
        out << "../../../../generated/" << name << "_status.qbk";
        return out.str();
    }

    explicit qbk_outputter(std::string const& name)
        : m_out(filename(name).c_str())
    {
    }

    inline void ok() { m_out << "[ [$img/ok.png] ]"; }
    inline void nyi() { m_out << "[ [$img/nyi.png] ]"; }

    inline void header(std::string const& )
    {
        m_out << "[heading Supported geometries]" << std::endl;
    }

    template <typename Types>
    inline void table_header() 
    {
        m_out << "[table" << std::endl << "[[ ]";
        boost::mpl::for_each<Types>(qbk_table_row_header(m_out));
        m_out << "]" << std::endl;
    }
    inline void table_header()
    {
        m_out << "[table" << std::endl << "[[Geometry][Status]]" << std::endl;
    }

    inline void table_footer() 
    {
        m_out << "]" << std::endl;
    }

    template <typename G>
    inline void begin_row() 
    {
        m_out 
            << "[["
            << qbk_geometry_name
                <
                    typename boost::geometry::tag<G>::type
                >::name()
            << "]" 
            ;
    }

    inline void end_row() 
    {
        m_out << "]" << std::endl;
    }

};


#endif // BOOST_GEOMETRY_SUPPORT_STATUS_QBK_OUTPUTTER_HPP

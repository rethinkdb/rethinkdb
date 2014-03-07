// Boost.Geometry Index
// Additional tests

// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <GL/glut.h>

#include <boost/foreach.hpp>

#include <boost/geometry/index/rtree.hpp>

#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>

#include <boost/geometry/index/detail/rtree/utilities/gl_draw.hpp>
#include <boost/geometry/index/detail/rtree/utilities/print.hpp>
#include <boost/geometry/index/detail/rtree/utilities/are_boxes_ok.hpp>
#include <boost/geometry/index/detail/rtree/utilities/are_levels_ok.hpp>
#include <boost/geometry/index/detail/rtree/utilities/statistics.hpp>

namespace bg = boost::geometry;
namespace bgi = bg::index;

typedef bg::model::point<float, 2, boost::geometry::cs::cartesian> P;
typedef bg::model::box<P> B;
//bgi::rtree<B> t(2, 1);
typedef bg::model::linestring<P> LS;
typedef bg::model::segment<P> S;
typedef bg::model::ring<P> R;
typedef bg::model::polygon<P> Poly;
typedef bg::model::multi_polygon<Poly> MPoly;

typedef bgi::rtree<
    B,
    bgi::rstar<4, 2>
> RTree;
RTree t;
std::vector<B> vect;

size_t found_count = 0;
P search_point;
size_t count = 5;
std::vector<B> nearest_boxes;
B search_box;
R search_ring;
Poly search_poly;
MPoly search_multi_poly;
S search_segment;
LS search_linestring;
LS search_path;

enum query_mode_type {
    qm_knn, qm_c, qm_d, qm_i, qm_o, qm_w, qm_nc, qm_nd, qm_ni, qm_no, qm_nw, qm_all, qm_ri, qm_pi, qm_mpi, qm_si, qm_lsi, qm_path
} query_mode = qm_knn;

bool search_valid = false;

void query_knn()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;

    search_point = P(x, y);
    nearest_boxes.clear();
    found_count = t.query(
        bgi::nearest(search_point, count),
        std::back_inserter(nearest_boxes)
        );

    if ( found_count > 0 )
    {
        std::cout << "search point: ";
        bgi::detail::utilities::print_indexable(std::cout, search_point);
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "nearest not found\n";
}

void query_path()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;
    float w = 20 + ( rand() % 1000 ) / 100.0f;
    float h = 20 + ( rand() % 1000 ) / 100.0f;

    search_path.resize(10);
    float yy = y-h;
    for ( int i = 0 ; i < 5 ; ++i, yy += h / 2 )
    {
        search_path[2 * i] = P(x-w, yy);
        search_path[2 * i + 1] = P(x+w, yy);
    }
        
    nearest_boxes.clear();
    found_count = t.query(
        bgi::detail::path<LS>(search_path, count),
        std::back_inserter(nearest_boxes)
        );

    if ( found_count > 0 )
    {
        std::cout << "search path: ";
        BOOST_FOREACH(P const& p, search_path)
            bgi::detail::utilities::print_indexable(std::cout, p);
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "values on path not found\n";
}

template <typename Predicate>
void query()
{
    if ( query_mode != qm_all )
    {
        float x = ( rand() % 1000 ) / 10.0f;
        float y = ( rand() % 1000 ) / 10.0f;
        float w = 10 + ( rand() % 1000 ) / 100.0f;
        float h = 10 + ( rand() % 1000 ) / 100.0f;

        search_box = B(P(x - w, y - h), P(x + w, y + h));
        nearest_boxes.clear();
        found_count = t.query(Predicate(search_box), std::back_inserter(nearest_boxes) );
    }
    else
    {
        search_box = t.bounds();
        nearest_boxes.clear();
        found_count = t.query(Predicate(search_box), std::back_inserter(nearest_boxes) );
    }

    if ( found_count > 0 )
    {
        std::cout << "search box: ";
        bgi::detail::utilities::print_indexable(std::cout, search_box);
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "boxes not found\n";
}

template <typename Predicate>
void query_ring()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;
    float w = 10 + ( rand() % 1000 ) / 100.0f;
    float h = 10 + ( rand() % 1000 ) / 100.0f;

    search_ring.clear();
    search_ring.push_back(P(x - w, y - h));
    search_ring.push_back(P(x - w/2, y - h));
    search_ring.push_back(P(x, y - 3*h/2));
    search_ring.push_back(P(x + w/2, y - h));
    search_ring.push_back(P(x + w, y - h));
    search_ring.push_back(P(x + w, y - h/2));
    search_ring.push_back(P(x + 3*w/2, y));
    search_ring.push_back(P(x + w, y + h/2));
    search_ring.push_back(P(x + w, y + h));
    search_ring.push_back(P(x + w/2, y + h));
    search_ring.push_back(P(x, y + 3*h/2));
    search_ring.push_back(P(x - w/2, y + h));
    search_ring.push_back(P(x - w, y + h));
    search_ring.push_back(P(x - w, y + h/2));
    search_ring.push_back(P(x - 3*w/2, y));
    search_ring.push_back(P(x - w, y - h/2));
    search_ring.push_back(P(x - w, y - h));
        
    nearest_boxes.clear();
    found_count = t.query(Predicate(search_ring), std::back_inserter(nearest_boxes) );
    
    if ( found_count > 0 )
    {
        std::cout << "search ring: ";
        BOOST_FOREACH(P const& p, search_ring)
        {
            bgi::detail::utilities::print_indexable(std::cout, p);
            std::cout << ' ';
        }
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "boxes not found\n";
}

template <typename Predicate>
void query_poly()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;
    float w = 10 + ( rand() % 1000 ) / 100.0f;
    float h = 10 + ( rand() % 1000 ) / 100.0f;

    search_poly.clear();
    search_poly.outer().push_back(P(x - w, y - h));
    search_poly.outer().push_back(P(x - w/2, y - h));
    search_poly.outer().push_back(P(x, y - 3*h/2));
    search_poly.outer().push_back(P(x + w/2, y - h));
    search_poly.outer().push_back(P(x + w, y - h));
    search_poly.outer().push_back(P(x + w, y - h/2));
    search_poly.outer().push_back(P(x + 3*w/2, y));
    search_poly.outer().push_back(P(x + w, y + h/2));
    search_poly.outer().push_back(P(x + w, y + h));
    search_poly.outer().push_back(P(x + w/2, y + h));
    search_poly.outer().push_back(P(x, y + 3*h/2));
    search_poly.outer().push_back(P(x - w/2, y + h));
    search_poly.outer().push_back(P(x - w, y + h));
    search_poly.outer().push_back(P(x - w, y + h/2));
    search_poly.outer().push_back(P(x - 3*w/2, y));
    search_poly.outer().push_back(P(x - w, y - h/2));
    search_poly.outer().push_back(P(x - w, y - h));

    search_poly.inners().push_back(Poly::ring_type());
    search_poly.inners()[0].push_back(P(x - w/2, y - h/2));
    search_poly.inners()[0].push_back(P(x + w/2, y - h/2));
    search_poly.inners()[0].push_back(P(x + w/2, y + h/2));
    search_poly.inners()[0].push_back(P(x - w/2, y + h/2));
    search_poly.inners()[0].push_back(P(x - w/2, y - h/2));

    nearest_boxes.clear();
    found_count = t.query(Predicate(search_poly), std::back_inserter(nearest_boxes) );

    if ( found_count > 0 )
    {
        std::cout << "search poly outer: ";
        BOOST_FOREACH(P const& p, search_poly.outer())
        {
            bgi::detail::utilities::print_indexable(std::cout, p);
            std::cout << ' ';
        }
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "boxes not found\n";
}

template <typename Predicate>
void query_multi_poly()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;
    float w = 10 + ( rand() % 1000 ) / 100.0f;
    float h = 10 + ( rand() % 1000 ) / 100.0f;

    search_multi_poly.clear();

    search_multi_poly.push_back(Poly());
    search_multi_poly[0].outer().push_back(P(x - w, y - h));
    search_multi_poly[0].outer().push_back(P(x - w/2, y - h));
    search_multi_poly[0].outer().push_back(P(x, y - 3*h/2));
    search_multi_poly[0].outer().push_back(P(x + w/2, y - h));
    search_multi_poly[0].outer().push_back(P(x + w, y - h));
    search_multi_poly[0].outer().push_back(P(x + w, y - h/2));
    search_multi_poly[0].outer().push_back(P(x + 3*w/2, y));
    search_multi_poly[0].outer().push_back(P(x + w, y + h/2));
    search_multi_poly[0].outer().push_back(P(x + w, y + h));
    search_multi_poly[0].outer().push_back(P(x + w/2, y + h));
    search_multi_poly[0].outer().push_back(P(x, y + 3*h/2));
    search_multi_poly[0].outer().push_back(P(x - w/2, y + h));
    search_multi_poly[0].outer().push_back(P(x - w, y + h));
    search_multi_poly[0].outer().push_back(P(x - w, y + h/2));
    search_multi_poly[0].outer().push_back(P(x - 3*w/2, y));
    search_multi_poly[0].outer().push_back(P(x - w, y - h/2));
    search_multi_poly[0].outer().push_back(P(x - w, y - h));

    search_multi_poly[0].inners().push_back(Poly::ring_type());
    search_multi_poly[0].inners()[0].push_back(P(x - w/2, y - h/2));
    search_multi_poly[0].inners()[0].push_back(P(x + w/2, y - h/2));
    search_multi_poly[0].inners()[0].push_back(P(x + w/2, y + h/2));
    search_multi_poly[0].inners()[0].push_back(P(x - w/2, y + h/2));
    search_multi_poly[0].inners()[0].push_back(P(x - w/2, y - h/2));

    search_multi_poly.push_back(Poly());
    search_multi_poly[1].outer().push_back(P(x - 2*w, y - 2*h));
    search_multi_poly[1].outer().push_back(P(x - 6*w/5, y - 2*h));
    search_multi_poly[1].outer().push_back(P(x - 6*w/5, y - 6*h/5));
    search_multi_poly[1].outer().push_back(P(x - 2*w, y - 6*h/5));
    search_multi_poly[1].outer().push_back(P(x - 2*w, y - 2*h));

    search_multi_poly.push_back(Poly());
    search_multi_poly[2].outer().push_back(P(x + 6*w/5, y + 6*h/5));
    search_multi_poly[2].outer().push_back(P(x + 2*w, y + 6*h/5));
    search_multi_poly[2].outer().push_back(P(x + 2*w, y + 2*h));
    search_multi_poly[2].outer().push_back(P(x + 6*w/5, y + 2*h));
    search_multi_poly[2].outer().push_back(P(x + 6*w/5, y + 6*h/5));

    nearest_boxes.clear();
    found_count = t.query(Predicate(search_multi_poly), std::back_inserter(nearest_boxes) );

    if ( found_count > 0 )
    {
        std::cout << "search multi_poly[0] outer: ";
        BOOST_FOREACH(P const& p, search_multi_poly[0].outer())
        {
            bgi::detail::utilities::print_indexable(std::cout, p);
            std::cout << ' ';
        }
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "boxes not found\n";
}

template <typename Predicate>
void query_segment()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;
    float w = 10.0f - ( rand() % 1000 ) / 50.0f;
    float h = 10.0f - ( rand() % 1000 ) / 50.0f;
    w += 0 <= w ? 10 : -10;
    h += 0 <= h ? 10 : -10;

    boost::geometry::set<0, 0>(search_segment, x - w);
    boost::geometry::set<0, 1>(search_segment, y - h);
    boost::geometry::set<1, 0>(search_segment, x + w);
    boost::geometry::set<1, 1>(search_segment, y + h);

    nearest_boxes.clear();
    found_count = t.query(Predicate(search_segment), std::back_inserter(nearest_boxes) );

    if ( found_count > 0 )
    {
        std::cout << "search segment: ";
        bgi::detail::utilities::print_indexable(std::cout, P(x-w, y-h));
        bgi::detail::utilities::print_indexable(std::cout, P(x+w, y+h));

        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "boxes not found\n";
}

template <typename Predicate>
void query_linestring()
{
    float x = ( rand() % 1000 ) / 10.0f;
    float y = ( rand() % 1000 ) / 10.0f;
    float w = 10 + ( rand() % 1000 ) / 100.0f;
    float h = 10 + ( rand() % 1000 ) / 100.0f;

    search_linestring.clear();
    float a = 0;
    float d = 0;
    for ( size_t i = 0 ; i < 300 ; ++i, a += 0.05, d += 0.005 )
    {
        float xx = x + w * d * ::cos(a);
        float yy = y + h * d * ::sin(a);
        search_linestring.push_back(P(xx, yy));
    }

    nearest_boxes.clear();
    found_count = t.query(Predicate(search_linestring), std::back_inserter(nearest_boxes) );

    if ( found_count > 0 )
    {
        std::cout << "search linestring: ";
        BOOST_FOREACH(P const& p, search_linestring)
        {
            bgi::detail::utilities::print_indexable(std::cout, p);
            std::cout << ' ';
        }
        std::cout << "\nfound: ";
        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
        {
            bgi::detail::utilities::print_indexable(std::cout, nearest_boxes[i]);
            std::cout << '\n';
        }
    }
    else
        std::cout << "boxes not found\n";
}

void search()
{
    namespace d = bgi::detail;

    if ( query_mode == qm_knn )
        query_knn();
    else if ( query_mode == qm_c )
        query< d::spatial_predicate<B, d::covered_by_tag, false> >();
    else if ( query_mode == qm_d )
        query< d::spatial_predicate<B, d::disjoint_tag, false> >();
    else if ( query_mode == qm_i )
        query< d::spatial_predicate<B, d::intersects_tag, false> >();
    else if ( query_mode == qm_o )
        query< d::spatial_predicate<B, d::overlaps_tag, false> >();
    else if ( query_mode == qm_w )
        query< d::spatial_predicate<B, d::within_tag, false> >();
    else if ( query_mode == qm_nc )
        query< d::spatial_predicate<B, d::covered_by_tag, true> >();
    else if ( query_mode == qm_nd )
        query< d::spatial_predicate<B, d::disjoint_tag, true> >();
    else if ( query_mode == qm_ni )
        query< d::spatial_predicate<B, d::intersects_tag, true> >();
    else if ( query_mode == qm_no )
        query< d::spatial_predicate<B, d::overlaps_tag, true> >();
    else if ( query_mode == qm_nw )
        query< d::spatial_predicate<B, d::within_tag, true> >();
    else if ( query_mode == qm_all )
        query< d::spatial_predicate<B, d::intersects_tag, false> >();
    else if ( query_mode == qm_ri )
        query_ring< d::spatial_predicate<R, d::intersects_tag, false> >();
    else if ( query_mode == qm_pi )
        query_poly< d::spatial_predicate<Poly, d::intersects_tag, false> >();
    else if ( query_mode == qm_mpi )
        query_multi_poly< d::spatial_predicate<MPoly, d::intersects_tag, false> >();
    else if ( query_mode == qm_si )
        query_segment< d::spatial_predicate<S, d::intersects_tag, false> >();
    else if ( query_mode == qm_lsi )
        query_linestring< d::spatial_predicate<LS, d::intersects_tag, false> >();
    else if ( query_mode == qm_path )
        query_path();

    search_valid = true;
}

void draw_knn_area(float min_distance, float max_distance)
{
    float x = boost::geometry::get<0>(search_point);
    float y = boost::geometry::get<1>(search_point);
    float z = bgi::detail::rtree::utilities::view<RTree>(t).depth();

    // search point
    glBegin(GL_TRIANGLES);
    glVertex3f(x, y, z);
    glVertex3f(x + 1, y, z);
    glVertex3f(x + 1, y + 1, z);
    glEnd();

    // search min circle

    glBegin(GL_LINE_LOOP);
    for(float a = 0 ; a < 3.14158f * 2 ; a += 3.14158f / 180)
        glVertex3f(x + min_distance * ::cos(a), y + min_distance * ::sin(a), z);
    glEnd();

    // search max circle

    glBegin(GL_LINE_LOOP);
    for(float a = 0 ; a < 3.14158f * 2 ; a += 3.14158f / 180)
        glVertex3f(x + max_distance * ::cos(a), y + max_distance * ::sin(a), z);
    glEnd();
}

void draw_linestring(LS const& ls)
{
    glBegin(GL_LINE_STRIP);

    BOOST_FOREACH(P const& p, ls)
    {
        float x = boost::geometry::get<0>(p);
        float y = boost::geometry::get<1>(p);
        float z = bgi::detail::rtree::utilities::view<RTree>(t).depth();
        glVertex3f(x, y, z);
    }

    glEnd();
}

void draw_segment(S const& s)
{
    float x1 = boost::geometry::get<0, 0>(s);
    float y1 = boost::geometry::get<0, 1>(s);
    float x2 = boost::geometry::get<1, 0>(s);
    float y2 = boost::geometry::get<1, 1>(s);
    float z = bgi::detail::rtree::utilities::view<RTree>(t).depth();

    glBegin(GL_LINES);
    glVertex3f(x1, y1, z);
    glVertex3f(x2, y2, z);
    glEnd();
}

template <typename Box>
void draw_box(Box const& box)
{
    float x1 = boost::geometry::get<bg::min_corner, 0>(box);
    float y1 = boost::geometry::get<bg::min_corner, 1>(box);
    float x2 = boost::geometry::get<bg::max_corner, 0>(box);
    float y2 = boost::geometry::get<bg::max_corner, 1>(box);
    float z = bgi::detail::rtree::utilities::view<RTree>(t).depth();

    // search box
    glBegin(GL_LINE_LOOP);
        glVertex3f(x1, y1, z);
        glVertex3f(x2, y1, z);
        glVertex3f(x2, y2, z);
        glVertex3f(x1, y2, z);
    glEnd();
}

template <typename Range>
void draw_ring(Range const& range)
{
    float z = bgi::detail::rtree::utilities::view<RTree>(t).depth();

    // search box
    glBegin(GL_LINE_LOOP);
    
    BOOST_FOREACH(P const& p, range)
    {
        float x = boost::geometry::get<0>(p);
        float y = boost::geometry::get<1>(p);

        glVertex3f(x, y, z);
    }
    glEnd();
}

template <typename Polygon>
void draw_polygon(Polygon const& polygon)
{
    draw_ring(polygon.outer());
    BOOST_FOREACH(Poly::ring_type const& r, polygon.inners())
        draw_ring(r);
}

template <typename MultiPolygon>
void draw_multi_polygon(MultiPolygon const& multi_polygon)
{
    BOOST_FOREACH(Poly const& p, multi_polygon)
        draw_polygon(p);
}

void render_scene(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    bgi::detail::rtree::utilities::gl_draw(t);

    if ( search_valid )
    {
        glColor3f(1.0f, 0.5f, 0.0f);

        if ( query_mode == qm_knn )
            draw_knn_area(0, 0);
        else if ( query_mode == qm_ri )
            draw_ring(search_ring);
        else if ( query_mode == qm_pi )
            draw_polygon(search_poly);
        else if ( query_mode == qm_mpi )
            draw_multi_polygon(search_multi_poly);
        else if ( query_mode == qm_si )
            draw_segment(search_segment);
        else if ( query_mode == qm_lsi )
            draw_linestring(search_linestring);
        else if ( query_mode == qm_path )
            draw_linestring(search_path);
        else
            draw_box(search_box);

        for ( size_t i = 0 ; i < nearest_boxes.size() ; ++i )
            bgi::detail::utilities::gl_draw_indexable(
                nearest_boxes[i],
                bgi::detail::rtree::utilities::view<RTree>(t).depth());
    }

    glFlush();
}

void resize(int w, int h)
{
    if ( h == 0 )
        h = 1;

    //float ratio = float(w) / h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glViewport(0, 0, w, h);

    //gluPerspective(45, ratio, 1, 1000);
    glOrtho(-150, 150, -150, 150, -150, 150);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /*gluLookAt(
        120.0f, 120.0f, 120.0f, 
        50.0f, 50.0f, -1.0f,
        0.0f, 1.0f, 0.0f);*/
    gluLookAt(
        50.0f, 50.0f, 75.0f, 
        50.0f, 50.0f, -1.0f,
        0.0f, 1.0f, 0.0f);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);

    srand(1);
}

void mouse(int button, int state, int /*x*/, int /*y*/)
{
    if ( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        float x = ( rand() % 100 );
        float y = ( rand() % 100 );
        float w = ( rand() % 2 ) + 1;
        float h = ( rand() % 2 ) + 1;

        B b(P(x - w, y - h),P(x + w, y + h));

        boost::geometry::index::insert(t, b);
        vect.push_back(b);

        std::cout << "inserted: ";
        bgi::detail::utilities::print_indexable(std::cout, b);
        std::cout << '\n';

        std::cout << ( bgi::detail::rtree::utilities::are_boxes_ok(t) ? "boxes OK\n" : "WRONG BOXES!\n" );
        std::cout << ( bgi::detail::rtree::utilities::are_levels_ok(t) ? "levels OK\n" : "WRONG LEVELS!\n" );
        std::cout << "\n";

        search_valid = false;
    }
    else if ( button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
    {
        if ( vect.empty() )
            return;

        size_t i = rand() % vect.size();
        B b = vect[i];

        bgi::remove(t, b);
        vect.erase(vect.begin() + i);

        std::cout << "removed: ";
        bgi::detail::utilities::print_indexable(std::cout, b);
        std::cout << '\n';

        std::cout << ( bgi::detail::rtree::utilities::are_boxes_ok(t) ? "boxes OK\n" : "WRONG BOXES!\n" );
        std::cout << ( bgi::detail::rtree::utilities::are_levels_ok(t) ? "levels OK\n" : "WRONG LEVELS!\n" );
        std::cout << "\n";

        search_valid = false;
    }
    else if ( button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN )
    {
        search();
    }

    glutPostRedisplay();
}

std::string current_line;

void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
    if ( key == '\r' || key == '\n' )
    {
        if ( current_line == "t" )
        {
            std::cout << "\n";
            bgi::detail::rtree::utilities::print(std::cout, t);
            std::cout << "\n";
        }
        else if ( current_line == "rand" )
        {
            for ( size_t i = 0 ; i < 35 ; ++i )
            {
                float x = ( rand() % 100 );
                float y = ( rand() % 100 );
                float w = ( rand() % 2 ) + 1;
                float h = ( rand() % 2 ) + 1;

                B b(P(x - w, y - h),P(x + w, y + h));

                boost::geometry::index::insert(t, b);
                vect.push_back(b);

                std::cout << "inserted: ";
                bgi::detail::utilities::print_indexable(std::cout, b);
                std::cout << '\n';
            }

            std::cout << ( bgi::detail::rtree::utilities::are_boxes_ok(t) ? "boxes OK\n" : "WRONG BOXES!\n" );
            std::cout << ( bgi::detail::rtree::utilities::are_levels_ok(t) ? "levels OK\n" : "WRONG LEVELS!\n" );
            std::cout << "\n";

            search_valid = false;

            glutPostRedisplay();
        }
        else if ( current_line == "bulk" )
        {
            vect.clear();

            for ( size_t i = 0 ; i < 35 ; ++i )
            {
                float x = ( rand() % 100 );
                float y = ( rand() % 100 );
                float w = ( rand() % 2 ) + 1;
                float h = ( rand() % 2 ) + 1;

                B b(P(x - w, y - h),P(x + w, y + h));
                vect.push_back(b);

                std::cout << "inserted: ";
                bgi::detail::utilities::print_indexable(std::cout, b);
                std::cout << '\n';
            }

            RTree t2(vect);
            t = boost::move(t2);

            std::cout << ( bgi::detail::rtree::utilities::are_boxes_ok(t) ? "boxes OK\n" : "WRONG BOXES!\n" );
            std::cout << ( bgi::detail::rtree::utilities::are_levels_ok(t) ? "levels OK\n" : "WRONG LEVELS!\n" );
            std::cout << "\n";

            search_valid = false;

            glutPostRedisplay();
        }
        else
        {
            if ( current_line == "knn" )
                query_mode = qm_knn;
            else if ( current_line == "c" )
                query_mode = qm_c;
            else if ( current_line == "d" )
                query_mode = qm_d;
            else if ( current_line == "i" )
                query_mode = qm_i;
            else if ( current_line == "o" )
                query_mode = qm_o;
            else if ( current_line == "w" )
                query_mode = qm_w;
            else if ( current_line == "nc" )
                query_mode = qm_nc;
            else if ( current_line == "nd" )
                query_mode = qm_nd;
            else if ( current_line == "ni" )
                query_mode = qm_ni;
            else if ( current_line == "no" )
                query_mode = qm_no;
            else if ( current_line == "nw" )
                query_mode = qm_nw;
            else if ( current_line == "all" )
                query_mode = qm_all;
            else if ( current_line == "ri" )
                query_mode = qm_ri;
            else if ( current_line == "pi" )
                query_mode = qm_pi;
            else if ( current_line == "mpi" )
                query_mode = qm_mpi;
            else if ( current_line == "si" )
                query_mode = qm_si;
            else if ( current_line == "lsi" )
                query_mode = qm_lsi;
            else if ( current_line == "path" )
                query_mode = qm_path;
            
            search();
            glutPostRedisplay();
        }

        current_line.clear();
        std::cout << '\n';
    }
    else
    {
        current_line += key;
        std::cout << key;
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(600, 600);
    glutCreateWindow("boost::geometry::index::rtree GLUT test");

    glutDisplayFunc(render_scene);
    glutReshapeFunc(resize);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    return 0;
}

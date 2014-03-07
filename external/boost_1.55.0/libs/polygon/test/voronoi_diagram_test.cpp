// Boost.Polygon library voronoi_diagram_test.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#define BOOST_TEST_MODULE voronoi_diagram_test
#include <boost/test/test_case_template.hpp>

#include <boost/polygon/voronoi_diagram.hpp>
#include <boost/polygon/voronoi_geometry_type.hpp>
using namespace boost::polygon;

typedef voronoi_cell<double> voronoi_cell_type;
typedef voronoi_vertex<double> voronoi_vertex_type;
typedef voronoi_edge<double> voronoi_edge_type;
typedef voronoi_diagram<double> voronoi_diagram_type;

BOOST_AUTO_TEST_CASE(voronoi_cell_test) {
  voronoi_cell_type cell(1, SOURCE_CATEGORY_INITIAL_SEGMENT);
  cell.color(27);
  BOOST_CHECK(!cell.contains_point());
  BOOST_CHECK(cell.contains_segment());
  BOOST_CHECK(cell.is_degenerate());
  BOOST_CHECK(cell.source_index() == 1);
  BOOST_CHECK(cell.source_category() == SOURCE_CATEGORY_INITIAL_SEGMENT);
  BOOST_CHECK(cell.incident_edge() == NULL);
  BOOST_CHECK(cell.color() == 27);

  voronoi_edge_type edge(true, true);
  cell.incident_edge(&edge);
  BOOST_CHECK(!cell.is_degenerate());
  BOOST_CHECK(cell.incident_edge() == &edge);
}

BOOST_AUTO_TEST_CASE(voronoi_vertex_test) {
  voronoi_vertex_type vertex(1, 2);
  vertex.color(27);
  BOOST_CHECK(vertex.is_degenerate());
  BOOST_CHECK(vertex.x() == 1);
  BOOST_CHECK(vertex.y() == 2);
  BOOST_CHECK(vertex.incident_edge() == NULL);
  BOOST_CHECK(vertex.color() == 27);

  voronoi_edge_type edge(true, true);
  vertex.incident_edge(&edge);
  BOOST_CHECK(!vertex.is_degenerate());
  BOOST_CHECK(vertex.incident_edge() == &edge);
}

BOOST_AUTO_TEST_CASE(voronoi_edge_test) {
  voronoi_edge_type edge1(false, false);
  edge1.color(13);
  BOOST_CHECK(!edge1.is_primary());
  BOOST_CHECK(edge1.is_secondary());
  BOOST_CHECK(!edge1.is_linear());
  BOOST_CHECK(edge1.is_curved());
  BOOST_CHECK(!edge1.is_finite());
  BOOST_CHECK(edge1.is_infinite());
  BOOST_CHECK(edge1.color() == 13);

  voronoi_edge_type edge2(true, true);
  edge2.color(14);
  BOOST_CHECK(edge2.is_primary());
  BOOST_CHECK(!edge2.is_secondary());
  BOOST_CHECK(edge2.is_linear());
  BOOST_CHECK(!edge2.is_curved());
  BOOST_CHECK(!edge2.is_finite());
  BOOST_CHECK(edge2.is_infinite());
  BOOST_CHECK(edge2.color() == 14);

  edge1.twin(&edge2);
  edge2.twin(&edge1);
  BOOST_CHECK(edge1.twin() == &edge2);
  BOOST_CHECK(edge2.twin() == &edge1);

  edge1.next(&edge2);
  edge1.prev(&edge2);
  edge2.next(&edge1);
  edge2.prev(&edge1);
  BOOST_CHECK(edge1.next() == &edge2);
  BOOST_CHECK(edge1.prev() == &edge2);
  BOOST_CHECK(edge1.rot_next() == &edge1);
  BOOST_CHECK(edge1.rot_prev() == &edge1);

  voronoi_cell_type cell(1, SOURCE_CATEGORY_INITIAL_SEGMENT);
  edge1.cell(&cell);
  BOOST_CHECK(edge1.cell() == &cell);

  voronoi_vertex_type vertex0(1, 2);
  edge1.vertex0(&vertex0);
  BOOST_CHECK(edge1.vertex0() == &vertex0);
  BOOST_CHECK(edge2.vertex1() == &vertex0);

  voronoi_vertex_type vertex1(2, 1);
  edge2.vertex0(&vertex1);
  BOOST_CHECK(edge1.vertex1() == &vertex1);
  BOOST_CHECK(edge2.vertex0() == &vertex1);

  BOOST_CHECK(edge1.is_finite());
  BOOST_CHECK(edge2.is_finite());
}

BOOST_AUTO_TEST_CASE(voronoi_diagram_test) {
  voronoi_diagram_type vd;
  BOOST_CHECK(vd.num_cells() == 0);
  BOOST_CHECK(vd.num_vertices() == 0);
  BOOST_CHECK(vd.num_edges() == 0);
  vd.clear();
}

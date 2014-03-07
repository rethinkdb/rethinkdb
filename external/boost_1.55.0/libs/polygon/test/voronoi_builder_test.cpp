// Boost.Polygon library voronoi_builder_test.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <ctime>
#include <limits>
#include <list>
#include <vector>

#define BOOST_TEST_MODULE voronoi_builder_test
#include <boost/mpl/list.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/test/test_case_template.hpp>

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#include "voronoi_test_helper.hpp"
using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;

typedef boost::mpl::list<int> test_types;
typedef voronoi_diagram<double> vd_type;
typedef vd_type::coordinate_type coordinate_type;
typedef vd_type::edge_type voronoi_edge_type;
typedef vd_type::const_cell_iterator const_cell_iterator;
typedef vd_type::const_vertex_iterator const_vertex_iterator;

#define CHECK_OUTPUT_SIZE(output, cells, vertices, edges) \
    BOOST_CHECK_EQUAL(output.num_cells(), (std::size_t)cells); \
    BOOST_CHECK_EQUAL(output.num_vertices(), (std::size_t)vertices); \
    BOOST_CHECK_EQUAL(output.num_edges(), (std::size_t)edges)

#define VERIFY_OUTPUT(output) \
    BOOST_CHECK(voronoi_test_helper::verify_output(output, \
        voronoi_test_helper::CELL_CONVEXITY)); \
    BOOST_CHECK(voronoi_test_helper::verify_output(output, \
        voronoi_test_helper::INCIDENT_EDGES_CCW_ORDER)); \
    BOOST_CHECK(voronoi_test_helper::verify_output(output, \
        voronoi_test_helper::NO_HALF_EDGE_INTERSECTIONS))

#define VERIFY_NO_HALF_EDGE_INTERSECTIONS(output) \
    BOOST_CHECK(voronoi_test_helper::verify_output(output, \
        voronoi_test_helper::NO_HALF_EDGE_INTERSECTIONS))

// Sites: (0, 0).
BOOST_AUTO_TEST_CASE_TEMPLATE(single_site_test, T, test_types) {
  std::vector< point_data<T> > points;
  points.push_back(point_data<T>(0, 0));
  vd_type test_output;
  construct_voronoi(points.begin(), points.end(), &test_output);
  VERIFY_OUTPUT(test_output);

  BOOST_CHECK(test_output.cells().size() == 1);
  CHECK_OUTPUT_SIZE(test_output, 1, 0, 0);

  const_cell_iterator it = test_output.cells().begin();
  BOOST_CHECK(it->incident_edge() == NULL);
}

// Sites: (0, 0), (0, 1).
BOOST_AUTO_TEST_CASE_TEMPLATE(collinear_sites_test1, T, test_types) {
  std::vector< point_data<T> > points;
  points.push_back(point_data<T>(0, 0));
  points.push_back(point_data<T>(0, 1));
  vd_type test_output;
  construct_voronoi(points.begin(), points.end(), &test_output);
  VERIFY_OUTPUT(test_output);
  CHECK_OUTPUT_SIZE(test_output, 2, 0, 2);

  const_cell_iterator cell_it = test_output.cells().begin();
  cell_it++;

  const voronoi_edge_type* edge1_1 = cell_it->incident_edge();
  const voronoi_edge_type* edge1_2 = edge1_1->twin();

  BOOST_CHECK(edge1_1->twin() == edge1_2);
  BOOST_CHECK(edge1_2->twin() == edge1_1);

  BOOST_CHECK(edge1_1->next() == edge1_1);
  BOOST_CHECK(edge1_1->prev() == edge1_1);
  BOOST_CHECK(edge1_1->rot_next() == edge1_2);
  BOOST_CHECK(edge1_1->rot_prev() == edge1_2);

  BOOST_CHECK(edge1_2->next() == edge1_2);
  BOOST_CHECK(edge1_2->prev() == edge1_2);
  BOOST_CHECK(edge1_2->rot_next() == edge1_1);
  BOOST_CHECK(edge1_2->rot_prev() == edge1_1);
}

// Sites: (0, 0), (1, 1), (2, 2).
BOOST_AUTO_TEST_CASE_TEMPLATE(collinear_sites_test2, T, test_types) {
  std::vector< point_data<T> > points;
  points.push_back(point_data<T>(0, 0));
  points.push_back(point_data<T>(1, 1));
  points.push_back(point_data<T>(2, 2));
  vd_type test_output;
  construct_voronoi(points.begin(), points.end(), &test_output);
  VERIFY_OUTPUT(test_output);
  CHECK_OUTPUT_SIZE(test_output, 3, 0, 4);

  const_cell_iterator cell_it = test_output.cells().begin();
  const voronoi_edge_type* edge1_1 = cell_it->incident_edge();
  const voronoi_edge_type* edge1_2 = edge1_1->twin();
  cell_it++;
  cell_it++;
  const voronoi_edge_type* edge2_2 = cell_it->incident_edge();
  const voronoi_edge_type* edge2_1 = edge2_2->twin();

  BOOST_CHECK(edge1_1->twin() == edge1_2 && edge1_2->twin() == edge1_1);
  BOOST_CHECK(edge2_1->twin() == edge2_2 && edge2_2->twin() == edge2_1);

  BOOST_CHECK(edge1_1->next() == edge1_1 && edge1_1->prev() == edge1_1);
  BOOST_CHECK(edge1_2->next() == edge2_1 && edge1_2->prev() == edge2_1);
  BOOST_CHECK(edge2_1->next() == edge1_2 && edge2_1->prev() == edge1_2);
  BOOST_CHECK(edge2_2->next() == edge2_2 && edge2_2->prev() == edge2_2);

  BOOST_CHECK(edge1_1->rot_next() == edge1_2 && edge1_1->rot_prev() == edge2_1);
  BOOST_CHECK(edge1_2->rot_next() == edge2_2 && edge1_2->rot_prev() == edge1_1);
  BOOST_CHECK(edge2_1->rot_next() == edge1_1 && edge2_1->rot_prev() == edge2_2);
  BOOST_CHECK(edge2_2->rot_next() == edge2_1 && edge2_2->rot_prev() == edge1_2);

  BOOST_CHECK(edge1_2->next() == edge2_1 && edge1_2->prev() == edge2_1);
  BOOST_CHECK(edge2_1->next() == edge1_2 && edge2_1->prev() == edge1_2);
}

// Sites: (0, 0), (0, 4), (2, 1).
BOOST_AUTO_TEST_CASE_TEMPLATE(triangle_test1, T, test_types) {
  point_data<T> point1(0, 0);
  point_data<T> point2(0, 4);
  point_data<T> point3(2, 1);
  std::vector< point_data<T> > points;
  points.push_back(point1);
  points.push_back(point2);
  points.push_back(point3);
  vd_type test_output;
  construct_voronoi(points.begin(), points.end(), &test_output);
  VERIFY_OUTPUT(test_output);
  CHECK_OUTPUT_SIZE(test_output, 3, 1, 6);

  const_vertex_iterator it = test_output.vertices().begin();
  BOOST_CHECK_EQUAL(it->x(), 0.25);
  BOOST_CHECK_EQUAL(it->y(), 2.0);

  const voronoi_edge_type* edge1_1 = it->incident_edge();
  const voronoi_edge_type* edge1_2 = edge1_1->twin();
  BOOST_CHECK(edge1_1->cell()->source_index() == 1);
  BOOST_CHECK(edge1_2->cell()->source_index() == 2);

  const voronoi_edge_type* edge2_1 = edge1_1->rot_prev();
  const voronoi_edge_type* edge2_2 = edge2_1->twin();
  BOOST_CHECK(edge2_1->cell()->source_index() == 2);
  BOOST_CHECK(edge2_2->cell()->source_index() == 0);

  const voronoi_edge_type* edge3_1 = edge2_1->rot_prev();
  const voronoi_edge_type* edge3_2 = edge3_1->twin();
  BOOST_CHECK(edge3_1->cell()->source_index() == 0);
  BOOST_CHECK(edge3_2->cell()->source_index() == 1);

  BOOST_CHECK(edge1_2->twin() == edge1_1);
  BOOST_CHECK(edge2_2->twin() == edge2_1);
  BOOST_CHECK(edge3_2->twin() == edge3_1);

  BOOST_CHECK(edge1_1->prev() == edge3_2 && edge1_1->next() == edge3_2);
  BOOST_CHECK(edge2_1->prev() == edge1_2 && edge2_1->next() == edge1_2);
  BOOST_CHECK(edge3_1->prev() == edge2_2 && edge3_1->next() == edge2_2);

  BOOST_CHECK(edge1_2->next() == edge2_1 && edge1_2->prev() == edge2_1);
  BOOST_CHECK(edge2_2->next() == edge3_1 && edge2_2->prev() == edge3_1);
  BOOST_CHECK(edge3_2->next() == edge1_1 && edge3_2->prev() == edge1_1);

  BOOST_CHECK(edge1_1->rot_next() == edge3_1);
  BOOST_CHECK(edge3_1->rot_next() == edge2_1);
  BOOST_CHECK(edge2_1->rot_next() == edge1_1);

  BOOST_CHECK(edge1_2->rot_next() == edge2_2);
  BOOST_CHECK(edge2_2->rot_next() == edge3_2);
  BOOST_CHECK(edge3_2->rot_next() == edge1_2);
}

// Sites: (0, 1), (2, 0), (2, 4).
BOOST_AUTO_TEST_CASE_TEMPLATE(triangle_test2, T, test_types) {
  point_data<T> point1(0, 1);
  point_data<T> point2(2, 0);
  point_data<T> point3(2, 4);
  std::vector< point_data<T> > points;
  points.push_back(point1);
  points.push_back(point2);
  points.push_back(point3);
  vd_type test_output;
  construct_voronoi(points.begin(), points.end(), &test_output);
  VERIFY_OUTPUT(test_output);
  CHECK_OUTPUT_SIZE(test_output, 3, 1, 6);

  const_vertex_iterator it = test_output.vertices().begin();
  BOOST_CHECK_EQUAL(it->x(), 1.75);
  BOOST_CHECK_EQUAL(it->y(), 2.0);

  const voronoi_edge_type* edge1_1 = it->incident_edge();
  const voronoi_edge_type* edge1_2 = edge1_1->twin();
  BOOST_CHECK(edge1_1->cell()->source_index() == 2);
  BOOST_CHECK(edge1_2->cell()->source_index() == 1);

  const voronoi_edge_type* edge2_1 = edge1_1->rot_prev();
  const voronoi_edge_type* edge2_2 = edge2_1->twin();
  BOOST_CHECK(edge2_1->cell()->source_index() == 1);
  BOOST_CHECK(edge2_2->cell()->source_index() == 0);

  const voronoi_edge_type* edge3_1 = edge2_1->rot_prev();
  const voronoi_edge_type* edge3_2 = edge3_1->twin();
  BOOST_CHECK(edge3_1->cell()->source_index() == 0);
  BOOST_CHECK(edge3_2->cell()->source_index() == 2);

  BOOST_CHECK(edge1_2->twin() == edge1_1);
  BOOST_CHECK(edge2_2->twin() == edge2_1);
  BOOST_CHECK(edge3_2->twin() == edge3_1);

  BOOST_CHECK(edge1_1->prev() == edge3_2 && edge1_1->next() == edge3_2);
  BOOST_CHECK(edge2_1->prev() == edge1_2 && edge2_1->next() == edge1_2);
  BOOST_CHECK(edge3_1->prev() == edge2_2 && edge3_1->next() == edge2_2);

  BOOST_CHECK(edge1_2->next() == edge2_1 && edge1_2->prev() == edge2_1);
  BOOST_CHECK(edge2_2->next() == edge3_1 && edge2_2->prev() == edge3_1);
  BOOST_CHECK(edge3_2->next() == edge1_1 && edge3_2->prev() == edge1_1);

  BOOST_CHECK(edge1_1->rot_next() == edge3_1);
  BOOST_CHECK(edge3_1->rot_next() == edge2_1);
  BOOST_CHECK(edge2_1->rot_next() == edge1_1);

  BOOST_CHECK(edge1_2->rot_next() == edge2_2);
  BOOST_CHECK(edge2_2->rot_next() == edge3_2);
  BOOST_CHECK(edge3_2->rot_next() == edge1_2);
}

// Sites: (0, 0), (0, 1), (1, 0), (1, 1).
BOOST_AUTO_TEST_CASE_TEMPLATE(square_test1, T, test_types) {
  point_data<T> point1(0, 0);
  point_data<T> point2(0, 1);
  point_data<T> point3(1, 0);
  point_data<T> point4(1, 1);
  std::vector< point_data<T> > points;
  points.push_back(point1);
  points.push_back(point2);
  points.push_back(point3);
  points.push_back(point4);
  vd_type test_output;
  construct_voronoi(points.begin(), points.end(), &test_output);
  VERIFY_OUTPUT(test_output);
  CHECK_OUTPUT_SIZE(test_output, 4, 1, 8);

  // Check voronoi vertex.
  const_vertex_iterator it = test_output.vertices().begin();
  BOOST_CHECK_EQUAL(it->x(), 0.5);
  BOOST_CHECK_EQUAL(it->y(), 0.5);

  // Check voronoi edges.
  const voronoi_edge_type* edge1_1 = it->incident_edge();
  const voronoi_edge_type* edge1_2 = edge1_1->twin();
  BOOST_CHECK(edge1_1->cell()->source_index() == 3);
  BOOST_CHECK(edge1_2->cell()->source_index() == 2);

  const voronoi_edge_type* edge2_1 = edge1_1->rot_prev();
  const voronoi_edge_type* edge2_2 = edge2_1->twin();
  BOOST_CHECK(edge2_1->cell()->source_index() == 2);
  BOOST_CHECK(edge2_2->cell()->source_index() == 0);

  const voronoi_edge_type* edge3_1 = edge2_1->rot_prev();
  const voronoi_edge_type* edge3_2 = edge3_1->twin();
  BOOST_CHECK(edge3_1->cell()->source_index() == 0);
  BOOST_CHECK(edge3_2->cell()->source_index() == 1);

  const voronoi_edge_type* edge4_1 = edge3_1->rot_prev();
  const voronoi_edge_type* edge4_2 = edge4_1->twin();
  BOOST_CHECK(edge4_1->cell()->source_index() == 1);
  BOOST_CHECK(edge4_2->cell()->source_index() == 3);

  BOOST_CHECK(edge1_2->twin() == edge1_1);
  BOOST_CHECK(edge2_2->twin() == edge2_1);
  BOOST_CHECK(edge3_2->twin() == edge3_1);
  BOOST_CHECK(edge4_2->twin() == edge4_1);

  BOOST_CHECK(edge1_1->prev() == edge4_2 && edge1_1->next() == edge4_2);
  BOOST_CHECK(edge2_1->prev() == edge1_2 && edge2_1->next() == edge1_2);
  BOOST_CHECK(edge3_1->prev() == edge2_2 && edge3_1->next() == edge2_2);
  BOOST_CHECK(edge4_1->prev() == edge3_2 && edge4_1->next() == edge3_2);

  BOOST_CHECK(edge1_2->next() == edge2_1 && edge1_2->prev() == edge2_1);
  BOOST_CHECK(edge2_2->next() == edge3_1 && edge2_2->prev() == edge3_1);
  BOOST_CHECK(edge3_2->next() == edge4_1 && edge3_2->prev() == edge4_1);
  BOOST_CHECK(edge4_2->next() == edge1_1 && edge4_2->prev() == edge1_1);

  BOOST_CHECK(edge1_1->rot_next() == edge4_1);
  BOOST_CHECK(edge4_1->rot_next() == edge3_1);
  BOOST_CHECK(edge3_1->rot_next() == edge2_1);
  BOOST_CHECK(edge2_1->rot_next() == edge1_1);

  BOOST_CHECK(edge1_2->rot_next() == edge2_2);
  BOOST_CHECK(edge2_2->rot_next() == edge3_2);
  BOOST_CHECK(edge3_2->rot_next() == edge4_2);
  BOOST_CHECK(edge4_2->rot_next() == edge1_2);
}

#ifdef NDEBUG
BOOST_AUTO_TEST_CASE_TEMPLATE(grid_test, T, test_types) {
  vd_type test_output_small, test_output_large;
  std::vector< point_data<T> > point_vec_small, point_vec_large;
  int grid_size[] = {10, 33, 101};
  int max_value[] = {10, 33, 101};
  int array_length = sizeof(grid_size) / sizeof(int);
  for (int k = 0; k < array_length; k++) {
    test_output_small.clear();
    test_output_large.clear();
    point_vec_small.clear();
    point_vec_large.clear();
    int koef = (std::numeric_limits<int>::max)() / max_value[k];
    for (int i = 0; i < grid_size[k]; i++) {
      for (int j = 0; j < grid_size[k]; j++) {
        point_vec_small.push_back(point_data<T>(i, j));
        point_vec_large.push_back(point_data<T>(koef * i, koef * j));
      }
    }
    construct_voronoi(point_vec_small.begin(), point_vec_small.end(), &test_output_small);
    construct_voronoi(point_vec_large.begin(), point_vec_large.end(), &test_output_large);
    VERIFY_OUTPUT(test_output_small);
    VERIFY_OUTPUT(test_output_large);
    unsigned int num_cells = grid_size[k] * grid_size[k];
    unsigned int num_vertices = num_cells - 2 * grid_size[k] + 1;
    unsigned int num_edges = 4 * num_cells - 4 * grid_size[k];
    CHECK_OUTPUT_SIZE(test_output_small, num_cells, num_vertices, num_edges);
    CHECK_OUTPUT_SIZE(test_output_large, num_cells, num_vertices, num_edges);
  }
}
#endif

#ifdef NDEBUG
BOOST_AUTO_TEST_CASE_TEMPLATE(random_test, T, test_types) {
  boost::mt19937 gen(static_cast<unsigned int>(time(NULL)));
  vd_type test_output_small, test_output_large;
  std::vector< point_data<T> > point_vec_small, point_vec_large;
  int num_points[] = {10, 100, 1000, 10000};
  int num_runs[] = {1000, 100, 10, 1};
  int mod_koef[] = {10, 100, 100, 1000};
  int max_value[] = {5, 50, 50, 5000};
  int array_length = sizeof(num_points) / sizeof(int);
  for (int k = 0; k < array_length; k++) {
    int koef = (std::numeric_limits<int>::max)() / max_value[k];
    for (int i = 0; i < num_runs[k]; i++) {
      test_output_small.clear();
      test_output_large.clear();
      point_vec_small.clear();
      point_vec_large.clear();
      for (int j = 0; j < num_points[k]; j++) {
        T x = gen() % mod_koef[k] - mod_koef[k] / 2;
        T y = gen() % mod_koef[k] - mod_koef[k] / 2;
        point_vec_small.push_back(point_data<T>(x, y));
        point_vec_large.push_back(point_data<T>(koef * x, koef * y));
      }
      construct_voronoi(point_vec_small.begin(), point_vec_small.end(), &test_output_small);
      construct_voronoi(point_vec_large.begin(), point_vec_large.end(), &test_output_large);
      VERIFY_OUTPUT(test_output_small);
      VERIFY_OUTPUT(test_output_large);
      BOOST_CHECK_EQUAL(test_output_small.num_cells(), test_output_large.num_cells());
      BOOST_CHECK_EQUAL(test_output_small.num_vertices(), test_output_large.num_vertices());
      BOOST_CHECK_EQUAL(test_output_small.num_edges(), test_output_large.num_edges());
    }
  }
}
#endif

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_sites_test1, T, test_types) {
  vd_type test_output;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(0, 0);
  point_data<T> point2(1, 1);
  segments.push_back(segment_data<T>(point1, point2));
  construct_voronoi(segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 3, 0, 4);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_sites_test2, T, test_types) {
  vd_type test_output;
  std::vector< point_data<T> > points;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(0, 0);
  point_data<T> point2(4, 4);
  point_data<T> point3(3, 1);
  point_data<T> point4(1, 3);
  segments.push_back(segment_data<T>(point1, point2));
  points.push_back(point3);
  points.push_back(point4);
  construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 5, 4, 16);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_sites_test3, T, test_types) {
  vd_type test_output;
  std::vector< point_data<T> > points;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(4, 0);
  point_data<T> point2(0, 4);
  point_data<T> point3(3, 3);
  point_data<T> point4(1, 1);
  segments.push_back(segment_data<T>(point1, point2));
  points.push_back(point3);
  points.push_back(point4);
  construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 5, 4, 16);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_sites_test4, T, test_types) {
  vd_type test_output;
  std::vector< point_data<T> > points;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(4, 0);
  point_data<T> point2(0, 4);
  point_data<T> point3(3, 2);
  point_data<T> point4(2, 3);
  segments.push_back(segment_data<T>(point1, point2));
  points.push_back(point3);
  points.push_back(point4);
  construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 5, 3, 14);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_site_test5, T, test_types) {
  vd_type test_output;
  std::vector< point_data<T> > points;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(0, 0);
  point_data<T> point2(0, 8);
  point_data<T> point3(-2, -2);
  point_data<T> point4(-2, 4);
  point_data<T> point5(-2, 10);
  segments.push_back(segment_data<T>(point1, point2));
  points.push_back(point3);
  points.push_back(point4);
  points.push_back(point5);
  construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 6, 4, 18);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_site_test6, T, test_types) {
  vd_type test_output;
  std::vector< point_data<T> > points;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(-1, 1);
  point_data<T> point2(1, 0);
  point_data<T> point3(1, 2);
  segments.push_back(segment_data<T>(point2, point3));
  points.push_back(point1);
  construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 4, 2, 10);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_site_test7, T, test_types) {
  vd_type test_output;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(0, 0);
  point_data<T> point2(4, 0);
  point_data<T> point3(0, 4);
  point_data<T> point4(4, 4);
  segments.push_back(segment_data<T>(point1, point2));
  segments.push_back(segment_data<T>(point2, point3));
  segments.push_back(segment_data<T>(point3, point4));
  construct_voronoi(segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 7, 6, 24);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_site_test8, T, test_types) {
  vd_type test_output;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(0, 0);
  point_data<T> point2(4, 0);
  point_data<T> point3(4, 4);
  point_data<T> point4(0, 4);
  segments.push_back(segment_data<T>(point1, point2));
  segments.push_back(segment_data<T>(point2, point3));
  segments.push_back(segment_data<T>(point3, point4));
  segments.push_back(segment_data<T>(point4, point1));
  construct_voronoi(segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 8, 5, 24);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(segment_site_test9, T, test_types) {
  vd_type test_output;
  std::vector< segment_data<T> > segments;
  point_data<T> point1(0, 0);
  point_data<T> point2(2, 0);
  point_data<T> point3(4, 0);
  segments.push_back(segment_data<T>(point1, point2));
  segments.push_back(segment_data<T>(point2, point3));
  construct_voronoi(segments.begin(), segments.end(), &test_output);
  CHECK_OUTPUT_SIZE(test_output, 5, 0, 8);
  VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
}

#ifdef NDEBUG
BOOST_AUTO_TEST_CASE_TEMPLATE(segment_grid_test, T, test_types) {
  vd_type test_output_small, test_output_large;
  std::vector< segment_data<T> > segments_small, segments_large;
  int grid_size[] = {10, 27, 53};
  int max_value[] = {100, 330, 1000};
  int array_length = sizeof(grid_size) / sizeof(int);
  for (int k = 0; k < array_length; k++) {
    test_output_small.clear();
    test_output_large.clear();
    segments_small.clear();
    segments_large.clear();
    int cur_sz = grid_size[k];
    int koef = (std::numeric_limits<int>::max)() / max_value[k];
    for (int i = 0; i < cur_sz + 1; i++)
      for (int j = 0; j < cur_sz; j++) {
        point_data<T> point1_1(10 * i, 10 * j);
        point_data<T> point1_2(koef * 10 * i, koef * 10 * j);
        point_data<T> point2_1(10 * i, 10 * j + 10);
        point_data<T> point2_2(koef * 10 * i, koef * (10 * j + 10));
        segments_small.push_back(segment_data<T>(point1_1, point2_1));
        segments_large.push_back(segment_data<T>(point1_2, point2_2));
        point_data<T> point3_1(10 * j, 10 * i);
        point_data<T> point3_2(koef * 10 * j, koef * 10 * i);
        point_data<T> point4_1(10 * j + 10, 10 * i);
        point_data<T> point4_2(koef * (10 * j + 10), koef * 10 * i);
        segments_small.push_back(segment_data<T>(point3_1, point4_1));
        segments_large.push_back(segment_data<T>(point3_2, point4_2));
      }
    construct_voronoi(segments_small.begin(), segments_small.end(), &test_output_small);
    construct_voronoi(segments_large.begin(), segments_large.end(), &test_output_large);
    VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output_small);
    VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output_large);
    BOOST_CHECK_EQUAL(test_output_small.num_cells(), test_output_large.num_cells());
    BOOST_CHECK_EQUAL(test_output_small.num_vertices(), test_output_large.num_vertices());
    BOOST_CHECK_EQUAL(test_output_small.num_edges(), test_output_large.num_edges());
  }
}
#endif

#ifdef NDEBUG
BOOST_AUTO_TEST_CASE_TEMPLATE(segment_random_test1, T, test_types) {
  boost::mt19937 gen(static_cast<unsigned int>(time(NULL)));
  vd_type test_output;
  std::vector< point_data<T> > points;
  std::vector< segment_data<T> > segments;
  int num_runs = 1000;
  int num_segments = 10;
  points.push_back(point_data<T>(-100, -100));
  points.push_back(point_data<T>(-100, 100));
  points.push_back(point_data<T>(100, -100));
  points.push_back(point_data<T>(100, 100));
  for (int i = 0; i < num_runs; i++) {
    test_output.clear();
    segments.clear();
    for (int j = 0; j < num_segments; j++) {
      T x1 = 0, y1 = 0, x2 = 0, y2 = 0;
      while (x1 == x2 && y1 == y2) {
        x1 = (gen() % 100) - 50;
        y1 = (gen() % 100) - 50;
        x2 = (gen() % 100) - 50;
        y2 = (gen() % 100) - 50;
      }
      point_data<T> point1(x1, y1);
      point_data<T> point2(x2, y2);
      segments.push_back(segment_data<T>(point1, point2));
    }
    voronoi_test_helper::clean_segment_set(segments);
    construct_voronoi(points.begin(), points.end(), segments.begin(), segments.end(), &test_output);
    VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output);
  }
}
#endif

#ifdef NDEBUG
BOOST_AUTO_TEST_CASE_TEMPLATE(segment_random_test2, T, test_types) {
  boost::mt19937 gen(static_cast<unsigned int>(time(NULL)));
  vd_type test_output_small, test_output_large;
  std::vector< segment_data<T> > segments_small, segments_large;
  int num_segments[] = {5, 25, 125, 625};
  int num_runs[] = {1000, 100, 10, 1};
  int mod_koef1[] = {10, 100, 200, 300};
  int mod_koef2[] = {10, 20, 50, 100};
  int max_value[] = {10, 60, 125, 200};
  int array_length = sizeof(num_segments) / sizeof(int);
  for (int k = 0; k < array_length; k++) {
    int koef = (std::numeric_limits<int>::max)() / max_value[k];
    for (int i = 0; i < num_runs[k]; i++) {
      test_output_small.clear();
      test_output_large.clear();
      segments_small.clear();
      segments_large.clear();
      for (int j = 0; j < num_segments[k]; j++) {
        T x1 = (gen() % mod_koef1[k]) - mod_koef1[k] / 2;
        T y1 = (gen() % mod_koef1[k]) - mod_koef1[k] / 2;
        T dx = 0, dy = 0;
        while (dx == 0 && dy == 0) {
          dx = (gen() % mod_koef2[k]) - mod_koef2[k] / 2;
          dy = (gen() % mod_koef2[k]) - mod_koef2[k] / 2;
        }
        T x2 = x1 + dx;
        T y2 = y1 + dy;
        point_data<T> point1_small(x1, y1);
        point_data<T> point2_small(x2, y2);
        segments_small.push_back(segment_data<T>(point1_small, point2_small));
      }
      voronoi_test_helper::clean_segment_set(segments_small);
      for (typename std::vector< segment_data<T> >::iterator it = segments_small.begin();
           it != segments_small.end(); ++it) {
        T x1 = it->low().x() * koef;
        T y1 = it->low().y() * koef;
        T x2 = it->high().x() * koef;
        T y2 = it->high().y() * koef;
        point_data<T> point1_large(x1, y1);
        point_data<T> point2_large(x2, y2);
        segments_large.push_back(segment_data<T>(point1_large, point2_large));
      }
      construct_voronoi(segments_small.begin(), segments_small.end(), &test_output_small);
      construct_voronoi(segments_large.begin(), segments_large.end(), &test_output_large);
      VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output_small);
      VERIFY_NO_HALF_EDGE_INTERSECTIONS(test_output_large);
      BOOST_CHECK_EQUAL(test_output_small.num_cells(), test_output_large.num_cells());
      BOOST_CHECK_EQUAL(test_output_small.num_vertices(), test_output_large.num_vertices());
      BOOST_CHECK_EQUAL(test_output_small.num_edges(), test_output_large.num_edges());
    }
  }
}
#endif

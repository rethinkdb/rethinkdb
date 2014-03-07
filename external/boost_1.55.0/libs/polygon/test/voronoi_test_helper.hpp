// Boost.Polygon library voronoi_test_helper.hpp file

//          Copyright Andrii Sydorchuk 2010-2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#ifndef VORONOI_TEST_HELPER
#define VORONOI_TEST_HELPER

#include <algorithm>
#include <iostream>
#include <iterator>
#include <fstream>
#include <map>
#include <vector>
#include <utility>

#include <boost/polygon/polygon.hpp>
using namespace boost::polygon;

namespace voronoi_test_helper {

enum kOrientation {
  RIGHT = -1,
  COLLINEAR = 0,
  LEFT = 1
};

template <typename VERTEX>
kOrientation get_orientation(
    const VERTEX& v1, const VERTEX& v2, const VERTEX& v3) {
  typename VERTEX::coordinate_type lhs = (v2.x() - v1.x()) * (v3.y() - v2.y());
  typename VERTEX::coordinate_type rhs = (v2.y() - v1.y()) * (v3.x() - v2.x());
  if (lhs == rhs) {
    return COLLINEAR;
  }
  return (lhs < rhs) ? RIGHT : LEFT;
}

template <typename OUTPUT>
bool verify_cell_convexity(const OUTPUT& output) {
  typename OUTPUT::const_cell_iterator cell_it;
  for (cell_it = output.cells().begin();
       cell_it != output.cells().end(); cell_it++) {
    const typename OUTPUT::edge_type* edge = cell_it->incident_edge();
    if (edge)
      do {
        if (edge->next()->prev() != edge) {
          return false;
        }
        if (edge->cell() != &(*cell_it)) {
          return false;
        }
        if (edge->vertex1() != edge->next()->vertex0()) {
          return false;
        }
        if (edge->vertex0() != NULL &&
            edge->vertex1() != NULL &&
            edge->next()->vertex1() != NULL) {
          if (get_orientation(*edge->vertex0(),
                              *edge->vertex1(),
                              *edge->next()->vertex1()) != LEFT) {
            return false;
          }
        }
        edge = edge->next();
      } while (edge != cell_it->incident_edge());
  }
  return true;
}

template <typename OUTPUT>
bool verify_incident_edges_ccw_order(const OUTPUT& output) {
  typedef typename OUTPUT::edge_type voronoi_edge_type;
  typename OUTPUT::const_vertex_iterator vertex_it;
  for (vertex_it = output.vertices().begin();
       vertex_it != output.vertices().end(); vertex_it++) {
    if (vertex_it->is_degenerate())
      continue;
    const voronoi_edge_type* edge = vertex_it->incident_edge();
    do {
      const voronoi_edge_type* next_edge = edge->rot_next();
      if (edge->vertex0() != next_edge->vertex0()) {
        return false;
      }
      if (edge->vertex1() != NULL && next_edge->vertex1() != NULL &&
          get_orientation(*edge->vertex1(),
                          *edge->vertex0(),
                          *next_edge->vertex1()) == LEFT) {
        return false;
      }
      edge = edge->rot_next();
    } while (edge != vertex_it->incident_edge());
  }
  return true;
}

template <typename VERTEX>
struct cmp {
  bool operator()(const VERTEX& v1, const VERTEX& v2) const {
    if (v1.x() != v2.x())
      return v1.x() < v2.x();
    return v1.y() < v2.y();
  }
};

template <typename Output>
bool verfiy_no_line_edge_intersections(const Output &output) {
  // Create map from edges with first point less than the second one.
  // Key is the first point of the edge, value is a vector of second points
  // with the same first point.
  typedef typename Output::vertex_type vertex_type;
  cmp<vertex_type> comparator;
  std::map< vertex_type, std::vector<vertex_type>, cmp<vertex_type> > edge_map;
  typename Output::const_edge_iterator edge_it;
  for (edge_it = output.edges().begin();
       edge_it != output.edges().end(); edge_it++) {
    if (edge_it->is_finite()) {
      if (comparator(*edge_it->vertex0(), *edge_it->vertex1())) {
        edge_map[*edge_it->vertex0()].push_back(*edge_it->vertex1());
      }
    }
  }
  return !intersection_check(edge_map);
}

template <typename Point2D>
bool intersection_check(
    const std::map< Point2D, std::vector<Point2D>, cmp<Point2D> > &edge_map) {
  // Iterate over map of edges and check if there are any intersections.
  // All the edges are stored by the low x value. That's why we iterate
  // left to right checking for intersections between all pairs of edges
  // that overlap in the x dimension.
  // Complexity. Approximately N*sqrt(N). Worst case N^2.
  typedef Point2D point_type;
  typedef typename point_type::coordinate_type coordinate_type;
  typedef typename std::map<point_type, std::vector<point_type>, cmp<Point2D> >::const_iterator
      edge_map_iterator;
  typedef typename std::vector<point_type>::size_type size_type;
  edge_map_iterator edge_map_it1, edge_map_it2, edge_map_it_bound;
  for (edge_map_it1 = edge_map.begin();
       edge_map_it1 != edge_map.end(); edge_map_it1++) {
    const point_type &point1 = edge_map_it1->first;
    for (size_type i = 0; i < edge_map_it1->second.size(); i++) {
      const point_type &point2 = edge_map_it1->second[i];
      coordinate_type min_y1 = (std::min)(point1.y(), point2.y());
      coordinate_type max_y1 = (std::max)(point1.y(), point2.y());

      // Find the first edge with greater or equal first point.
      edge_map_it_bound = edge_map.lower_bound(point2);

      edge_map_it2 = edge_map_it1;
      edge_map_it2++;
      for (; edge_map_it2 != edge_map_it_bound; edge_map_it2++) {
        const point_type &point3 = edge_map_it2->first;
        for (size_type j = 0; j < edge_map_it2->second.size(); j++) {
          const point_type &point4 = edge_map_it2->second[j];
          coordinate_type min_y2 = (std::min)(point3.y(), point4.y());
          coordinate_type max_y2 = (std::max)(point3.y(), point4.y());

          // In most cases it is enought to make
          // simple intersection check in the y dimension.
          if (!(max_y1 > min_y2 && max_y2 > min_y1))
            continue;

          // Intersection check.
          if (get_orientation(point1, point2, point3) *
              get_orientation(point1, point2, point4) == RIGHT &&
              get_orientation(point3, point4, point1) *
              get_orientation(point3, point4, point2) == RIGHT)
            return true;
        }
      }
    }
  }
  return false;
}

enum kVerification {
  CELL_CONVEXITY = 1,
  INCIDENT_EDGES_CCW_ORDER = 2,
  NO_HALF_EDGE_INTERSECTIONS = 4,
  FAST_VERIFICATION = 3,
  COMPLETE_VERIFICATION = 7
};

template <typename Output>
bool verify_output(const Output &output, kVerification mask) {
  bool result = true;
  if (mask & CELL_CONVEXITY)
    result &= verify_cell_convexity(output);
  if (mask & INCIDENT_EDGES_CCW_ORDER)
    result &= verify_incident_edges_ccw_order(output);
  if (mask & NO_HALF_EDGE_INTERSECTIONS)
    result &= verfiy_no_line_edge_intersections(output);
  return result;
}

template <typename PointIterator>
void save_points(
    PointIterator first, PointIterator last, const char* file_name) {
  std::ofstream ofs(file_name);
  ofs << std::distance(first, last) << std::endl;
  for (PointIterator it = first; it != last; ++it) {
    ofs << it->x() << " " << it->y() << std::endl;
  }
  ofs.close();
}

template <typename SegmentIterator>
void save_segments(
     SegmentIterator first, SegmentIterator last, const char* file_name) {
  std::ofstream ofs(file_name);
  ofs << std::distance(first, last) << std::endl;
  for (SegmentIterator it = first; it != last; ++it) {
    ofs << it->low().x() << " " << it->low().y() << " ";
    ofs << it->high().x() << " " << it->high().y() << std::endl;
  }
  ofs.close();
}

template <typename T>
void clean_segment_set(std::vector< segment_data<T> >& data) {
  typedef T Unit;
  typedef typename scanline_base<Unit>::Point Point;
  typedef typename scanline_base<Unit>::half_edge half_edge;
  typedef int segment_id;
  std::vector<std::pair<half_edge, segment_id> > half_edges;
  std::vector<std::pair<half_edge, segment_id> > half_edges_out;
  segment_id id = 0;
  half_edges.reserve(data.size());
  for (typename std::vector< segment_data<T> >::iterator it = data.begin();
       it != data.end(); ++it) {
    Point l = it->low();
    Point h = it->high();
    half_edges.push_back(std::make_pair(half_edge(l, h), id++));
  }
  half_edges_out.reserve(half_edges.size());
  // Apparently no need to pre-sort data when calling validate_scan.
  line_intersection<Unit>::validate_scan(
      half_edges_out, half_edges.begin(), half_edges.end());
  std::vector< segment_data<T> > result;
  result.reserve(half_edges_out.size());
  for (std::size_t i = 0; i < half_edges_out.size(); ++i) {
    id = half_edges_out[i].second;
    Point l = half_edges_out[i].first.first;
    Point h = half_edges_out[i].first.second;
    segment_data<T> orig_seg = data[id];
    if (orig_seg.high() < orig_seg.low())
      std::swap(l, h);
    result.push_back(segment_data<T>(l, h));
  }
  std::swap(result, data);
}
}  // voronoi_test_helper

#endif

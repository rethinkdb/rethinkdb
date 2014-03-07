/*
Copyright 2010 Intel Corporation

Use, modification and distribution are subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt).
*/
#include <iostream>
#include <boost/polygon/polygon.hpp>

typedef boost::polygon::point_data<int> point;
typedef boost::polygon::polygon_set_data<int> polygon_set;
typedef boost::polygon::polygon_with_holes_data<int> polygon;
typedef std::pair<point, point> edge;
using namespace boost::polygon::operators;

void convolve_two_segments(std::vector<point>& figure, const edge& a, const edge& b) {
  using namespace boost::polygon;
  figure.clear();
  figure.push_back(point(a.first));
  figure.push_back(point(a.first));
  figure.push_back(point(a.second));
  figure.push_back(point(a.second));
  convolve(figure[0], b.second);
  convolve(figure[1], b.first);
  convolve(figure[2], b.first);
  convolve(figure[3], b.second);
}

template <typename itrT1, typename itrT2>
void convolve_two_point_sequences(polygon_set& result, itrT1 ab, itrT1 ae, itrT2 bb, itrT2 be) {
  using namespace boost::polygon;
  if(ab == ae || bb == be)
    return;
  point first_a = *ab;
  point prev_a = *ab;
  std::vector<point> vec;
  polygon poly;
  ++ab;
  for( ; ab != ae; ++ab) {
    point first_b = *bb;
    point prev_b = *bb;
    itrT2 tmpb = bb;
    ++tmpb;
    for( ; tmpb != be; ++tmpb) {
      convolve_two_segments(vec, std::make_pair(prev_b, *tmpb), std::make_pair(prev_a, *ab));
      set_points(poly, vec.begin(), vec.end());
      result.insert(poly);
      prev_b = *tmpb;
    }
    prev_a = *ab;
  }
}

template <typename itrT>
void convolve_point_sequence_with_polygons(polygon_set& result, itrT b, itrT e, const std::vector<polygon>& polygons) {
  using namespace boost::polygon;
  for(std::size_t i = 0; i < polygons.size(); ++i) {
    convolve_two_point_sequences(result, b, e, begin_points(polygons[i]), end_points(polygons[i]));
    for(polygon_with_holes_traits<polygon>::iterator_holes_type itrh = begin_holes(polygons[i]);
        itrh != end_holes(polygons[i]); ++itrh) {
      convolve_two_point_sequences(result, b, e, begin_points(*itrh), end_points(*itrh));
    }
  }
}

void convolve_two_polygon_sets(polygon_set& result, const polygon_set& a, const polygon_set& b) {
  using namespace boost::polygon;
  result.clear();
  std::vector<polygon> a_polygons;
  std::vector<polygon> b_polygons;
  a.get(a_polygons);
  b.get(b_polygons);
  for(std::size_t ai = 0; ai < a_polygons.size(); ++ai) {
    convolve_point_sequence_with_polygons(result, begin_points(a_polygons[ai]), 
                                          end_points(a_polygons[ai]), b_polygons);
    for(polygon_with_holes_traits<polygon>::iterator_holes_type itrh = begin_holes(a_polygons[ai]);
        itrh != end_holes(a_polygons[ai]); ++itrh) {
      convolve_point_sequence_with_polygons(result, begin_points(*itrh), 
                                            end_points(*itrh), b_polygons);
    }
    for(std::size_t bi = 0; bi < b_polygons.size(); ++bi) {
      polygon tmp_poly = a_polygons[ai];
      result.insert(convolve(tmp_poly, *(begin_points(b_polygons[bi]))));
      tmp_poly = b_polygons[bi];
      result.insert(convolve(tmp_poly, *(begin_points(a_polygons[ai]))));
    }
  }
}

namespace boost { namespace polygon{

  template <typename T>
  std::ostream& operator<<(std::ostream& o, const polygon_data<T>& poly) {
    o << "Polygon { ";
    for(typename polygon_data<T>::iterator_type itr = poly.begin(); 
        itr != poly.end(); ++itr) {
      if(itr != poly.begin()) o << ", ";
      o << (*itr).get(HORIZONTAL) << " " << (*itr).get(VERTICAL);
    } 
    o << " } ";
    return o;
  } 

  template <typename T>
  std::ostream& operator<<(std::ostream& o, const polygon_with_holes_data<T>& poly) {
    o << "Polygon With Holes { ";
    for(typename polygon_with_holes_data<T>::iterator_type itr = poly.begin(); 
        itr != poly.end(); ++itr) {
      if(itr != poly.begin()) o << ", ";
      o << (*itr).get(HORIZONTAL) << " " << (*itr).get(VERTICAL);
    } o << " { ";
    for(typename polygon_with_holes_data<T>::iterator_holes_type itr = poly.begin_holes();
        itr != poly.end_holes(); ++itr) {
      o << (*itr);
    }
    o << " } } ";
    return o;
  }
}}

int main(int argc, char **argv) {
  polygon_set a, b, c;
  a += boost::polygon::rectangle_data<int>(0, 0, 1000, 1000);
  a -= boost::polygon::rectangle_data<int>(100, 100, 900, 900);
  a += boost::polygon::rectangle_data<int>(1000, -1000, 1010, -990);
  std::vector<polygon> polys;
  std::vector<point> pts;
  pts.push_back(point(-40, 0));
  pts.push_back(point(-10, 10));
  pts.push_back(point(0, 40));
  pts.push_back(point(10, 10));
  pts.push_back(point(40, 0));
  pts.push_back(point(10, -10));
  pts.push_back(point(0, -40));
  pts.push_back(point(-10, -10));
  pts.push_back(point(-40, 0));
  polygon poly;
  boost::polygon::set_points(poly, pts.begin(), pts.end());
  b+=poly;
  pts.clear();
  pts.push_back(point(1040, 1040));
  pts.push_back(point(1050, 1045));
  pts.push_back(point(1045, 1050));
  boost::polygon::set_points(poly, pts.begin(), pts.end());
  b+=poly;
  polys.clear();
  convolve_two_polygon_sets(c, a, b);
  c.get(polys);
  for(int i = 0; i < polys.size(); ++i ){
    std::cout << polys[i] << std::endl;
  }
  return 0;
}

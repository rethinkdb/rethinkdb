/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#include <iostream>
#define BOOST_POLYGON_NO_DEPS
#include <boost/polygon/polygon.hpp>

namespace gtl = boost::polygon;
using namespace boost::polygon::operators;
#include <time.h>
#include <stdlib.h>

void assert_s(bool c, std::string msg) {
  if(!c) {
    std::cout << msg << std::endl;
    exit( 1);
  }
}

namespace boost { namespace polygon{
  void addpoly(polygon_45_set_data<int>& pset,
               int* pts, unsigned int numpts) {
    std::vector<point_data<int> > mppts;
    for(unsigned int i = 0; i < numpts*2; i += 2) {
      point_data<int>  pt(pts[i], pts[i+1]);
      mppts.push_back(pt);
    }
    polygon_45_data<int> poly;
    poly.set(mppts.begin(), mppts.end());
    pset += poly;
  }

  template <class T>
  std::ostream& operator << (std::ostream& o, const interval_data<T>& i)
  {
    return o << i.get(LOW) << ' ' << i.get(HIGH);
  }
  template <class T>
  std::ostream& operator << (std::ostream& o, const point_data<T>& r)
  {
    return o << r.get(HORIZONTAL) << ' ' << r.get(VERTICAL);
  }
  template <typename T>
  std::ostream& operator<<(std::ostream& o, const polygon_45_data<T>& poly) {
    o << "Polygon { ";
    for(typename polygon_45_data<T>::iterator_type itr = poly.begin();
        itr != poly.end(); ++itr) {
      if(itr != poly.begin()) o << ", ";
      o << (*itr).get(HORIZONTAL) << " " << (*itr).get(VERTICAL);
    }
    o << " } ";
    return o;
  }
  template <typename Unit>
  inline std::ostream& operator<< (std::ostream& o, const polygon_45_set_data<Unit>& p) {
    o << "Polygon45Set ";
    o << " " << !p.sorted() << " " << p.dirty() << " { ";
    for(typename polygon_45_set_data<Unit>::iterator_type itr = p.begin();
        itr != p.end(); ++itr) {
      o << (*itr).pt << ":";
      for(unsigned int i = 0; i < 4; ++i) {
        o << (*itr).count[i] << ",";
      } o << " ";
      //o << (*itr).first << ":" <<  (*itr).second << "; ";
    }
    o << "} ";
    return o;
  }

  template <typename Unit>
  inline std::istream& operator>> (std::istream& i, polygon_45_set_data<Unit>& p) {
    //TODO
    return i;
  }
  template <typename T>
  std::ostream& operator << (std::ostream& o, const polygon_90_data<T>& r)
  {
    o << "Polygon { ";
    for(typename polygon_90_data<T>::iterator_type itr = r.begin(); itr != r.end(); ++itr) {
      o << *itr << ", ";
    }
    return o << "} ";
  }

  template <typename T>
  std::istream& operator >> (std::istream& i, polygon_90_data<T>& r)
  {
    std::size_t size;
    i >> size;
    std::vector<T> vec;
    vec.reserve(size);
    for(std::size_t ii = 0; ii < size; ++ii) {
      T coord;
      i >> coord;
      vec.push_back(coord);
    }
    r.set_compact(vec.begin(), vec.end());
    return i;
  }

  template <typename T>
  std::ostream& operator << (std::ostream& o, const std::vector<polygon_90_data<T> >& r) {
    o << r.size() << ' ';
    for(std::size_t ii = 0; ii < r.size(); ++ii) {
      o << (r[ii]);
    }
    return o;
  }
  template <typename T>
  std::istream& operator >> (std::istream& i, std::vector<polygon_90_data<T> >& r) {
    std::size_t size;
    i >> size;
    r.clear();
    r.reserve(size);
    for(std::size_t ii = 0; ii < size; ++ii) {
      polygon_90_data<T> tmp;
      i >> tmp;
      r.push_back(tmp);
    }
    return i;
  }
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
  std::ostream& operator << (std::ostream& o, const polygon_set_data<T>& r)
  {
    o << "Polygon Set Data { ";
    for(typename polygon_set_data<T>::iterator_type itr = r.begin(); itr != r.end(); ++itr) {
      o << "<" << (*itr).first.first << ", " << (*itr).first.second << ">:" << (*itr).second << " ";
    }
    o << "} ";
    return o;
  }
  template <typename T>
  std::ostream& operator<<(std::ostream& o, const polygon_90_with_holes_data<T>& poly) {
    o << "Polygon With Holes { ";
    for(typename polygon_90_with_holes_data<T>::iterator_type itr = poly.begin();
        itr != poly.end(); ++itr) {
      if(itr != poly.begin()) o << ", ";
      o << (*itr).get(HORIZONTAL) << " " << (*itr).get(VERTICAL);
    } o << " { ";
    for(typename polygon_90_with_holes_data<T>::iterator_holes_type itr = poly.begin_holes();
        itr != poly.end_holes(); ++itr) {
      o << (*itr);
    }
    o << " } } ";
    return o;
  }
  template <typename T>
  std::ostream& operator<<(std::ostream& o, const polygon_45_with_holes_data<T>& poly) {
    o << "Polygon With Holes { ";
    for(typename polygon_45_with_holes_data<T>::iterator_type itr = poly.begin();
        itr != poly.end(); ++itr) {
      if(itr != poly.begin()) o << ", ";
      o << (*itr).get(HORIZONTAL) << " " << (*itr).get(VERTICAL);
    } o << " { ";
    for(typename polygon_45_with_holes_data<T>::iterator_holes_type itr = poly.begin_holes();
        itr != poly.end_holes(); ++itr) {
      o << (*itr);
    }
    o << " } } ";
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
  template <class T>
  std::ostream& operator << (std::ostream& o, const rectangle_data<T>& r)
  {
    return o << r.get(HORIZONTAL) << ' ' << r.get(VERTICAL);
  }
  template <class T>
  std::ostream& operator << (std::ostream& o, const segment_data<T>& r)
  {
    return o << r.get(LOW) << ' ' << r.get(HIGH);
  }


  template <typename T>
  typename enable_if<typename is_polygon_90_set_type<T>::type, void>::type
  print_is_polygon_90_set_concept(const T& ) { std::cout << "is polygon 90 set concept\n"; }
  template <typename T>
  typename enable_if<typename is_mutable_polygon_90_set_type<T>::type, void>::type
  print_is_mutable_polygon_90_set_concept(const T& ) { std::cout << "is mutable polygon 90 set concept\n"; }
  namespace boolean_op {
    //self contained unit test for BooleanOr algorithm
    template <typename Unit>
    inline bool testBooleanOr() {
      BooleanOp<int, Unit> booleanOr;
      //test one rectangle
      std::vector<std::pair<interval_data<Unit>, int> > container;
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), -1);
      if(container.size() != 2) {
        std::cout << "Test one rectangle, wrong output size\n";
        return false;
      }
      if(container[0] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 10), 1)) {
        std::cout << "Test one rectangle, first output wrong: Interval(" <<
          container[0].first << "), " << container[0].second << std::endl;
      }
      if(container[1] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 10), -1)) {
        std::cout << "Test one rectangle, second output wrong: Interval(" <<
          container[1].first << "), " << container[1].second << std::endl;
      }

      //test two rectangles
      container.clear();
      booleanOr = BooleanOp<int, Unit>();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(5, 15), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), -1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(5, 15), -1);
      if(container.size() != 4) {
        std::cout << "Test two rectangles, wrong output size\n";
        for(std::size_t i = 0; i < container.size(); ++i){
          std::cout << container[i].first << "), " << container[i].second << std::endl;
        }
        return false;
      }
      if(container[0] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 10), 1)) {
        std::cout << "Test two rectangles, first output wrong: Interval(" <<
          container[0].first << "), " << container[0].second << std::endl;
      }
      if(container[1] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(10, 15), 1)) {
        std::cout << "Test two rectangles, second output wrong: Interval(" <<
          container[1].first << "), " << container[1].second << std::endl;
      }
      if(container[2] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 5), -1)) {
        std::cout << "Test two rectangles, third output wrong: Interval(" <<
          container[2].first << "), " << container[2].second << std::endl;
      }
      if(container[3] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(5, 15), -1)) {
        std::cout << "Test two rectangles, fourth output wrong: Interval(" <<
          container[3].first << "), " << container[3].second << std::endl;
      }

      //test two rectangles
      container.clear();
      booleanOr = BooleanOp<int, Unit>();
      booleanOr.processInterval(container, interval_data<Unit>(5, 15), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(5, 15), -1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), -1);
      if(container.size() != 4) {
        std::cout << "Test other two rectangles, wrong output size\n";
        for(std::size_t i = 0; i < container.size(); ++i){
          std::cout << container[i].first << "), " << container[i].second << std::endl;
        }
        return false;
      }
      if(container[0] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(5, 15), 1)) {
        std::cout << "Test other two rectangles, first output wrong: Interval(" <<
          container[0].first << "), " << container[0].second << std::endl;
      }
      if(container[1] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 5), 1)) {
        std::cout << "Test other two rectangles, second output wrong: Interval(" <<
          container[1].first << "), " << container[1].second << std::endl;
      }
      if(container[2] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(10, 15), -1)) {
        std::cout << "Test other two rectangles, third output wrong: Interval(" <<
          container[2].first << "), " << container[2].second << std::endl;
      }
      if(container[3] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 10), -1)) {
        std::cout << "Test other two rectangles, fourth output wrong: Interval(" <<
          container[3].first << "), " << container[3].second << std::endl;
      }

      //test two nonoverlapping rectangles
      container.clear();
      booleanOr = BooleanOp<int, Unit>();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(15, 25), 1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(0, 10), -1);
      booleanOr.advanceScan();
      booleanOr.processInterval(container, interval_data<Unit>(15, 25), -1);
      if(container.size() != 4) {
        std::cout << "Test two nonoverlapping rectangles, wrong output size\n";
        return false;
      }
      if(container[0] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 10), 1)) {
        std::cout << "Test two nonoverlapping rectangles, first output wrong: Interval(" <<
          container[0].first << "), " << container[0].second << std::endl;
      }
      if(container[1] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(15, 25), 1)) {
        std::cout << "Test two nonoverlapping rectangles, second output wrong: Interval(" <<
          container[1].first << "), " << container[1].second << std::endl;
      }
      if(container[2] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(0, 10), -1)) {
        std::cout << "Test two nonoverlapping rectangles, third output wrong: Interval(" <<
          container[2].first << "), " << container[2].second << std::endl;
      }
      if(container[3] != std::pair<interval_data<Unit>, int>(interval_data<Unit>(15, 25), -1)) {
        std::cout << "Test two nonoverlapping rectangles, fourth output wrong: Interval(" <<
          container[3].first << "), " << container[3].second << std::endl;
      }
      return true;
    }
  }

  void test_assign() {
    using namespace gtl;
    std::vector<polygon_data<int> > ps;
    polygon_90_set_data<int> ps90;
    assign(ps, ps90);
  }

  //this is a compile time test, if it compiles it passes
  void test_view_as() {
    using namespace gtl;
    polygon_data<int> p;
    polygon_45_data<int> p45;
    polygon_90_data<int> p90;
    polygon_with_holes_data<int> pwh;
    polygon_45_with_holes_data<int> p45wh;
    polygon_90_with_holes_data<int> p90wh;
    rectangle_data<int> rect(0, 1, 10, 11);
    polygon_90_set_data<int> ps90;
    polygon_45_set_data<int> ps45;
    polygon_set_data<int> ps;

    assign(p, rect);
    assign(p90, view_as<polygon_90_concept>(p));
    if(!equivalence(p90, rect))
      std::cout << "fail 1\n";
    assign(p45, view_as<polygon_45_concept>(p));
    if(!equivalence(p45, rect))
      std::cout << "fail 2\n";
    assign(p90, view_as<polygon_90_concept>(p45));
    if(!equivalence(p90, rect))
      std::cout << "fail 3\n";
    if(!equivalence(rect, view_as<rectangle_concept>(p)))
      std::cout << "fail 4\n";
    if(!equivalence(rect, view_as<rectangle_concept>(p45)))
      std::cout << "fail 5\n";
    if(!equivalence(rect, view_as<rectangle_concept>(p90)))
      std::cout << "fail 6\n";
    assign(pwh, rect);
    assign(p90wh, rect);
    assign(p45wh, rect);
    if(!equivalence(rect, view_as<rectangle_concept>(pwh)))
      std::cout << "fail 7\n";
    if(!equivalence(rect, view_as<rectangle_concept>(p45wh)))
      std::cout << "fail 8\n";
    if(!equivalence(rect, view_as<rectangle_concept>(p90wh)))
      std::cout << "fail 9\n";
    assign(p90wh, view_as<polygon_90_with_holes_concept>(pwh));
    if(!equivalence(p90wh, rect))
      std::cout << "fail 10\n";
    assign(p45wh, view_as<polygon_45_with_holes_concept>(pwh));
    if(!equivalence(p45wh, rect))
      std::cout << "fail 11\n";
    assign(p90wh, view_as<polygon_90_with_holes_concept>(p45wh));
    if(!equivalence(p90wh, rect))
      std::cout << "fail 12\n";
    assign(p90, view_as<polygon_90_concept>(pwh));
    if(!equivalence(p90, rect))
      std::cout << "fail 13\n";
    assign(p45, view_as<polygon_45_concept>(pwh));
    if(!equivalence(p45, rect))
      std::cout << "fail 14\n";
    assign(p90, view_as<polygon_90_concept>(p45wh));
    if(!equivalence(p90, rect))
      std::cout << "fail 15\n";
    assign(ps, rect);
    assign(ps90, view_as<polygon_90_set_concept>(ps));
    if(!equivalence(ps90, rect))
      std::cout << "fail 16\n";
    assign(ps45, view_as<polygon_45_set_concept>(ps));
    if(!equivalence(ps45, rect))
      std::cout << "fail 17\n";
    assign(ps90, view_as<polygon_90_set_concept>(ps45));
    if(!equivalence(ps90, rect))
      std::cout << "fail 18\n";
  }

  inline bool testPolygon45SetRect() {
    std::vector<point_data<int> > points;
    points.push_back(point_data<int>(0,0));
    points.push_back(point_data<int>(0,10));
    points.push_back(point_data<int>(10,10));
    points.push_back(point_data<int>(10,0));
    polygon_45_data<int> poly;
    poly.set(points.begin(), points.end());
    polygon_45_set_data<int> ps;
    ps.insert(poly);
    std::vector<polygon_45_data<int> > polys;
    ps.get_polygons(polys);
    std::cout << polys.size() << std::endl;
    for(unsigned int i = 0; i < polys.size(); ++i) {
      std::cout << polys[i] << std::endl;
    }
    return true;
  }

  inline bool testPolygon45Set() {
    polygon_45_formation<int>::Polygon45Formation pf(true);
    typedef boolean_op_45<int>::Vertex45 Vertex45;
    std::vector<Vertex45> data;
    // result == 0 8 -1 1
    data.push_back(Vertex45(point_data<int>(0, 8), -1, 1));
    // result == 0 8 1 -1
    data.push_back(Vertex45(point_data<int>(0, 8), 1, -1));
    // result == 4 0 1 1
    data.push_back(Vertex45(point_data<int>(4, 0), 1, 1));
    // result == 4 0 2 1
    data.push_back(Vertex45(point_data<int>(4, 0), 2, 1));
    // result == 4 4 2 -1
    data.push_back(Vertex45(point_data<int>(4, 4), 2, -1));
    // result == 4 4 -1 -1
    data.push_back(Vertex45(point_data<int>(4, 4), -1, -1));
    // result == 4 12 1 1
    data.push_back(Vertex45(point_data<int>(4, 12), 1, 1));
    // result == 4 12 2 1
    data.push_back(Vertex45(point_data<int>(4, 12), 2, 1));
    // result == 4 16 2 -1
    data.push_back(Vertex45(point_data<int>(4, 16), 2, 1));
    // result == 4 16 -1 -1
    data.push_back(Vertex45(point_data<int>(4, 16), -1, -1));
    // result == 6 2 1 -1
    data.push_back(Vertex45(point_data<int>(6, 2), 1, -1));
    // result == 6 14 -1 1
    data.push_back(Vertex45(point_data<int>(6, 14), -1, 1));
    // result == 6 2 -1 1
    data.push_back(Vertex45(point_data<int>(6, 2), -1, 1));
    // result == 6 14 1 -1
    data.push_back(Vertex45(point_data<int>(6, 14), 1, -1));
    // result == 8 0 -1 -1
    data.push_back(Vertex45(point_data<int>(8, 0), -1, -1));
    // result == 8 0 2 -1
    data.push_back(Vertex45(point_data<int>(8, 0), 2, -1));
    // result == 8 4 2 1
    data.push_back(Vertex45(point_data<int>(8, 4), 2, 1));
    // result == 8 4 1 1
    data.push_back(Vertex45(point_data<int>(8, 4), 1, 1));
    // result == 8 12 -1 -1
    data.push_back(Vertex45(point_data<int>(8, 12), -1, -1));
    // result == 8 12 2 -1
    data.push_back(Vertex45(point_data<int>(8, 12), 2, -1));
    // result == 8 16 2 1
    data.push_back(Vertex45(point_data<int>(8, 16), 2, 1));
    // result == 8 16 1 1
    data.push_back(Vertex45(point_data<int>(8, 16), 1, 1));
    // result == 12 8 1 -1
    data.push_back(Vertex45(point_data<int>(12, 8), 1, -1));
    // result == 12 8 -1 1
    data.push_back(Vertex45(point_data<int>(12, 8), -1, 1));

    data.push_back(Vertex45(point_data<int>(6, 4), 1, -1));
    data.push_back(Vertex45(point_data<int>(6, 4), 2, -1));
    data.push_back(Vertex45(point_data<int>(6, 12), -1, 1));
    data.push_back(Vertex45(point_data<int>(6, 12), 2, 1));
    data.push_back(Vertex45(point_data<int>(10, 8), -1, -1));
    data.push_back(Vertex45(point_data<int>(10, 8), 1, 1));

    std::sort(data.begin(), data.end());
    std::vector<polygon_45_data<int> > polys;
    pf.scan(polys, data.begin(), data.end());
    polygon_45_set_data<int> ps;
    std::cout << "inserting1\n";
    //std::vector<point_data<int> > points;
    //points.push_back(point_data<int>(0,0));
    //points.push_back(point_data<int>(0,10));
    //points.push_back(point_data<int>(10,10));
    //points.push_back(point_data<int>(10,0));
    //Polygon45 poly;
    //poly.set(points.begin(), points.end());
    //ps.insert(poly);
    ps.insert(polys[0]);

    polygon_45_set_data<int> ps2;
    std::cout << "inserting2\n";
    ps2.insert(polys[0]);
    std::cout << "applying boolean\n";
    ps |= ps2;
    std::vector<polygon_45_data<int> > polys2;
    std::cout << "getting result\n";
    ps.get_polygons(polys2);
    std::cout << ps2 << std::endl;
    std::cout << ps << std::endl;
    std::cout << polys[0] << std::endl;
    std::cout << polys2[0] << std::endl;
    if(polys != polys2) std::cout << "test Polygon45Set failed\n";
    return polys == polys2;
  }

  inline bool testPolygon45SetPerterbation() {
    polygon_45_formation<int>::Polygon45Formation pf(true);
    typedef boolean_op_45<int>::Vertex45 Vertex45;
    std::vector<Vertex45> data;
    // result == 0 8 -1 1
    data.push_back(Vertex45(point_data<int>(0, 80), -1, 1));
    // result == 0 8 1 -1
    data.push_back(Vertex45(point_data<int>(0, 80), 1, -1));
    // result == 4 0 1 1
    data.push_back(Vertex45(point_data<int>(40, 0), 1, 1));
    // result == 4 0 2 1
    data.push_back(Vertex45(point_data<int>(40, 0), 2, 1));
    // result == 4 4 2 -1
    data.push_back(Vertex45(point_data<int>(40, 40), 2, -1));
    // result == 4 4 -1 -1
    data.push_back(Vertex45(point_data<int>(40, 40), -1, -1));
    // result == 4 12 1 1
    data.push_back(Vertex45(point_data<int>(40, 120), 1, 1));
    // result == 4 12 2 1
    data.push_back(Vertex45(point_data<int>(40, 120), 2, 1));
    // result == 4 16 2 -1
    data.push_back(Vertex45(point_data<int>(40, 160), 2, 1));
    // result == 4 16 -1 -1
    data.push_back(Vertex45(point_data<int>(40, 160), -1, -1));
    // result == 6 2 1 -1
    data.push_back(Vertex45(point_data<int>(60, 20), 1, -1));
    // result == 6 14 -1 1
    data.push_back(Vertex45(point_data<int>(60, 140), -1, 1));
    // result == 6 2 -1 1
    data.push_back(Vertex45(point_data<int>(60, 20), -1, 1));
    // result == 6 14 1 -1
    data.push_back(Vertex45(point_data<int>(60, 140), 1, -1));
    // result == 8 0 -1 -1
    data.push_back(Vertex45(point_data<int>(80, 0), -1, -1));
    // result == 8 0 2 -1
    data.push_back(Vertex45(point_data<int>(80, 0), 2, -1));
    // result == 8 4 2 1
    data.push_back(Vertex45(point_data<int>(80, 40), 2, 1));
    // result == 8 4 1 1
    data.push_back(Vertex45(point_data<int>(80, 40), 1, 1));
    // result == 8 12 -1 -1
    data.push_back(Vertex45(point_data<int>(80, 120), -1, -1));
    // result == 8 12 2 -1
    data.push_back(Vertex45(point_data<int>(80, 120), 2, -1));
    // result == 8 16 2 1
    data.push_back(Vertex45(point_data<int>(80, 160), 2, 1));
    // result == 8 16 1 1
    data.push_back(Vertex45(point_data<int>(80, 160), 1, 1));
    // result == 12 8 1 -1
    data.push_back(Vertex45(point_data<int>(120, 80), 1, -1));
    // result == 12 8 -1 1
    data.push_back(Vertex45(point_data<int>(120, 80), -1, 1));

    data.push_back(Vertex45(point_data<int>(60, 40), 1, -1));
    data.push_back(Vertex45(point_data<int>(60, 40), 2, -1));
    data.push_back(Vertex45(point_data<int>(60, 120), -1, 1));
    data.push_back(Vertex45(point_data<int>(60, 120), 2, 1));
    data.push_back(Vertex45(point_data<int>(100, 80), -1, -1));
    data.push_back(Vertex45(point_data<int>(100, 80), 1, 1));

    std::sort(data.begin(), data.end());
    std::vector<polygon_45_data<int> > polys;
    pf.scan(polys, data.begin(), data.end());
    polygon_45_set_data<int> ps;
    std::cout << "inserting1\n";
    //std::vector<point_data<int> > points;
    //points.push_back(point_data<int>(0,0));
    //points.push_back(point_data<int>(0,10));
    //points.push_back(point_data<int>(10,10));
    //points.push_back(point_data<int>(10,0));
    //Polygon45 poly;
    //poly.set(points.begin(), points.end());
    //ps.insert(poly);
    polygon_45_set_data<int> preps(polys[0]);

    ps.insert(polys[0]);
    convolve(polys[0], point_data<int>(0, 1) );

    polygon_45_set_data<int> ps2;
    std::cout << "inserting2\n";
    ps2.insert(polys[0]);
    std::cout << "applying boolean\n";
    ps |= ps2;
    std::vector<polygon_45_data<int> > polys2;
    std::cout << "getting result\n";
    ps.get_polygons(polys2);
    std::cout << preps << std::endl;
    std::cout << ps2 << std::endl;
    std::cout << ps << std::endl;
    std::cout << polys[0] << std::endl;
    std::cout << polys2[0] << std::endl;
    if(polys != polys2) std::cout << "test Polygon45Set failed\n";
    return polys == polys2;
    //return true;
  }

  inline int testPolygon45SetDORA() {
    std::cout << "testPolygon45SetDORA" << std::endl;
    std::vector<point_data<int> > pts;
    pts.push_back(point_data<int>(0, 0));
    pts.push_back(point_data<int>(10, 0));
    pts.push_back(point_data<int>(10, 10));
    pts.push_back(point_data<int>(0, 10));
    polygon_45_data<int> apoly;
    apoly.set(pts.begin(), pts.end());
    polygon_45_set_data<int> ps(apoly);
    polygon_45_set_data<int> ps2(ps);
    ps2 = apoly;
    std::vector<polygon_45_data<int> > apolys;
    apolys.push_back(apoly);
    ps2.insert(apolys.begin(), apolys.end());
    apolys.clear();
    ps2.get(apolys);
    std::cout << apolys.size() << std::endl;
    std::cout << (ps == ps2) << std::endl;
    std::cout << !(ps != ps2) << std::endl;
    ps2.clear();
    std::cout << (ps2.value().empty()) << std::endl;
    ps2.set(apolys.begin(), apolys.end());
    ps2.set(ps.value());
    ps.clean();
    ps2.set_clean(ps.value());
    ps2.insert(ps.value().begin(), ps.value().end());
    ps2.clear();
    for(polygon_45_set_data<int>::iterator_type itr = ps.begin();
        itr != ps.end(); ++itr) {
      ps2.insert(*itr);
    }
    std::vector<polygon_45_with_holes_data<int> > apolywhs;
    ps2.get_polygons_with_holes(apolywhs);
    std::cout << apolywhs.size() << std::endl;
    ps2 += 1;
    apolywhs.clear();
    ps2.get_polygons_with_holes(apolywhs);
    if(apolywhs.size()) std::cout << apolywhs[0] << std::endl;
    ps2 -= 1;
    apolywhs.clear();
    ps2.get_polygons_with_holes(apolywhs);
    if(apolywhs.size()) std::cout << apolywhs[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    rectangle_data<int> rect;
    extents(rect, apolywhs[0]);
    ps2.clear();
    ps2.insert(rect);
    ps2.extents(rect);
    ps2.clear();
    ps2.insert(rect);
    ps2.clear();
    ps2.insert(apolywhs[0]);
    apolywhs.clear();
    ps2.get_trapezoids(apolywhs);
    if(apolywhs.size()) std::cout << apolywhs[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    ps2 *= ps;
    std::cout << (ps2 == ps) << std::endl;
    ps2 ^= ps;
    std::cout << ps2.empty() << std::endl;
    axis_transformation atr(axis_transformation::WS);
    ps2 = ps;
    ps.transform(atr);
    transformation<int> tr(atr);
    tr.invert();
    ps.transform(tr);
    ps.scale_up(2);
    ps.scale_down(2);
    std::cout << (ps2 == ps) << std::endl;
    pts.clear();
    pts.push_back(point_data<int>(0,0));
    pts.push_back(point_data<int>(10,10));
    pts.push_back(point_data<int>(10,11));
    pts.push_back(point_data<int>(0,21));
    apoly.set(pts.begin(), pts.end());
    ps2.clear();
    ps2.insert(apoly);
    ps2 -= 1;
    apolywhs.clear();
    ps2.get_polygons_with_holes(apolywhs);
    if(apolywhs.size()) std::cout << apolywhs[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    pts.clear();
    pts.push_back(point_data<int>(0, 0));
    pts.push_back(point_data<int>(10, 10));
    pts.push_back(point_data<int>(0, 20));
    apoly.set(pts.begin(), pts.end());
    ps2.clear();
    ps2.insert(apoly);
    pts.clear();
    pts.push_back(point_data<int>(0, 5));
    pts.push_back(point_data<int>(10, 15));
    pts.push_back(point_data<int>(0, 25));
    apoly.set(pts.begin(), pts.end());
    ps2.insert(apoly);
    apolywhs.clear();
    ps2.get_polygons_with_holes(apolywhs);
    if(apolywhs.size()) std::cout << apolywhs[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    return 0;

  }
}
}
using namespace gtl;

bool testRectangle() {
  rectangle_data<int> rect, rect2;
#ifdef BOOST_POLYGON_MSVC
  horizontal(rect, interval_data<int>(0, 10));
  vertical(rect, interval_data<int>(20, 30));
#else
  horizontal(rect, interval_data<polygon_long_long_type>(0, 10));
  vertical(rect, interval_data<polygon_long_long_type>(20, 30));
#endif
  xl(rect2, 0);
  xh(rect2, 10);
  yl(rect2, 20);
  yh(rect2, 30);
  if(euclidean_distance(rect, rect2) != 0) return false;
  if(euclidean_distance(rect2, rect) != 0) return false;
#ifdef BOOST_POLYGON_MSVC
  set(rect, HORIZONTAL, interval_data<int>(0, 10));
  if(!equivalence(horizontal(rect), interval_data<int>(0, 10))) return false;
  if(!equivalence(vertical(rect2), interval_data<int>(20, 30))) return false;
#else
  set(rect, HORIZONTAL, interval_data<polygon_long_long_type>(0, 10));
  if(!equivalence(horizontal(rect), interval_data<polygon_long_long_type>(0, 10))) return false;
  if(!equivalence(vertical(rect2), interval_data<polygon_long_long_type>(20, 30))) return false;
#endif
  if(xl(rect) != 0) return false;
  if(xh(rect) != 10) return false;
  if(yl(rect) != 20) return false;
  if(yh(rect) != 30) return false;
  move(rect, HORIZONTAL, 10);
  if(xl(rect) != 10) return false;
#ifdef BOOST_POLYGON_MSVC
  set_points(rect, point_data<int>(0, 20), point_data<int>(10, 30));
#else
  set_points(rect, point_data<int>(0, 20), point_data<polygon_long_long_type>(10, 30));
#endif
  if(xl(rect) != 0) return false;
  convolve(rect, rect2);
  if(xh(rect) != 20) return false;
  deconvolve(rect, rect2);
  if(xh(rect) != 10) return false;
  reflected_convolve(rect, rect2);
  reflected_deconvolve(rect, rect2);
  if(!equivalence(rect, rect2)) return false;
#ifdef BOOST_POLYGON_MSVC
  convolve(rect, point_data<int>(100, 200));
#else
  convolve(rect, point_data<polygon_long_long_type>(100, 200));
#endif
  if(xh(rect) != 110) return false;
  deconvolve(rect, point_data<int>(100, 200));
  if(!equivalence(rect, rect2)) return false;
  xh(rect, 100);
  if(delta(rect, HORIZONTAL) != 100) return false;
  if(area(rect) != 1000) return false;
  if(half_perimeter(rect) != 110) return false;
  if(perimeter(rect) != 220) return false;
  if(guess_orientation(rect) != HORIZONTAL) return false;
  return true;
}


bool testPolygon() {
  int rect[4] = {0, 10, 20, 30};
  iterator_compact_to_points<int*, point_data<int> > itr(rect, rect+4);
  iterator_compact_to_points<int*, point_data<int> > itr_end(rect, rect+4);
  std::vector<point_data<int> > points;
  points.insert(points.end(), itr, itr_end);
  polygon_90_data<int> p90;
  assign(p90, rectangle_data<int>(interval_data<int>(0, 10), interval_data<int>(20, 30)));
  if(winding(p90) != COUNTERCLOCKWISE) return false;
  polygon_45_data<int> p45;
  assign(p45, rectangle_data<int>(interval_data<int>(0, 10), interval_data<int>(20, 30)));
  if(winding(p45) != COUNTERCLOCKWISE) return false;
  polygon_data<int> p;
  assign(p, rectangle_data<int>(interval_data<int>(0, 10), interval_data<int>(20, 30)));
  if(winding(p) != COUNTERCLOCKWISE) return false;
  set_compact(p90, rect, rect+4);
  if(winding(p90) != COUNTERCLOCKWISE) return false;
  points.clear();
  points.push_back(point_data<int>(0, 0));
  points.push_back(point_data<int>(10, 10));
  points.push_back(point_data<int>(0, 20));
  points.push_back(point_data<int>(-10, 10));
  set_points(p45, points.begin(), points.end());
  if(winding(p45) != COUNTERCLOCKWISE) return false;
  std::swap(points[1], points[3]);
  set_points(p, points.begin(), points.end());
  if(winding(p) == COUNTERCLOCKWISE) return false;
  point_data<int> cp;
  center(cp, p);
  if(cp != point_data<int>(0, 10)) return false;
  move(p, HORIZONTAL, 3);
  rectangle_data<int> bounding_box;
  extents(bounding_box, p);
  if(bounding_box != rectangle_data<int>(interval_data<int>(-7, 13), interval_data<int>(0, 20))) return false;
  if(area(p90) != 400) return false;
  if(area(p45) != 200) return false;
  if(perimeter(p90) != 80) return false;
  return true;
}

bool testPolygonAssign() {
  polygon_data<int> p;
  polygon_data<int> p1;
  polygon_45_data<int> p_45;
  polygon_45_data<int> p_451;
  polygon_90_data<int> p_90;
  polygon_90_data<int> p_901;
  polygon_with_holes_data<int> p_wh;
  polygon_with_holes_data<int> p_wh1;
  polygon_45_with_holes_data<int> p_45_wh;
  polygon_45_with_holes_data<int> p_45_wh1;
  polygon_90_with_holes_data<int> p_90_wh;
  polygon_90_with_holes_data<int> p_90_wh1;
  assign(p, p1);
  assign(p, p_45);
  assign(p, p_90);
  //assign(p, p_wh);
  //assign(p, p_45_wh);
  //assign(p, p_90_wh);
  //assign(p_45, p);
  assign(p_451, p_45);
  assign(p_45, p_90);
  //assign(p_45, p_wh);
  //assign(p_45, p_45_wh);
  //assign(p_45, p_90_wh);
  //assign(p_90, p);
  //assign(p_90, p_45);
  assign(p_901, p_90);
  //assign(p_90, p_wh);
  //assign(p_90, p_45_wh);
  //assign(p_90, p_90_wh);
  assign(p_wh, p);
  assign(p_wh, p_45);
  assign(p_wh, p_90);
  assign(p_wh1, p_wh);
  assign(p_wh, p_45_wh);
  assign(p_wh, p_90_wh);
  //assign(p_45_wh, p);
  assign(p_45_wh, p_45);
  assign(p_45_wh, p_90);
  //assign(p_45_wh, p_wh);
  assign(p_45_wh1, p_45_wh);
  //assign(p_90_wh, p);
  //assign(p_90_wh, p_45);
  assign(p_90_wh, p_90);
  assign(p_90_wh1, p_90_wh);
  return true;
}

int testPropertyMerge() {
  rectangle_data<int> rect1 = construct<rectangle_data<int> >(0, 1, 10, 11);
  rectangle_data<int> rect2 = construct<rectangle_data<int> >(5, 6, 17, 18);
  property_merge_90<int, int> pm;
  pm.insert(rect1, 0);
  pm.insert(rect2, 1);
  std::map<std::set<int>, polygon_90_set_data<int> > result;
  pm.merge(result);
  std::vector<rectangle_data<int> > rects;
  std::set<int> key;
  key.insert(0);
  result[key].get(rects);
  std::cout << rects.size() << std::endl;
  std::vector<polygon_data<int> > polys;
  result[key].get(polys);
  std::cout << polys.size() << std::endl;
  std::vector<polygon_90_with_holes_data<int> > polywhs;
  result[key].get(polywhs);
  std::cout << polys.size() << std::endl;
  return result.size();
}

bool testPolygonWithHoles() {
  int rect[4] = {0, 10, 20, 30};
  iterator_compact_to_points<int*, point_data<int> > itr(rect, rect+4);
  iterator_compact_to_points<int*, point_data<int> > itr_end(rect, rect+4);
  std::vector<point_data<int> > points;
  points.insert(points.end(), itr, itr_end);
  polygon_45_with_holes_data<int> p45wh;
  assign(p45wh, rectangle_data<int>(interval_data<int>(0, 10), interval_data<int>(20, 30)));
  if(winding(p45wh) != COUNTERCLOCKWISE) return false;
  polygon_45_with_holes_data<int> p45;
  assign(p45, rectangle_data<int>(interval_data<int>(0, 10), interval_data<int>(20, 30)));
  if(winding(p45) != COUNTERCLOCKWISE) return false;
  polygon_45_with_holes_data<int> p;
  assign(p, rectangle_data<int>(interval_data<int>(0, 10), interval_data<int>(20, 30)));
  if(winding(p) != COUNTERCLOCKWISE) return false;
  set_compact(p45wh, rect, rect+4);
  if(winding(p45wh) != COUNTERCLOCKWISE) return false;
  points.clear();
  points.push_back(point_data<int>(0, 0));
  points.push_back(point_data<int>(10, 10));
  points.push_back(point_data<int>(0, 20));
  points.push_back(point_data<int>(-10, 10));
  set_points(p45, points.begin(), points.end());
  if(winding(p45) != COUNTERCLOCKWISE) return false;
  std::swap(points[1], points[3]);
  set_points(p, points.begin(), points.end());
  if(winding(p) == COUNTERCLOCKWISE) return false;
  point_data<int> cp;
  center(cp, p);
  if(cp != point_data<int>(0, 10)) return false;
  move(p, HORIZONTAL, 3);
  rectangle_data<int> bounding_box;
  extents(bounding_box, p);
  if(bounding_box != rectangle_data<int>(interval_data<int>(-7, 13), interval_data<int>(0, 20))) return false;
  if(area(p45wh) != 400) return false;
  if(area(p45) != 200) return false;
  if(perimeter(p45wh) != 80) return false;
  return true;
}

using namespace gtl;

typedef int Unit;
typedef point_data<int> Point;
typedef interval_data<int> Interval;
typedef rectangle_data<int> Rectangle;
typedef polygon_90_data<int> Polygon;
typedef polygon_90_with_holes_data<int> PolygonWithHoles;
typedef polygon_45_data<int> Polygon45;
typedef polygon_45_with_holes_data<int> Polygon45WithHoles;
typedef polygon_90_set_data<int> PolygonSet;
typedef polygon_45_set_data<int> Polygon45Set;
typedef axis_transformation AxisTransform;
typedef transformation<int> Transform;

bool getRandomBool() {
  return rand()%2 != 0;
}
int getRandomInt() {
  return rand()%6-2;
}
Point getRandomPoint() {
  int x = rand()%8;
  int y = rand()%8;
  return Point(x, y);
}
Polygon45 getRandomTriangle() {
  Point pts[3];
  pts[0] = getRandomPoint();
  pts[1] = pts[2] = pts[0];
  int disp = getRandomInt();
  bool dir = getRandomBool();
  x(pts[2], x(pts[2]) + disp);
  x(pts[1], x(pts[1]) + disp);
  if(dir)
    y(pts[1], y(pts[1]) + disp);
  else
    y(pts[1], y(pts[1]) - disp);
  return Polygon45(pts, pts+3);
}

bool nonInteger45StessTest() {
  for(unsigned int tests = 0; tests < 10; ++tests) {
    Polygon45Set ps1, ps2;
    std::vector<Polygon45> p45s;
    for(unsigned int i = 0; i < 10; ++i) {
      Polygon45 p45 = getRandomTriangle();
      p45s.push_back(p45);
      ps1.insert(p45);
      scale_up(p45, 2);
      ps2.insert(p45);
    }
    std::vector<Polygon45> polys;
    ps1.get(polys);
    Polygon45Set ps3;
    for(unsigned int i = 0; i < polys.size(); ++i) {
      scale_up(polys[i], 2);
      ps3.insert(polys[i]);
    }
    Polygon45Set ps4 = ps3 ^ ps2;
    std::vector<Polygon45> polys_error;
    ps4.get(polys_error);
    for(unsigned int i = 0; i < polys_error.size(); ++i) {
      //if(polys_error[i].size() > 3) return false;
      if(area(polys_error[i]) != 1) {
        if(area(polys_error[i]) == 2) {
          //if two area 1 errors merge it will have area 2
          continue;
        }
        std::cout << "test failed\n";
        for(unsigned int j =0; j < p45s.size(); ++j) {
          std::cout << p45s[j] << std::endl;
        }
        return false;
      }
    }
  }
  return true;
}

bool validate_polygon_set_op(Polygon45Set& ps45_o,
                             const Polygon45Set& ps45_1,
                             const Polygon45Set& ps45_2,
                             int op_type) {
  Polygon45Set s_ps_45_o(ps45_o);
  Polygon45Set s_ps_45_1(ps45_1);
  Polygon45Set s_ps_45_2(ps45_2);
  s_ps_45_o.scale_up(2);
  s_ps_45_1.scale_up(2);
  s_ps_45_2.scale_up(2);
  Polygon45Set s_ps_45_validate;
  if(op_type == 0) {
    s_ps_45_validate = s_ps_45_1 + s_ps_45_2;
    s_ps_45_validate += Rectangle(4, 4, 6, 6);
  } else if(op_type == 1) {
    s_ps_45_validate = s_ps_45_1 * s_ps_45_2;
    s_ps_45_validate -= Rectangle(4, 4, 6, 6);
  } else if(op_type == 2) {
    s_ps_45_validate = s_ps_45_1 ^ s_ps_45_2;
    s_ps_45_validate -= Rectangle(4, 4, 6, 6);
  } else {
    s_ps_45_validate = s_ps_45_1 - s_ps_45_2;
    s_ps_45_validate -= Rectangle(4, 4, 6, 6);
  }
  if(s_ps_45_validate != s_ps_45_o) {
    std::cout << "TEST FAILED\n";
    std::vector<Polygon45> polys;
    s_ps_45_o.get(polys);
    std::cout << "Result:\n";
    for(unsigned int i = 0; i < polys.size(); ++i) {
      std::cout << polys[i] << std::endl;
    }
    polys.clear();
    s_ps_45_validate.get(polys);
    std::cout << "Expected Result:\n";
    for(unsigned int i = 0; i < polys.size(); ++i) {
      std::cout << polys[i] << std::endl;
    }
    //redo the operation, set breakpoints here
    switch (op_type) {
    case 0:
      ps45_o = ps45_1 + ps45_2;
      ps45_o.get(polys);//needed to force clean
      break;
    case 1:
      ps45_o = ps45_1 * ps45_2;
      break;
    case 2:
      ps45_o = ps45_1 ^ ps45_2;
      break;
    default:
      ps45_o = ps45_1 - ps45_2;
    };
    //redo the check, set breakpoints here
    if(op_type == 0) {
      s_ps_45_validate = s_ps_45_1 + s_ps_45_2;
      s_ps_45_validate += Rectangle(4, 4, 6, 6);
      s_ps_45_validate.get(polys);
    } else if(op_type == 1) {
      s_ps_45_validate = s_ps_45_1 * s_ps_45_2;
      s_ps_45_validate -= Rectangle(4, 4, 6, 6);
    } else if(op_type == 2) {
      s_ps_45_validate = s_ps_45_1 ^ s_ps_45_2;
      s_ps_45_validate -= Rectangle(4, 4, 6, 6);
    } else {
      s_ps_45_validate = s_ps_45_1 - s_ps_45_2;
      s_ps_45_validate -= Rectangle(4, 4, 6, 6);
    }
    return false;
  }
  return true;
}

bool test_two_polygon_sets(const Polygon45Set& ps45_1,
                           const Polygon45Set& ps45_2) {
  std::cout << "test two polygon sets \n";
  std::vector<Polygon45> polys;
  ps45_1.get(polys);
  std::cout << "LVALUE:\n";
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  polys.clear();
  ps45_2.get(polys);
  std::cout << "RVALUE:\n";
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  Polygon45Set ps45_o;
  std::cout << "OR\n";
  ps45_o = ps45_1 + ps45_2;
  polys.clear();
  ps45_o.get(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  if(!validate_polygon_set_op(ps45_o, ps45_1, ps45_2, 0)) return false;
  std::cout << "AND\n";
  ps45_o = ps45_1 * ps45_2;
  polys.clear();
  ps45_o.get(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  if(!validate_polygon_set_op(ps45_o, ps45_1, ps45_2, 1)) return false;
  std::cout << "XOR\n";
  ps45_o = ps45_1 ^ ps45_2;
  polys.clear();
  ps45_o.get(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  if(!validate_polygon_set_op(ps45_o, ps45_1, ps45_2, 2)) return false;
  std::cout << "SUBTRACT\n";
  ps45_o = ps45_1 - ps45_2;
  polys.clear();
  ps45_o.get(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  if(!validate_polygon_set_op(ps45_o, ps45_1, ps45_2, 3)) return false;
  return true;
}

bool test_two_polygons(const Polygon45& p45_1,
                       const Polygon45& p45_2) {
  Polygon45Set ps45_1, ps45_2;
  ps45_1.insert(p45_1);
  ps45_2.insert(p45_2);
  ps45_1.insert(rectangle_data<int>(10, -100, 20, 100));
  ps45_2.insert(rectangle_data<int>(0, 10, 100, 20));
  if(!test_two_polygon_sets(ps45_1, ps45_2)) return false;
  Polygon45Set ps45_1_c = ps45_1 - Rectangle(0, 0, 2, 5);
  Polygon45Set ps45_2_c = ps45_2 - Rectangle(0, 0, 2, 5);
  if(!test_two_polygon_sets(ps45_1_c, ps45_2_c)) return false;
  if(!test_two_polygon_sets(ps45_1_c, ps45_2)) return false;
  if(!test_two_polygon_sets(ps45_1, ps45_2_c)) return false;
  return true;
}

bool test_45_touch() {
  using namespace gtl;
  connectivity_extraction_45<int> ce;
  rectangle_data<int> rect1(0, 0, 10, 10);
  rectangle_data<int> rect2(5, 5, 15, 15);
  rectangle_data<int> rect3(5, 20, 15, 25);
  ce.insert(rect1);
  ce.insert(rect2);
  ce.insert(rect3);
  std::vector<std::set<int> > graph(3);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1 && graph[2].size() == 0) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_45_touch_ur() {
  using namespace gtl;
  connectivity_extraction_45<int> ce;
  rectangle_data<int> rect1(0, 0, 5, 5);
  rectangle_data<int> rect2(5, 5, 10, 10);
  ce.insert(rect1);
  ce.insert(rect2);
  std::vector<std::set<int> > graph(2);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_45_touch_r() {
  using namespace gtl;
  connectivity_extraction_45<int> ce;
  rectangle_data<int> rect1(0, 0, 5, 5);
  rectangle_data<int> rect2(5, 0, 10, 5);
  ce.insert(rect1);
  ce.insert(rect2);
  std::vector<std::set<int> > graph(2);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_45_touch_boundaries() {
  using namespace gtl;
  connectivity_extraction_45<int> ce;
  rectangle_data<int> rect1(0, 0, 10, 10);
  rectangle_data<int> rect2(10, 0, 20, 10);
  rectangle_data<int> rect3(20, 0, 30, 10);
  rectangle_data<int> rect4(0, 10, 10, 20);
  rectangle_data<int> rect5(10, 10, 20, 20);
  rectangle_data<int> rect6(20, 10, 30, 20);
  rectangle_data<int> rect7(0, 20, 10, 30);
  rectangle_data<int> rect8(10, 20, 20, 30);
  rectangle_data<int> rect9(20, 20, 30, 30);
  ce.insert(rect1);
  ce.insert(rect2);
  ce.insert(rect3);
  ce.insert(rect4);
  ce.insert(rect5);
  ce.insert(rect6);
  ce.insert(rect7);
  ce.insert(rect8);
  ce.insert(rect9);
  std::vector<std::set<int> > graph(9);
  ce.extract(graph);
  for(unsigned int i = 0; i < 9; ++i) {
    std::cout << i << ": ";
    for(std::set<int>::iterator itr = graph[i].begin(); itr != graph[i].end(); ++itr) {
      std::cout << *itr << " ";
    } std::cout << std::endl;
  }
  if(graph[0].size() == 3 && graph[1].size() == 5 && graph[2].size() == 3 &&
     graph[3].size() == 5 && graph[4].size() == 8 && graph[5].size() == 5 &&
     graph[6].size() == 3 && graph[7].size() == 5 && graph[8].size() == 3) {
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_45_concept_interact() {
  using namespace gtl;
  std::vector<polygon_45_data<int> > polys;
  polys += rectangle_data<int>(10, 10, 20, 20);
  polys += rectangle_data<int>(15, 15, 25, 25);
  polys += rectangle_data<int>(5, 25, 10, 35);
  interact(polys, rectangle_data<int>(0, 0, 13, 13));
  if(polys.size() != 1) return false;
  return true;
}

bool test_aa_touch() {
  using namespace gtl;
  connectivity_extraction<int> ce;
  rectangle_data<int> rect1(0, 0, 10, 10);
  rectangle_data<int> rect2(5, 5, 15, 15);
  rectangle_data<int> rect3(5, 20, 15, 25);
  ce.insert(rect1);
  ce.insert(rect2);
  ce.insert(rect3);
  std::vector<std::set<int> > graph(3);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1 && graph[2].size() == 0) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_aa_touch_ur() {
  using namespace gtl;
  connectivity_extraction<int> ce;
  rectangle_data<int> rect1(0, 0, 5, 5);
  rectangle_data<int> rect2(5, 5, 10, 10);
  ce.insert(rect1);
  ce.insert(rect2);
  std::vector<std::set<int> > graph(2);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_aa_touch_ur2() {
  using namespace gtl;
  connectivity_extraction<int> ce;
  rectangle_data<int> rect2(5, 5, 10, 10);
  point_data<int> pts[3] = {
    point_data<int>(0, 0),
    point_data<int>(5, 5),
    point_data<int>(0, 5)
  };
  polygon_data<int> poly;
  poly.set(pts, pts+3);
  ce.insert(poly);
  ce.insert(rect2);
  std::vector<std::set<int> > graph(2);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_aa_touch_r() {
  using namespace gtl;
  connectivity_extraction<int> ce;
  rectangle_data<int> rect1(0, 0, 5, 5);
  rectangle_data<int> rect2(5, 0, 10, 5);
  ce.insert(rect1);
  ce.insert(rect2);
  std::vector<std::set<int> > graph(2);
  ce.extract(graph);
  if(graph[0].size() == 1 && graph[1].size() == 1) {
    std::set<int>::iterator itr = graph[0].begin();
    std::cout << *itr << std::endl;
    std::set<int>::iterator itr1 = graph[1].begin();
    std::cout << *itr1 << std::endl;
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_aa_touch_boundaries() {
  using namespace gtl;
  connectivity_extraction<int> ce;
  rectangle_data<int> rect1(0, 0, 10, 10);
  rectangle_data<int> rect2(10, 0, 20, 10);
  rectangle_data<int> rect3(20, 0, 30, 10);
  rectangle_data<int> rect4(0, 10, 10, 20);
  rectangle_data<int> rect5(10, 10, 20, 20);
  rectangle_data<int> rect6(20, 10, 30, 20);
  rectangle_data<int> rect7(0, 20, 10, 30);
  rectangle_data<int> rect8(10, 20, 20, 30);
  rectangle_data<int> rect9(20, 20, 30, 30);
  ce.insert(rect1);
  ce.insert(rect2);
  ce.insert(rect3);
  ce.insert(rect4);
  ce.insert(rect5);
  ce.insert(rect6);
  ce.insert(rect7);
  ce.insert(rect8);
  ce.insert(rect9);
  std::vector<std::set<int> > graph(9);
  ce.extract(graph);
  for(unsigned int i = 0; i < 9; ++i) {
    std::cout << i << ": ";
    for(std::set<int>::iterator itr = graph[i].begin(); itr != graph[i].end(); ++itr) {
      std::cout << *itr << " ";
    } std::cout << std::endl;
  }
  if(graph[0].size() == 3 && graph[1].size() == 5 && graph[2].size() == 3 &&
     graph[3].size() == 5 && graph[4].size() == 8 && graph[5].size() == 5 &&
     graph[6].size() == 3 && graph[7].size() == 5 && graph[8].size() == 3) {
    return true;
  }
  std::cout << "test failed\n";
  return false;
}

bool test_aa_concept_interact() {
  using namespace gtl;
  std::vector<polygon_data<int> > polys;
  polys += rectangle_data<int>(10, 10, 20, 20);
  polys += rectangle_data<int>(15, 15, 25, 25);
  polys += rectangle_data<int>(5, 25, 10, 35);
  interact(polys, rectangle_data<int>(0, 0, 13, 13));
  if(polys.size() != 1) return false;
  return true;
}

bool test_get_rectangles() {
  using namespace gtl;
  polygon_90_set_data<int> ps(VERTICAL);
  ps += rectangle_data<int>(0, 0, 10, 10);
  ps += rectangle_data<int>(5, 5, 15, 15);
  std::vector<polygon_90_data<int> > polys;
  ps.get_rectangles(polys, HORIZONTAL);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  if(polys.size() != 3) return false;
  std::vector<rectangle_data<int> > rects;
  ps.get_rectangles(rects, HORIZONTAL);
  for(unsigned int i = 0; i < rects.size(); ++i) {
    std::cout << rects[i] << std::endl;
  }
  if(rects.size() != 3) return false;
  if(!equivalence(rects[2], rectangle_data<int>(5,10,15,15))) return false;

  get_rectangles(polys, rects, VERTICAL);
  get_rectangles(rects, polys, HORIZONTAL);
  return equivalence(rects, polys);
}

bool test_get_trapezoids() {
  using namespace gtl;
  polygon_45_set_data<int> ps;
  ps += rectangle_data<int>(0, 0, 10, 10);
  ps += rectangle_data<int>(5, 5, 15, 15);
  std::vector<polygon_45_data<int> > polys;
  ps.get_trapezoids(polys, HORIZONTAL);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  if(polys.size() != 3) return false;
  std::vector<polygon_45_data<int> > rects;
  ps.get_trapezoids(rects, HORIZONTAL);
  for(unsigned int i = 0; i < rects.size(); ++i) {
    std::cout << rects[i] << std::endl;
  }
  if(rects.size() != 3) return false;
  if(!equivalence(rects[2], rectangle_data<int>(5,10,15,15))) return false;
  get_trapezoids(polys, rects, VERTICAL);
  get_trapezoids(rects, polys, HORIZONTAL);
  return equivalence(rects, polys);
}

bool test_SQRT1OVER2() {
  Point pts[] = {
    Point(100, 100),
    Point(0, 100),
    Point(100, 200),
    Point(0, 300),
    Point(100, 400),
    Point(0, 500),
    Point(100, 500),
    Point(100, 600),
    Point(200, 500),
    Point(300, 600),
    Point(400, 500),
    Point(500, 600),
    Point(500, 500),
    Point(600, 500),
    Point(500, 400),
    Point(600, 300),
    Point(500, 200),
    Point(600, 100),
    Point(500, 100),
    Point(500, 0),
    Point(400, 100),
    Point(300, 0),
    Point(200, 100),
    Point(100, 0),
    Point(100, 100)
  };
  Polygon45 p45(pts, pts+25);
  std::cout << is_45(p45) << std::endl;
  std::cout << p45 << std::endl;
  Polygon45Set ps45;
  ps45 += p45;
  ps45.resize(10, SQRT1OVER2, ORTHOGONAL);
  std::vector<Polygon45> polys;
  ps45.get(polys);
  if(polys.size() != 1) return false;
  Point pts2[] = {
    Point(90, 90),
    Point(-10, 90),
    Point(-10, 100),
    Point(90, 200),
    Point(-10, 300),
    Point(90, 400),
    Point(-10, 500),
    Point(-10, 510),
    Point(90, 510),
    Point(90, 610),
    Point(100, 610),
    Point(200, 510),
    Point(300, 610),
    Point(400, 510),
    Point(500, 610),
    Point(510, 610),
    Point(510, 510),
    Point(610, 510),
    Point(610, 500),
    Point(510, 400),
    Point(610, 300),
    Point(510, 200),
    Point(610, 100),
    Point(610, 90),
    Point(510, 90),
    Point(510, -10),
    Point(500, -10),
    Point(400, 90),
    Point(300, -10),
    Point(200, 90),
    Point(100, -10),
    Point(90, -10),
    Point(90, 90)
  };
  Polygon45 p45reference(pts2, pts2+33);
  std::cout << is_45(polys[0]) << std::endl;
  std::cout << polys[0] << std::endl;
  std::cout << p45reference << std::endl;
  std::cout << is_45(p45reference) << std::endl;
  if(!equivalence(polys[0], p45reference)) {
    std::cout << "polys don't match\n";
    return false;
  }
  ps45.resize(-10, SQRT1OVER2, ORTHOGONAL);
  polys.clear();
  ps45.get(polys);
  if(polys.size() != 1) return false;
  std::cout << is_45(polys[0]) << std::endl;
  std::cout << polys[0] << std::endl;
  if(!equivalence(polys[0], p45)) {
    std::cout << "polys don't match\n";
    return false;
  }
  ps45.resize(11, SQRT1OVER2, UNFILLED);
  polys.clear();
  ps45.get(polys);
  if(polys.size() != 1) return false;
  std::cout << is_45(polys[0]) << std::endl;
  std::cout << polys[0] << std::endl;
  return true;
}

bool test_scaling_by_floating(){
  Point pts[] = {
    Point(1, 1),
    Point(10, 1),
    Point(1, 10)
  };
  Polygon45 poly(pts, pts+3);
  Polygon45Set ps45;
  ps45 += poly;
  ps45.scale(double(2.5));
  std::vector<Polygon45> polys;
  ps45.get(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
    std::cout << area(polys[i]) << std::endl;
  }
  if(polys.size() != 1) return false;
  if(area(polys[0]) != 242) return false;
  scale(ps45, double(1)/double(2.5));
  polys.clear();
  ps45.get(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  return equivalence(polys, poly);
}

bool test_directional_resize() {
  std::vector<Rectangle> rects;
  rects.push_back(Rectangle(0, 0, 100, 100));
  resize(rects, -10, 10, -10, 10);
  for(unsigned int i = 0; i < rects.size(); ++i) {
    std::cout << rects[i] << std::endl;
  }
  if(rects.size() != 1) return false;
  if(rects[0] != Rectangle(10, 10, 110, 110)) return false;

  return true;
}

bool test_self_xor() {
  std::vector<Rectangle> rects;
  rects.push_back(Rectangle(0, 0, 10, 10));
  rects.push_back(Rectangle(5, 5, 15, 15));
  self_xor(rects);
  for(unsigned int i = 0; i < rects.size(); ++i) {
    std::cout << rects[i] << std::endl;
  }
  if(rects.size() == 4) return true;
  else return false;
}

bool test_grow_and_45() {
  polygon_45_set_data<int> ps;
  ps.insert(Rectangle(0, 0, 5, 5));
  ps.insert(Rectangle(5, 5, 15, 15));
  grow_and(ps, 2);
  std::vector<polygon_45_data<int> > rects;
  ps.get_trapezoids(rects);
  for(unsigned int i = 0; i < rects.size(); ++i) {
    std::cout << rects[i] << std::endl;
  }
  if(rects.size() != 1) return false;
  return equivalence(rects, Rectangle(3, 3, 7, 7));
}

bool test_self_xor_45() {
  polygon_45_set_data<int> ps;
  ps.insert(Rectangle(0, 0, 10, 10));
  ps.insert(Rectangle(5, 5, 15, 15));
  self_xor(ps);
  std::vector<polygon_45_data<int> > rects;
  ps.get_trapezoids(rects);
  for(unsigned int i = 0; i < rects.size(); ++i) {
    std::cout << rects[i] << std::endl;
  }
  if(rects.size() == 4) return true;
  else return false;
}

bool testViewCopyConstruct() {
  PolygonSet ps1, ps2;
  ps1.insert(Rectangle(0, 0, 10, 10));
  ps2.insert(Rectangle(5, 5, 15, 15));
  PolygonSet psr = ps1 - ps2;
  std::vector<Rectangle> rects;
  rects += psr;
  for(unsigned int i = 0; i < rects.size(); ++i)
    std::cout << rects[i] << std::endl;
  if( rects.size() != 2) return false;
  Polygon45Set ps45_1, ps45_2;
  ps45_1.insert(Rectangle(0, 0, 10, 10));
  ps45_2.insert(Rectangle(5, 5, 15, 15));
  Polygon45Set ps45_r = ps45_1 - ps45_2;
  std::vector<Polygon45> polys;
  ps45_r.get_trapezoids(polys);
  for(unsigned int i = 0; i < polys.size(); ++i)
    std::cout << polys[i] << std::endl;
  if( polys.size() != 2) return false;
  return true;
}

bool testpip() {
  std::vector<Point> pts;
  pts.push_back(Point(0, 0));
  pts.push_back(Point(10, 0));
  pts.push_back(Point(20, 10));
  pts.push_back(Point(0, 20));
  pts.push_back(Point(30, 40));
  pts.push_back(Point(-10, 50));
  pts.push_back(Point(-20, -20));
  pts.push_back(Point(0, 0));
  polygon_data<int> poly;
  polygon_with_holes_data<int> poly2;
  polygon_45_data<int> poly45;
  polygon_45_with_holes_data<int> poly245;
  polygon_90_data<int> poly90;
  polygon_90_with_holes_data<int> poly290;
  poly.set(pts.begin(), pts.end());
  poly2.set(pts.begin(), pts.end());
  assign(poly45, Rectangle(0, 0, 100, 100));
  assign(poly245, Rectangle(0, 0, 100, 100));
  assign(poly90, Rectangle(0, 0, 100, 100));
  assign(poly290, Rectangle(0, 0, 100, 100));
  for(unsigned int i = 0; i < pts.size(); ++i) {
    if(!contains(poly, pts[i], true)) return false;
    if(contains(poly, pts[i], false)) return false;
    if(!contains(poly2, pts[i], true)) return false;
    if(contains(poly2, pts[i], false)) return false;
  }
  if(!contains(poly45, pts[0], true)) return false;
  if(contains(poly245, pts[0], false)) return false;
  if(!contains(poly90, pts[0], true)) return false;
  if(contains(poly290, pts[0], false)) return false;
  Point pt(0, -10);
  if(contains(poly, pt)) return false;
  Point p2(0, 1);
  if(!contains(poly, p2)) return false;
  return true;
}

void testHand() {
  using namespace gtl;
  int handcoords[] = {
12375, 11050, 13175, 10200, 15825, 9275, 18750, 8525, 24150, 8300, 27575, 8400, 31775, 7800,
35975, 7200, 41375, 4800, 42575, 4200, 43175, 4200, 47375, 2400, 49175, 1800, 51150, 2200,
52275, 2825, 52625, 4150, 52375, 4975, 51575, 6000, 49275, 6850, 45700, 7950, 43175, 9600,
39575, 10800, 37775, 12000, 37775, 12600, 37775, 13800, 38975, 14400, 41375, 14400, 45575, 13200,
48600, 13000, 51575, 13200, 55175, 12600, 58775, 12600, 61175, 13200, 62375, 14400, 62550, 15700,
61975, 16875, 60775, 17600, 60100, 17675, 58525, 17675, 56150, 17575, 52175, 18000, 47975, 18600,
45575, 19200, 44375, 19200, 42675, 19325, 41600, 19775, 41600, 20500, 42100, 20825, 44975, 20400,
48575, 20400, 52775, 21000, 53975, 21000, 57575, 21000, 62375, 21000, 65450, 22000, 66300, 23100,
66100, 24550, 64750, 25925, 62975, 26400, 61175, 26400, 58775, 26400, 56025, 26050, 53450, 26025,
50975, 26400, 48575, 26400, 46775, 26400, 43650, 26075, 41375, 26400, 40775, 27000, 40775, 27600,
42225, 28650, 44375, 29400, 48575, 30000, 50975, 31200, 53975, 31800, 58775, 33000, 61200, 34300,
62375, 35400, 62375, 37200, 61175, 38400, 60000, 38700, 57575, 38400, 54550, 37575, 50975, 36600,
49075, 36125, 47750, 36125, 45700, 35425, 42350, 34350, 38900, 33775, 30575, 33000, 26975, 33600,
25975, 34900, 26375, 36600, 28175, 38400, 30575, 40800, 32375, 43800, 33200, 46200, 33200, 48000,
32650, 49300, 31425, 50000, 29950, 50125, 28825, 49375, 27575, 48000, 25825, 46000, 23975, 44100,
22175, 42600, 19775, 39600, 17325, 37300, 14975, 34800, 13175, 31800, 10775, 29400, 9600, 27400,
10175, 27000, 11375, 27600, 12575, 28800, 14375, 31800, 16175, 34800, 18575, 37200, 21575, 39000,
22775, 40200, 23975, 41400, 24575, 42600, 26375, 44400, 28325, 46000, 29850, 46775, 31175, 46200,
31550, 44575, 30575, 43200, 28775, 40800, 25775, 38400, 24575, 34800, 24750, 33175, 26975, 31800,
29975, 31800, 33575, 31800, 37775, 32400, 39575, 33000, 41975, 33600, 45150, 34175, 46975, 34750,
48575, 35400, 50975, 35400, 51575, 34800, 51875, 33725, 50775, 32575, 48575, 31800, 45750, 30875,
43775, 30600, 41375, 29400, 38975, 28800, 35975, 28200, 34775, 27600, 34175, 27000, 34775, 25800,
37175, 25200, 40175, 25200, 43175, 25200, 46775, 25200, 50975, 25425, 53375, 25200, 55175, 24600,
55525, 23450, 53975, 22200, 52775, 22200, 49075, 21850, 45950, 21925, 40775, 21600, 37775, 21600,
35150, 21350, 34325, 20950, 34175, 19800, 35975, 19200, 38375, 19200, 40750, 18900, 42575, 18600,
44375, 18000, 47975, 17400, 50375, 17125, 52025, 16625, 52775, 15600, 52100, 14625, 49675, 14125,
48625, 14125, 46775, 14400, 44375, 15000, 41375, 15150, 37700, 15275, 34775, 15600, 32850, 15925,
31775, 15600, 31425, 14875, 32375, 13800, 36575, 11400, 38975, 10200, 41375, 9000, 43075, 8150,
43650, 7200, 43325, 6250, 42225, 5825, 40800, 6275, 38900, 6925, 35375, 8400, 32375, 10200,
27575, 11400, 22775, 12600, 19775, 13225, 16775, 13800, 14975, 14400, 13050, 14000, 11975, 12600,
    0, 0 };
  std::vector<Point> handpoints;
  for(unsigned int i = 0; i < 100000; i += 2) {
    Point pt(handcoords[i], handcoords[i+1]);
    if(pt == Point(0, 0)) break;
    handpoints.push_back(pt);
  }
  polygon_data<int> handpoly;
  handpoly.set(handpoints.begin(), handpoints.end());
  int spiralcoords [] = {
37200, 3600, 42075, 4025, 47475, 5875, 51000, 7800, 55800, 12300, 59000, 17075, 60000, 20400,
61200, 25800, 61200, 29400, 60600, 33600, 58800, 38400, 55800, 42600, 53200, 45625,
49200, 48600, 43200, 51000, 35400, 51600, 29400, 50400, 23400, 47400, 19200, 43800,
16200, 39600, 14400, 35400, 13200, 29400, 13200, 24000, 15000, 18600, 17400, 13800,
20525, 10300, 24600, 7200, 29400, 4800, 32450, 4000, 34825, 3675, 35625, 3625,
35825, 7275, 39600, 7200, 43800, 8400, 46800, 9600, 50400, 12000, 53400, 15000,
55800, 18600, 57000, 23400, 57600, 27000, 57000, 32400, 55200, 37200, 52200, 41400,
48000, 45000, 42000, 47400, 35400, 48000, 30000, 46800, 24600, 43800, 20325, 39100,
17850, 34275, 16800, 27600, 17400, 22200, 20400, 16200, 24600, 11400, 28800, 9000,
32400, 7800, 33200, 7575, 33925, 11050, 35400, 10800, 37200, 10800, 41400, 11400,
46200, 13200, 49800, 16200, 51600, 19200, 53400, 23400, 54000, 29400, 52800, 33600,
49800, 39000, 45000, 42600, 39000, 44400, 33600, 43800, 28200, 42000, 24000, 37800,
21000, 33000, 20400, 26400, 21600, 21000, 24600, 16200, 28200, 13200, 31875, 11625,
33200, 15625, 36000, 15000, 39000, 15000, 43800, 16800, 46800, 19200, 49200, 23400,
49800, 27600, 48750, 32700, 46350, 36275, 42600, 39000, 38400, 40200, 31800, 39000,
28200, 36600, 25200, 31200, 24600, 26400, 26025, 21800, 28200, 18600, 30600, 16800,
32575, 19875, 34200, 19200, 36000, 18600, 37200, 18600, 40375, 19125, 43200, 21000,
45600, 24000, 46200, 27600, 45600, 30600, 43800, 33600, 41475, 35625, 37800, 36600,
33600, 36000, 30000, 33600, 28200, 28800, 28800, 24600, 30000, 22200, 31200, 23400,
30600, 25200, 30000, 27000, 30600, 30000, 31800, 32400, 34200, 34200, 38400, 34800,
41400, 33000, 44025, 30225, 44400, 26400, 43200, 23400, 40900, 21200, 37800, 20400,
34950, 20675, 32400, 22200, 30175, 19475, 28425, 21300, 27000, 24000, 26400, 27600,
27000, 31800, 31200, 36600, 36600, 38400, 42600, 37200, 46200, 33600, 48000, 30000,
47650, 24425, 45600, 20400, 42650, 18200, 39000, 16800, 35400, 16800, 33600, 17400,
32875, 17675, 31100, 13850, 28200, 15600, 25200, 18600, 22800, 22800, 22200, 27000,
23400, 33600, 26400, 38400, 31675, 41575, 37800, 42600, 40850, 42150, 42800, 41550,
47050, 39025, 50100, 35375, 52200, 29400, 51675, 23950, 49800, 19200, 46200, 15600,
41400, 13200, 37800, 12600, 35025, 12750, 33350, 13050, 32400, 9600, 30025, 10325,
25925, 12725, 22200, 16800, 19800, 21000, 18600, 25800, 18600, 30000, 20400, 35400,
22575, 39250, 25225, 41825, 28200, 43800, 33600, 46200, 39000, 46200, 44400, 45000,
48650, 42350, 52800, 37800, 55200, 32400, 55800, 26400, 54600, 21000, 53400, 18000,
50400, 14400, 47400, 12000, 42600, 9600, 39000, 9000, 36000, 9000, 34775, 9125,
34300, 5600, 30000, 6600, 25800, 8400, 22025, 11350, 18725, 15125, 16200, 20400,
15000, 24600, 15000, 30600, 16800, 36600, 20400, 42600, 25800, 46800, 31200, 49200,
38400, 49800, 45000, 48600, 51000, 45000, 55475, 40225, 58200, 34800, 59400, 30000,
59400, 25200, 58200, 19800, 55200, 14400, 52225, 11150, 47400, 7800, 44175, 6500,
40200, 5400, 38400, 5400, 37200, 5400, 0, 0 };
  std::vector<Point> spiralpoints;
  for(unsigned int i = 0; i < 100000; i += 2) {
    Point pt(spiralcoords[i], spiralcoords[i+1]);
    if(pt == Point(0, 0)) break;
    spiralpoints.push_back(pt);
  }
  polygon_data<int> spiralpoly;
  spiralpoly.set(spiralpoints.begin(), spiralpoints.end());
  polygon_set_data<int> handset;
  handset += handpoly;
  polygon_set_data<int> spiralset;
  spiralset += spiralpoly;
  polygon_set_data<int> xorset = handset ^ spiralset;
  std::vector<polygon_data<int> > polys;
  polys += xorset;
  std::cout << polys.size() << std::endl;
  for(unsigned int i = 0; i < polys.size(); ++i)
    std::cout << polys[i] << std::endl;
}

//void testHandFloat() {
//  using namespace gtl;
//  double handcoords[] = {
//12375, 11050, 13175, 10200, 15825, 9275, 18750, 8525, 24150, 8300, 27575, 8400, 31775, 7800,
//35975, 7200, 41375, 4800, 42575, 4200, 43175, 4200, 47375, 2400, 49175, 1800, 51150, 2200,
//52275, 2825, 52625, 4150, 52375, 4975, 51575, 6000, 49275, 6850, 45700, 7950, 43175, 9600,
//39575, 10800, 37775, 12000, 37775, 12600, 37775, 13800, 38975, 14400, 41375, 14400, 45575, 13200,
//48600, 13000, 51575, 13200, 55175, 12600, 58775, 12600, 61175, 13200, 62375, 14400, 62550, 15700,
//61975, 16875, 60775, 17600, 60100, 17675, 58525, 17675, 56150, 17575, 52175, 18000, 47975, 18600,
//45575, 19200, 44375, 19200, 42675, 19325, 41600, 19775, 41600, 20500, 42100, 20825, 44975, 20400,
//48575, 20400, 52775, 21000, 53975, 21000, 57575, 21000, 62375, 21000, 65450, 22000, 66300, 23100,
//66100, 24550, 64750, 25925, 62975, 26400, 61175, 26400, 58775, 26400, 56025, 26050, 53450, 26025,
//50975, 26400, 48575, 26400, 46775, 26400, 43650, 26075, 41375, 26400, 40775, 27000, 40775, 27600,
//42225, 28650, 44375, 29400, 48575, 30000, 50975, 31200, 53975, 31800, 58775, 33000, 61200, 34300,
//62375, 35400, 62375, 37200, 61175, 38400, 60000, 38700, 57575, 38400, 54550, 37575, 50975, 36600,
//49075, 36125, 47750, 36125, 45700, 35425, 42350, 34350, 38900, 33775, 30575, 33000, 26975, 33600,
//25975, 34900, 26375, 36600, 28175, 38400, 30575, 40800, 32375, 43800, 33200, 46200, 33200, 48000,
//32650, 49300, 31425, 50000, 29950, 50125, 28825, 49375, 27575, 48000, 25825, 46000, 23975, 44100,
//22175, 42600, 19775, 39600, 17325, 37300, 14975, 34800, 13175, 31800, 10775, 29400, 9600, 27400,
//10175, 27000, 11375, 27600, 12575, 28800, 14375, 31800, 16175, 34800, 18575, 37200, 21575, 39000,
//22775, 40200, 23975, 41400, 24575, 42600, 26375, 44400, 28325, 46000, 29850, 46775, 31175, 46200,
//31550, 44575, 30575, 43200, 28775, 40800, 25775, 38400, 24575, 34800, 24750, 33175, 26975, 31800,
//29975, 31800, 33575, 31800, 37775, 32400, 39575, 33000, 41975, 33600, 45150, 34175, 46975, 34750,
//48575, 35400, 50975, 35400, 51575, 34800, 51875, 33725, 50775, 32575, 48575, 31800, 45750, 30875,
//43775, 30600, 41375, 29400, 38975, 28800, 35975, 28200, 34775, 27600, 34175, 27000, 34775, 25800,
//37175, 25200, 40175, 25200, 43175, 25200, 46775, 25200, 50975, 25425, 53375, 25200, 55175, 24600,
//55525, 23450, 53975, 22200, 52775, 22200, 49075, 21850, 45950, 21925, 40775, 21600, 37775, 21600,
//35150, 21350, 34325, 20950, 34175, 19800, 35975, 19200, 38375, 19200, 40750, 18900, 42575, 18600,
//44375, 18000, 47975, 17400, 50375, 17125, 52025, 16625, 52775, 15600, 52100, 14625, 49675, 14125,
//48625, 14125, 46775, 14400, 44375, 15000, 41375, 15150, 37700, 15275, 34775, 15600, 32850, 15925,
//31775, 15600, 31425, 14875, 32375, 13800, 36575, 11400, 38975, 10200, 41375, 9000, 43075, 8150,
//43650, 7200, 43325, 6250, 42225, 5825, 40800, 6275, 38900, 6925, 35375, 8400, 32375, 10200,
//27575, 11400, 22775, 12600, 19775, 13225, 16775, 13800, 14975, 14400, 13050, 14000, 11975, 12600,
//    0, 0 };
//  std::vector<point_data<double> > handpoints;
//  for(unsigned int i = 0; i < 100000; i += 2) {
//    point_data<double>  pt(handcoords[i], handcoords[i+1]);
//    if(pt == point_data<double> (0, 0)) break;
//    handpoints.push_back(pt);
//  }
//  polygon_data<double> handpoly;
//  handpoly.set(handpoints.begin(), handpoints.end());
//  double spiralcoords [] = {
//37200, 3600, 42075, 4025, 47475, 5875, 51000, 7800, 55800, 12300, 59000, 17075, 60000, 20400,
//61200, 25800, 61200, 29400, 60600, 33600, 58800, 38400, 55800, 42600, 53200, 45625,
//49200, 48600, 43200, 51000, 35400, 51600, 29400, 50400, 23400, 47400, 19200, 43800,
//16200, 39600, 14400, 35400, 13200, 29400, 13200, 24000, 15000, 18600, 17400, 13800,
//20525, 10300, 24600, 7200, 29400, 4800, 32450, 4000, 34825, 3675, 35625, 3625,
//35825, 7275, 39600, 7200, 43800, 8400, 46800, 9600, 50400, 12000, 53400, 15000,
//55800, 18600, 57000, 23400, 57600, 27000, 57000, 32400, 55200, 37200, 52200, 41400,
//48000, 45000, 42000, 47400, 35400, 48000, 30000, 46800, 24600, 43800, 20325, 39100,
//17850, 34275, 16800, 27600, 17400, 22200, 20400, 16200, 24600, 11400, 28800, 9000,
//32400, 7800, 33200, 7575, 33925, 11050, 35400, 10800, 37200, 10800, 41400, 11400,
//46200, 13200, 49800, 16200, 51600, 19200, 53400, 23400, 54000, 29400, 52800, 33600,
//49800, 39000, 45000, 42600, 39000, 44400, 33600, 43800, 28200, 42000, 24000, 37800,
//21000, 33000, 20400, 26400, 21600, 21000, 24600, 16200, 28200, 13200, 31875, 11625,
//33200, 15625, 36000, 15000, 39000, 15000, 43800, 16800, 46800, 19200, 49200, 23400,
//49800, 27600, 48750, 32700, 46350, 36275, 42600, 39000, 38400, 40200, 31800, 39000,
//28200, 36600, 25200, 31200, 24600, 26400, 26025, 21800, 28200, 18600, 30600, 16800,
//32575, 19875, 34200, 19200, 36000, 18600, 37200, 18600, 40375, 19125, 43200, 21000,
//45600, 24000, 46200, 27600, 45600, 30600, 43800, 33600, 41475, 35625, 37800, 36600,
//33600, 36000, 30000, 33600, 28200, 28800, 28800, 24600, 30000, 22200, 31200, 23400,
//30600, 25200, 30000, 27000, 30600, 30000, 31800, 32400, 34200, 34200, 38400, 34800,
//41400, 33000, 44025, 30225, 44400, 26400, 43200, 23400, 40900, 21200, 37800, 20400,
//34950, 20675, 32400, 22200, 30175, 19475, 28425, 21300, 27000, 24000, 26400, 27600,
//27000, 31800, 31200, 36600, 36600, 38400, 42600, 37200, 46200, 33600, 48000, 30000,
//47650, 24425, 45600, 20400, 42650, 18200, 39000, 16800, 35400, 16800, 33600, 17400,
//32875, 17675, 31100, 13850, 28200, 15600, 25200, 18600, 22800, 22800, 22200, 27000,
//23400, 33600, 26400, 38400, 31675, 41575, 37800, 42600, 40850, 42150, 42800, 41550,
//47050, 39025, 50100, 35375, 52200, 29400, 51675, 23950, 49800, 19200, 46200, 15600,
//41400, 13200, 37800, 12600, 35025, 12750, 33350, 13050, 32400, 9600, 30025, 10325,
//25925, 12725, 22200, 16800, 19800, 21000, 18600, 25800, 18600, 30000, 20400, 35400,
//22575, 39250, 25225, 41825, 28200, 43800, 33600, 46200, 39000, 46200, 44400, 45000,
//48650, 42350, 52800, 37800, 55200, 32400, 55800, 26400, 54600, 21000, 53400, 18000,
//50400, 14400, 47400, 12000, 42600, 9600, 39000, 9000, 36000, 9000, 34775, 9125,
//34300, 5600, 30000, 6600, 25800, 8400, 22025, 11350, 18725, 15125, 16200, 20400,
//15000, 24600, 15000, 30600, 16800, 36600, 20400, 42600, 25800, 46800, 31200, 49200,
//38400, 49800, 45000, 48600, 51000, 45000, 55475, 40225, 58200, 34800, 59400, 30000,
//59400, 25200, 58200, 19800, 55200, 14400, 52225, 11150, 47400, 7800, 44175, 6500,
//40200, 5400, 38400, 5400, 37200, 5400, 0, 0 };
//  std::vector<point_data<double> > spiralpoints;
//  for(unsigned int i = 0; i < 100000; i += 2) {
//    point_data<double>  pt(spiralcoords[i], spiralcoords[i+1]);
//    if(pt == point_data<double> (0, 0)) break;
//    spiralpoints.push_back(pt);
//  }
//  polygon_data<double> spiralpoly;
//  spiralpoly.set(spiralpoints.begin(), spiralpoints.end());
//  polygon_set_data<double> handset;
//  handset += handpoly;
//  polygon_set_data<double> spiralset;
//  spiralset += spiralpoly;
//  polygon_set_data<double> xorset = handset ^ spiralset;
//  std::vector<polygon_data<double> > polys;
//  polys += xorset;
//  std::cout << polys.size() << std::endl;
//  for(unsigned int i = 0; i < polys.size(); ++i)
//    std::cout << polys[i] << std::endl;
//}

bool testDirectionalSize() {
  {
    PolygonSet ps(VERTICAL);
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(0, -10, 0, -10);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(0, 0, 90, 90) << std::endl;
    if(rects[0] != Rectangle(0, 0, 90, 90)) return false;
  }
  {
    PolygonSet ps(VERTICAL);
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(0, 0, 0, -10);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(0, 0, 100, 90) << std::endl;
    if(rects[0] != Rectangle(0, 0, 100, 90)) return false;
  }
  {
    PolygonSet ps;
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(0, -10, 0, 0);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(0, 0, 90, 100) << std::endl;
    if(rects[0] != Rectangle(0, 0, 90, 100)) return false;
  }
  {
    PolygonSet ps;
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(0, 0, -10, 0);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(0, 10, 100, 100) << std::endl;
    if(rects[0] != Rectangle(0, 10, 100, 100)) return false;
  }
  {
    PolygonSet ps;
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(-10, 0, 0, 0);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(10, 0, 100, 100) << std::endl;
    if(rects[0] != Rectangle(10, 0, 100, 100)) return false;
  }
  {
    PolygonSet ps;
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(-10, 10, 0, 0);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(10, 0, 110, 100) << std::endl;
    if(rects[0] != Rectangle(10, 0, 110, 100)) return false;
  }
  {
    PolygonSet ps;
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(-10, 10, 10, -10);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(10, -10, 110, 90) << std::endl;
    if(rects[0] != Rectangle(10, -10, 110, 90)) return false;
  }
  {
    PolygonSet ps;
    ps += Rectangle(0, 0, 100, 100);
    ps.resize(10, 10, -10, -10);
    std::vector<Rectangle> rects;
    ps.get(rects);
    if(rects.size() != 1) return false;
    std::cout << rects[0] << std::endl;
    std::cout << Rectangle(-10, 10, 110, 90) << std::endl;
    if(rects[0] != Rectangle(-10, 10, 110, 90)) return false;
  }
  return true;
}

bool testMaxCover() {
  std::vector<Rectangle> rects;
  rects.push_back(Rectangle(Interval(60, 124), Interval( 1, 3)));
  rects.push_back(Rectangle(Interval(59, 83), Interval( 9, 28)));
  rects.push_back(Rectangle(Interval(90, 124), Interval( 3, 29)));
  rects.push_back(Rectangle(Interval(64, 124), Interval( 29, 35)));
  rects.push_back(Rectangle(Interval(64, 102), Interval( 35, 49)));
  rects.push_back(Rectangle(Interval(1, 20), Interval( 44, 60)));
  rects.push_back(Rectangle(Interval(50, 102), Interval( 49, 71)));
  rects.push_back(Rectangle(Interval(49, 102), Interval( 71, 72)));
  rects.push_back(Rectangle(Interval(49, 94), Interval( 72, 75)));
  rects.push_back(Rectangle(Interval(50, 74), Interval( 75, 81)));
  rects.push_back(Rectangle(Interval(90, 127), Interval( 75, 81)));
  rects.push_back(Rectangle(Interval(50, 127), Interval( 81, 82)));
  rects.push_back(Rectangle(Interval(3, 7), Interval( 60, 88)));
  rects.push_back(Rectangle(Interval(50, 92), Interval( 82, 94)));
  rects.push_back(Rectangle(Interval(58, 92), Interval( 94, 111)));
  std::vector<Rectangle> expected_result;
  expected_result.push_back(Rectangle(Interval(60, 124), Interval( 1, 3)));
  expected_result.push_back(Rectangle(Interval(90, 124), Interval( 1, 35)));
  expected_result.push_back(Rectangle(Interval(90, 102), Interval( 1, 72)));
  expected_result.push_back(Rectangle(Interval(90, 94 ), Interval(1 ,82)));
  expected_result.push_back(Rectangle(Interval(90, 92), Interval( 1, 111)));
  expected_result.push_back(Rectangle(Interval(59, 83 ), Interval(9, 28)));
  expected_result.push_back(Rectangle(Interval(64, 124), Interval( 29, 35)));
  expected_result.push_back(Rectangle(Interval(64, 102), Interval( 29, 72)));
  expected_result.push_back(Rectangle(Interval(64, 94), Interval( 29, 75)));
  expected_result.push_back(Rectangle(Interval(64, 74), Interval( 29, 111)));
  expected_result.push_back(Rectangle(Interval(1, 20), Interval( 44, 60)));
  expected_result.push_back(Rectangle(Interval(3, 7), Interval( 44, 88)));
  expected_result.push_back(Rectangle(Interval(50, 102 ), Interval(49, 72)));
  expected_result.push_back(Rectangle(Interval(50, 94), Interval( 49, 75)));
  expected_result.push_back(Rectangle(Interval(50, 74), Interval( 49, 94)));
  expected_result.push_back(Rectangle(Interval(58, 74), Interval( 49, 111)));
  expected_result.push_back(Rectangle(Interval(49, 102 ), Interval(71, 72)));
  expected_result.push_back(Rectangle(Interval(49, 94 ), Interval(71, 75)));
  expected_result.push_back(Rectangle(Interval(90, 127), Interval( 75, 82)));
  expected_result.push_back(Rectangle(Interval(50, 127), Interval( 81, 82)));
  expected_result.push_back(Rectangle(Interval(50, 92), Interval( 81, 94)));
  expected_result.push_back(Rectangle(Interval(58, 92), Interval( 81, 111)));
  std::vector<Rectangle> result;
  get_max_rectangles(result, rects);
  std::cout << "result XOR clean: " << equivalence(result, rects) << std::endl;
  std::cout << "expected result XOR clean: " << equivalence(expected_result, rects) << std::endl;
  std::vector<Rectangle>& output = result;
  std::vector<Rectangle>& voutput = expected_result;
  std::sort(output.begin(), output.end(), less_rectangle_concept< Rectangle, Rectangle>());
  std::sort(voutput.begin(), voutput.end(), less_rectangle_concept< Rectangle, Rectangle>());
  if(output != voutput) {
    std::cerr << "Max Rectangle TEST failed\n";
    for(unsigned int i = 0; i < output.size(); ++i) {
      std::cerr << output[i] << std::endl;
    }
    std::cerr << "Incorrect result\n";
    for(unsigned int i = 0; i < voutput.size(); ++i) {
      std::cerr << voutput[i] << std::endl;
    }
    std::cerr << "Max Rectangle TEST failed\n";
    for(unsigned int i = 0; i < rects.size(); ++i) {
      std::cout << rects[i] << std::endl;
    }
    return false;
  }
  return true;
}

void max_cover_stress_test() {
  for(unsigned int k = 3; k < 20; k++) {
    for(unsigned int i = 0; i < k * k; ++i) {
      std::vector<Rectangle> rects, result;
      //std::cout << "test " << i << std::endl;
      for(unsigned int j = 0; j < k; ++j) {
        int x1 = rand() % 100;
        int x2 = rand() % 50;
        int y1 = rand() % 100;
        int y2 = rand() % 50;
        rects.push_back(Rectangle(x1, y1, x1+x2, y1+y2));
        //std::cout << rects.back() << std::endl;
      }
      get_max_rectangles(result, rects);
    }
  }
}

// namespace boost { namespace polygon{
//   template <typename GCT, typename T>
//   struct view_of {};

//   template <typename T>
//   struct view_of<polygon_45_concept, T> {
//     const T* t;
//     view_of(const T& obj) : t(&obj) {}
//     typedef typename polygon_traits<T>::coordinate_type coordinate_type;
//     typedef typename polygon_traits<T>::iterator_type iterator_type;
//     typedef typename polygon_traits<T>::point_type point_type;

//     /// Get the begin iterator
//     inline iterator_type begin() const {
//       return polygon_traits<T>::begin_points(*t);
//     }

//     /// Get the end iterator
//     inline iterator_type end() const {
//       return polygon_traits<T>::end_points(*t);
//     }

//     /// Get the number of sides of the polygon
//     inline unsigned int size() const {
//       return polygon_traits<T>::size(*t);
//     }

//     /// Get the winding direction of the polygon
//     inline winding_direction winding() const {
//       return polygon_traits<T>::winding(*t);
//     }
//   };

//   template <typename T1, typename T2>
//   view_of<T1, T2> view_as(const T2& obj) { return view_of<T1, T2>(obj); }

//   template <typename T>
//   struct geometry_concept<view_of<polygon_45_concept, T> > {
//     typedef polygon_45_concept type;
//   };

//   template <typename T>
//   struct view_of<polygon_90_concept, T> {
//     const T* t;
//     view_of(const T& obj) : t(&obj) {}
//     typedef typename polygon_traits<T>::coordinate_type coordinate_type;
//     typedef typename polygon_traits<T>::iterator_type iterator_type;
//     typedef typename polygon_traits<T>::point_type point_type;
//     typedef iterator_points_to_compact<iterator_type, point_type> compact_iterator_type;

//     /// Get the begin iterator
//     inline compact_iterator_type begin_compact() const {
//       return compact_iterator_type(polygon_traits<T>::begin_points(*t),
//                                    polygon_traits<T>::end_points(*t));
//     }

//     /// Get the end iterator
//     inline compact_iterator_type end_compact() const {
//       return compact_iterator_type(polygon_traits<T>::end_points(*t),
//                                    polygon_traits<T>::end_points(*t));
//     }

//     /// Get the number of sides of the polygon
//     inline unsigned int size() const {
//       return polygon_traits<T>::size(*t);
//     }

//     /// Get the winding direction of the polygon
//     inline winding_direction winding() const {
//       return polygon_traits<T>::winding(*t);
//     }
//   };

//   template <typename T>
//   struct geometry_concept<view_of<polygon_90_concept, T> > {
//     typedef polygon_90_concept type;
//   };
// }}
using namespace gtl;

//this test fails and I'd like to get it to pass
bool test_colinear_duplicate_points() {
  Point pts[6] = { Point(0, 10), Point(0, 0), Point(100, 0), Point(100, 100), Point(0, 100), Point(0, 10)};
  Polygon45 p1;
  p1.set(pts, pts+5);
  Polygon45 pg;
  pg.set(pts, pts+6);
  Polygon45 p2;
  p2.set(pts+1, pts+6);
  std::cout << p2 << std::endl;
  if(!equivalence(view_as<polygon_90_concept>(p2), view_as<polygon_90_concept>(pg))) return false;
  std::cout << p1 << std::endl;
  if(!equivalence(view_as<polygon_90_concept>(p1), view_as<polygon_90_concept>(pg))) return false;
  return true;
}

bool test_extents() {
  PolygonSet psT(gtl::VERTICAL);
  //int xy[] = { 126, 69, 54, 69, 54, 81, 126, 81 };
  //CPolygonQuery polygon(0, 4, xy);
  //Rectangle rectIn(54, 69, 126, 81);
  polygon_data<int> polygon;
  std::vector<Point> pts;
  pts.push_back(Point(126, 69));
  pts.push_back(Point(54, 69));
  pts.push_back(Point(54, 81));
  pts.push_back(Point(126, 81));
  set_points(polygon, pts.begin(), pts.end());
  psT.insert(view_as<polygon_90_concept>(polygon));

  Rectangle rect, rect2;
  psT.extents(rect2);
  gtl::extents(rect, psT);

  if (rect != rect2) {
    std::cout << "gtl::Rectangles differ: " << gtl::xl(rect)  << " " << gtl::xh(rect)  << " " << gtl::yl(rect)  << " " << gtl::yh(rect)  <<     std::endl;
        std::cout << "                        " << gtl::xl(rect2) << " " << gtl::xh(rect2) << " " << gtl::yl(rect2) << " " << gtl::yh(rect2) <<     std::endl;
    return false;
  }
  return true;
}

bool test_extents2() {
  Polygon45Set psT;
  Point xy[] = { Point(130, 50),   Point(50, 50),   Point(50, 100),   Point(119, 100),
                 Point(119, 59),   Point(89, 89),   Point(59, 59),   Point(119, 59),   Point(119, 100),  Point(130, 100) };
  Polygon45 polygon(xy, xy+10);

  psT.insert(polygon);
  psT += 2;

  Rectangle rect, rect2;
  psT.extents(rect2);
  gtl::extents(rect, psT);
  std::cout << "Extents: " << gtl::xl(rect)   << " " << gtl::xh(rect)   << " " << gtl::yl(rect)   << " " << gtl::yh(rect)  <<   std::endl;
    std::cout << "Extents: " << gtl::xl(rect2)  << " " << gtl::xh(rect2)  << " " << gtl::yl(rect2)  << " " << gtl::yh(rect2) <<   std::endl;
    std::vector<Polygon45WithHoles> pwhs;
    psT.get(pwhs);
    for(unsigned int i = 0; i < pwhs.size(); ++i) {
      std::cout << pwhs[i] << std::endl;
    }
  return gtl::equivalence(rect, rect2);
}

/*************New Polygon Formation Tests********************/
/*
 *
 * Test Input:
 * +--------------------+
 * |        +-------+   |
 * |        |       |   |
 * |        |       |   |
 * +-----+  |       |   |
 *       |  |       |   |
 *       |  |       |   |
 * +-----+  |       |   |
 * |        |       |   |
 * |        |       |   |
 * |        +-------+   |
 * +--------+           |
 *          |           |
 *          |           |
 * +--------+           |
 * |                    |
 * |                    |
 * +--------+           |
 *          |           |
 *          |           |
 * +--------+           |
 * |                    |
 * |                    |
 * +--------------------+
 *
 * Test Plan: 
 * a. call 'get(out, param)' , param >=4 
 * b. check if each polygon in the container is <= param
 * c. check the area of all the pieces sum up to original piece
 */
typedef int intDC;
typedef boost::polygon::polygon_90_with_holes_data<intDC> GTLPolygon;
typedef boost::polygon::polygon_90_set_data<intDC> GTLPolygonSet;
typedef boost::polygon::polygon_90_concept GTLPolygonConcept;
typedef boost::polygon::point_data<intDC> GTLPoint;
inline void PrintPolygon(const GTLPolygon&);
inline GTLPolygon CreateGTLPolygon(const int*, size_t); 
int test_new_polygon_formation(int argc, char** argv){
   //                                               //
   // Sub-Test-1: do a Boolean and call the new get //
   //                                               //
   int coordinates[] = {0,0, 10,0, 10,10, 0,10};
   int coordinates1[] = {9,1, 20,1, 20,10, 9,10};
   std::vector<GTLPoint> pts;
   size_t count = sizeof(coordinates)/(2*sizeof(intDC)); 
   size_t count1 = sizeof(coordinates1)/(2*sizeof(intDC));
   GTLPolygon poly, poly1;
   GTLPolygonSet polySet;
   
   poly = CreateGTLPolygon(coordinates, count);
   poly1 = CreateGTLPolygon(coordinates1, count1);

   polySet.insert(poly);
   polySet.insert(poly1);

   std::vector<GTLPolygon> result;
   polySet.get(result, 100);

   if(result.size() > 1){
      std::cerr << "FAILED: expecting only one polygon because the"
         " threshold is 100" << std::endl;
      return 1;
   }

   if(result[0].size() != 6){
      std::cerr << "FAILED: expecting only 6 vertices" << std::endl;
      return 1;
   }

   if(area(result[0]) != 190){
      std::cerr <<"FAILED: expecting only 6 vertices" << std::endl;
      return 1;
   }

   //expect no more than 1 polygon
   std::cout << "Found " << result.size() << "polygons after union" 
      << std::endl;
   for(size_t i=0; i<result.size(); i++){
      PrintPolygon(result[i]);
   }

   intDC shell_coords[] = {0,0, 10,0, 10,21, 0,21, 0,15, 3,15, 3,13,
      0,13, 0,10, 5,10, 5,8, 0,8, 0,5, 5,5, 5,3, 0,3};
   intDC hole_coords[] = {4,11, 7,11, 7,19, 4,19};
   GTLPolygon slice_polygon, slice_hole;
   count = sizeof(shell_coords)/(2*sizeof(intDC));
   count1 = sizeof(hole_coords)/(2*sizeof(intDC));

   slice_polygon = CreateGTLPolygon(shell_coords, count);
   slice_hole = CreateGTLPolygon(hole_coords, count1);

   result.clear();
   polySet.clear();
   polySet.insert(slice_polygon);
   polySet.insert(slice_hole, true);

   polySet.get(result);
   double gold_area = 0;
   std::cout << "Found " << result.size() << " slices" << std::endl;
   for(size_t i=0; i<result.size(); i++){
      PrintPolygon(result[i]);
      gold_area += area(result[i]); 
   }

   result.clear();
   polySet.get(result, 6);
   double platinum_area = 0;
   std::cout << "Found " << result.size() << " slices" << std::endl;
   for(size_t i=0; i<result.size(); i++){
      PrintPolygon(result[i]);
      platinum_area += area(result[i]); 
      if(result[i].size() > 6){
         std::cerr << "FAILED: expecting size to be less than 6" << std::endl;
         return 1;
      }
   }

   std::cout << "platinum_area = " << platinum_area << " , gold_area=" 
      << gold_area << std::endl;
   if( platinum_area != gold_area){
      std::cerr << "FAILED: Area mismatch" << std::endl;
      return 1;
   }
   std::cout << "[SUB-TEST-1] PASSED\n";

   result.clear();
   polySet.get(result, 4);
   platinum_area = 0;
   std::cout << "Found " << result.size() << " slices" << std::endl;
   for(size_t i=0; i<result.size(); i++){
      PrintPolygon(result[i]);
      platinum_area += area(result[i]); 
      if(result[i].size() > 4){ 
         std::cerr << "FAILED: expecting size to be < 4" << std::endl;
         return 1;
      }
   }

   std::cout << "platinum_area=" << platinum_area << ", gold_area=" 
      << gold_area << std::endl;

   if( platinum_area != gold_area){
      std::cerr << "FAILED: Area mismatch" << std::endl;
      return 1;
   }

   std::cout << "[SUB-TEST-1] PASSED" << std::endl;
   return 0;
}

/* 
 * INPUT:
 *   +--------+
 *   |        |
 *   |        |
 *   |        +---+
 *   |            |
 *   |        +---+
 *   |        |
 *   +--------+
 *            X 
 *            
 * TEST PLAN: as the sweepline moves and reaches
 * X the number of vertices in the solid jumps by 4
 * instead of 2. So make sure we don't get a 6 vertex
 * polygon when the threshold is 4 and 6.
 */
int test_new_polygon_formation_marginal_threshold(int argc, char**){
   std::vector<GTLPoint> pts;
   GTLPolygon polygon;
   GTLPolygonSet pset;
   std::vector<GTLPolygon> result;
   intDC coords[] = {0,0, 15,0, 15,10, 10,10, 10,15, 5,15, 5,10, 0,10};
   size_t count = sizeof(coords)/(2*sizeof(intDC));

   polygon = CreateGTLPolygon(coords, count);
   pset.insert(polygon);

   for(size_t i=0; i<1; i++){
      pset.get(result, i ? 4 : 6);
      double gold_area = 175, plat_area = 0;
      for(size_t i=0; i<result.size(); i++){
         if(result[i].size() > (i ? 4 : 6) ){
            size_t expected = i ? 4 : 6;
            std::cerr << "FAILED: Expecting no more than " <<
               expected << " vertices" << std::endl;
            return 1;
         }
         PrintPolygon(result[i]);
         plat_area += area(result[i]); 
      }

      if(plat_area != gold_area){
         std::cerr << "FAILED area mismatch gold=" << gold_area <<
            " plat=" << plat_area << std::endl;
         return 1;
      }
   }
   std::cout << "Test Passed" << std::endl;
   return 0;
}

inline void PrintPolygon(const GTLPolygon& p){
   //get an iterator of the point_data<int>
   boost::polygon::point_data<int> pt;
   boost::polygon::polygon_90_data<int>::iterator_type itr;

   size_t vertex_id = 0;
   for(itr = p.begin(); itr != p.end(); ++itr){
      pt = *itr;
      std::cout << "Vertex-" << ++vertex_id << "(" << pt.x() <<
         "," << pt.y() << ")" << std::endl;
   }
}

// size: is the number of vertices //
inline GTLPolygon CreateGTLPolygon(const int *coords, size_t size){
   GTLPolygon r;
   std::vector<GTLPoint> pts;

   for(size_t i=0; i<size; i++){
      pts.push_back( GTLPoint(coords[2*i], coords[2*i+1]) );
   }
   boost::polygon::set_points(r, pts.begin(), pts.end());
   return r;
}

/************************************************************/

int main() {
  test_view_as();
  //this test fails and I'd like to get it to pass
  //if(!test_colinear_duplicate_points()) return 1;
  if(!test_extents2()) return 1;
  if(!test_extents()) return 1;
  if(!testMaxCover()) return 1;
  //max_cover_stress_test(); //does not include functional testing
  if(!testDirectionalSize()) return 1;
  testHand();
  //testHandFloat();
  if(!testpip()) return 1;
  {
    PolygonSet ps;
    Polygon p;
    assign(ps, p);
  }
  if(!testViewCopyConstruct()) return 1;
  if(!test_grow_and_45()) return 1;
  if(!test_self_xor_45()) return 1;
  if(!test_self_xor()) return 1;
  if(!test_directional_resize()) return 1;
  if(!test_scaling_by_floating()) return 1;
  if(!test_SQRT1OVER2()) return 1;
  if(!test_get_trapezoids()) return 1;
  if(!test_get_rectangles()) return 1;
  if(!test_45_concept_interact()) return 1;
  if(!test_45_touch_r()) return 1;
  if(!test_45_touch_ur()) return 1;
  if(!test_45_touch()) return 1;
  if(!test_45_touch_boundaries()) return 1;
  {
  Point pts[] = {Point(0,0), Point(5, 5), Point(5, 0)};
  Polygon45 p45(pts, pts+3);
  pts[1] = Point(0, 5);
  Polygon45 p452(pts, pts+3);
  if(!test_two_polygons(p45,p452)) return 1;
  pts[2] = Point(5,5);
  p45.set(pts, pts+3);
  if(!test_two_polygons(p45,p452)) return 1;
  pts[0] = Point(5,0);
  p452.set(pts, pts+3);
  if(!test_two_polygons(p45, p452)) return 1;
  Point pts2[] = {Point(0,5), Point(5, 5), Point(5, 0)};
  Point pts3[] = {Point(0,0), Point(5, 5), Point(5, 0)};
  p45.set(pts2, pts2 + 3);
  p452.set(pts3, pts3+3);
  if(!test_two_polygons(p45, p452)) return 1;
  Point pts4[] = {Point(0, 5), Point(3, 2), Point(3,5)};
  Point pts5[] = {Point(0,0), Point(5, 5), Point(5, 0)};
  p45.set(pts4, pts4+3);
  p452.set(pts5, pts5+3);
  if(!test_two_polygons(p45, p452)) return 1;
  }
  {
  std::vector<point_data<int> > pts;
  pts.push_back(point_data<int>(0, 0));
  pts.push_back(point_data<int>(10, 0));
  pts.push_back(point_data<int>(10, 10));
  pts.push_back(point_data<int>(0, 10));
  std::vector<point_data<int> > pts2;
  pts2.push_back(point_data<int>(0, 0));
  pts2.push_back(point_data<int>(10, 10));
  pts2.push_back(point_data<int>(0, 20));
  pts2.push_back(point_data<int>(-10, 10));
  std::vector<point_data<int> > pts3;
  pts3.push_back(point_data<int>(0, 0));
  pts3.push_back(point_data<int>(10, 11));
  pts3.push_back(point_data<int>(0, 20));
  pts3.push_back(point_data<int>(-100, 8));
  polygon_data<int> p, p1; p.set(pts3.begin(), pts3.end());
  polygon_45_data<int> p45, p451; p45.set(pts2.begin(), pts2.end());
  polygon_90_data<int> p90, p901; p90.set(pts.begin(), pts.end());
  polygon_with_holes_data<int> pwh, pwh1; pwh.set(pts3.begin(), pts3.end());
  polygon_45_with_holes_data<int> p45wh, p45wh1; p45wh.set(pts2.begin(), pts2.end());
  polygon_90_with_holes_data<int> p90wh, p90wh1; p90wh.set(pts.begin(), pts.end());
  assign(p, p90);
  assign(p, p45);
  assign(p1, p);
  //illegal: assign(p, p90wh);
  //illegal: assign(p, p45wh);
  //illegal: assign(p, pwh);

  assign(p45, p90);
  assign(p451, p45);
  //illegal: assign(p45, p);
  //illegal: assign(p45, p90wh);
  //illegal: assign(p45, p45wh);
  //illegal: assign(p45, pwh);

  assign(p901, p90);
  //illegal: assign(p90, p45);
  //illegal: assign(p90, p);
  //illegal: assign(p90, p90wh);
  //illegal: assign(p90, p45wh);
  //illegal: assign(p90, pwh);

  assign(pwh, p90);
  assign(pwh, p45);
  assign(pwh, p);
  assign(pwh, p90wh);
  assign(pwh, p45wh);
  assign(pwh1, pwh);

  assign(p45wh, p90);
  assign(p45wh, p45);
  //illegal: assign(p45wh, p);
  assign(p45wh, p90wh);
  assign(p45wh1, p45wh);
  //illegal: assign(p45wh, pwh);

  assign(p90wh, p90);
  //illegal: assign(p90wh, p45);
  //illegal: assign(p90wh, p);
  assign(p90wh1, p90wh);
  //illegal: assign(p90wh, p45wh);
  //illegal: assign(p90wh, pwh);
  pts.clear();
  pts.push_back(point_data<int>(0, 0));
  pts.push_back(point_data<int>(3, 0));
  pts.push_back(point_data<int>(0, 1));
  p.set(pts.begin(), pts.end());
  std::cout << std::endl; std::cout << (area(p90));
  std::cout << std::endl; std::cout << (area(p45));
  std::cout << std::endl; std::cout << (area(p));
  std::cout << std::endl; std::cout << (area(p90wh));
  std::cout << std::endl; std::cout << (area(p45wh));
  std::cout << std::endl; std::cout << (area(pwh));
  std::cout << std::endl;
  point_data<int> pt(1, 1);
  std::cout << contains(p, pt) << std::endl;
  std::cout << contains(p90, pt) << std::endl;

  interval_data<int> ivl = construct<interval_data<int> >(0, 10);
  std::cout << get(ivl, LOW) << std::endl;
  set(ivl, HIGH, 20);

  std::cout << perimeter(p) << std::endl;
  if(winding(p) == LOW) std::cout << "LOW" << std::endl;
  if(winding(p) == HIGH) std::cout << "HIGH" << std::endl;
  rectangle_data<polygon_long_long_type> rd;
  std::cout << extents(rd, p) << std::endl;
  std::cout << rd << std::endl;

  boolean_op::testBooleanOr<int>();

  std::vector<rectangle_data<int> > rects1, rects2;
  rects2.push_back(rectangle_data<int>(0, 0, 10, 10));
  print_is_polygon_90_set_concept((polygon_90_set_data<int>()));
  print_is_mutable_polygon_90_set_concept((polygon_90_set_data<int>()));
  print_is_polygon_90_set_concept((polygon_90_data<int>()));
  print_is_polygon_90_set_concept((std::vector<polygon_90_data<int> >()));
  assign(rects1, rects2);
  polygon_90_set_data<int> ps90;
  assign(ps90, rects2);
  assign(rects2, ps90);
  assign(ps90, p90);
  assign(rects2, p90);
  std::cout << p90 << std::endl;
  for(unsigned int i = 0; i < rects2.size(); ++i) {
    std::cout << rects2[i] << std::endl;
  }
  bloat(rects2, 10);
  shrink(rects2[0], 10);
  for(unsigned int i = 0; i < rects2.size(); ++i) {
    std::cout << rects2[i] << std::endl;
  }
  move(rects2[0], HORIZONTAL, 30);
  assign(rects1, rects2 + p90);
  std::cout << "result of boolean or\n";
  for(unsigned int i = 0; i < rects1.size(); ++i) {
    std::cout << rects1[i] << std::endl;
  }
  rects1 -= p90;
  std::cout << "result of boolean not\n";
  for(unsigned int i = 0; i < rects1.size(); ++i) {
    std::cout << rects1[i] << std::endl;
  }
  rects1 += p90;
  std::cout << "result of boolean OR\n";
  for(unsigned int i = 0; i < rects1.size(); ++i) {
    std::cout << rects1[i] << std::endl;
  }
  rects1 *= p90;
  std::cout << "result of boolean AND\n";
  for(unsigned int i = 0; i < rects1.size(); ++i) {
    std::cout << rects1[i] << std::endl;
  }
  rects1 ^= rects2;
  std::cout << "result of boolean XOR\n";
  for(unsigned int i = 0; i < rects1.size(); ++i) {
    std::cout << rects1[i] << std::endl;
  }
  rects2.clear();
  get_max_rectangles(rects2, p90);
  std::cout << "result of max rectangles\n";
  for(unsigned int i = 0; i < rects2.size(); ++i) {
    std::cout << rects2[i] << std::endl;
  }
  rects2.clear();
  //operator += and -= don't support polygons, so + and - should not exist
//   rects2 += p90 + 6;
//   std::cout << "result of resize\n";
//   for(unsigned int i = 0; i < rects2.size(); ++i) {
//     std::cout << rects2[i] << std::endl;
//   }
//   std::cout << "result of resize\n";
   std::vector<polygon_90_with_holes_data<int> > polyswh1, polyswh2;
//   polyswh1 += p90 -2;
//   for(unsigned int i = 0; i < polyswh1.size(); ++i) {
//     std::cout << polyswh1[i] << std::endl;
//   }
//   std::cout << "result of resize\n";
   std::vector<polygon_90_data<int> > polys1, polys2;
   polys1 += p90;
   polys1 -= 2;
//   polys1 += p90 -2;
   for(unsigned int i = 0; i < polys1.size(); ++i) {
     std::cout << polys1[i] << std::endl;
   }

   boolean_op_45<int>::testScan45(std::cout);
   polygon_45_formation<int>::testPolygon45Formation(std::cout);
  polygon_45_formation<int>::testPolygon45Tiling(std::cout);

  axis_transformation atr;
  transform(p, atr);
  transform(p45, atr);
  transform(p90, atr);
  transform(pwh, atr);
  transform(p45wh, atr);
  transform(p90wh, atr);
  scale_up(p, 2);
  scale_up(p45, 2);
  scale_up(p90, 2);
  scale_up(pwh, 2);
  scale_up(p45wh, 2);
  scale_up(p90wh, 2);
  scale_down(p, 2);
  scale_down(p45, 2);
  scale_down(p90, 2);
  scale_down(pwh, 2);
  scale_down(p45wh, 2);
  scale_down(p90wh, 2);
  std::vector<polygon_45_data<int> > p45s1, p45s2;
  std::cout << equivalence(p45s1, p45s2) << std::endl;
  std::cout << equivalence(p45, p45wh) << std::endl;
  std::cout << equivalence(p90, p45wh) << std::endl;
  gtl::assign(p45s1, p90);
  p90 = polys1[0];
  move(p90, orientation_2d(HORIZONTAL), 8);
  std::cout << p90 << std::endl << p45wh << std::endl;
  polygon_45_set_data<int> ps45 = p90 + p45wh;
  assign(p45s1, ps45);
  std::cout << "result\n";
  for(unsigned int i = 0; i < p45s1.size(); ++i) {
    std::cout << p45s1[i] << std::endl;
  }
  std::cout << equivalence(p, pwh) << std::endl;
  std::cout << equivalence(p90, pwh) << std::endl;
  std::cout << equivalence(p45, pwh) << std::endl;
  std::cout << equivalence(pwh, pwh) << std::endl;
  p + pwh;
  p90 + pwh;
  p45 + pwh;
  std::cout << testRectangle() << std::endl;
  std::cout << testPolygon() << std::endl;
  std::cout << testPropertyMerge() << std::endl;
  std::cout << testPolygonAssign() << std::endl;
  std::cout << testPolygonWithHoles() << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationRect(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationP1(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationP2(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationPolys(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationSelfTouch1(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationSelfTouch2(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationSelfTouch3(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testSegmentIntersection(std::cout)) << std::endl;
  std::cout << (property_merge<int, int>::test_insertion(std::cout)) << std::endl;
  std::cout << (line_intersection<int>::test_verify_scan(std::cout)) << std::endl;
  std::cout << (line_intersection<int>::test_validate_scan(std::cout)) << std::endl;
  std::cout << (scanline<int, int>::test_scanline(std::cout)) << std::endl;
  std::cout << (property_merge<int, int>::test_merge(std::cout)) << std::endl;
  std::cout << (property_merge<int, int>::test_intersection(std::cout)) << std::endl;
  std::cout << (polygon_arbitrary_formation<int>::testPolygonArbitraryFormationColinear(std::cout)) << std::endl;
  std::cout << (property_merge<int, int>::test_manhattan_intersection(std::cout)) << std::endl;
  std::cout << (test_arbitrary_boolean_op<int>(std::cout)) << std::endl;
  }
  {
    polygon_set_data<int> psd;
    rectangle_data<int> rect;
    set_points(rect, point_data<int>(0, 0), point_data<int>(10, 10));
    psd.insert(rect);
    polygon_set_data<int> psd2;
    set_points(rect, point_data<int>(5, 5), point_data<int>(15, 15));
    psd2.insert(rect);
    std::vector<polygon_data<int> > pv;
    polygon_set_data<int> psd3;
    psd3 = psd + psd2;
    psd3.get(pv);
    for(unsigned int i = 0; i < pv.size(); ++i) {
      std::cout << pv[i] << std::endl;
    }
    psd += psd2;
    pv.clear();
    psd3.get(pv);
    for(unsigned int i = 0; i < pv.size(); ++i) {
      std::cout << pv[i] << std::endl;
    }
  }
  {
    polygon_90_set_data<int> psd;
    rectangle_data<int> rect;
    set_points(rect, point_data<int>(0, 0), point_data<int>(10, 10));
    psd.insert(rect);
    polygon_90_set_data<int> psd2;
    set_points(rect, point_data<int>(5, 5), point_data<int>(15, 15));
    psd2.insert(rect);
    std::vector<polygon_90_data<int> > pv;
    interact(psd, psd2);
    assign(pv, psd);
    for(unsigned int i = 0; i < pv.size(); ++i) {
      std::cout << pv[i] << std::endl;
    }

    connectivity_extraction_90<int> ce;
    ce.insert(pv[0]);
    ce.insert(psd2);
    std::vector<std::set<int> > graph(2);
    ce.extract(graph);
    if(graph[0].size() == 1) std::cout << "connectivity extraction is alive\n";

    //std::vector<rectangle_data<polygon_long_long_type> > lobs;
    //get_max_rectangles(lobs, psd);
    //if(lobs.size() == 1) std::cout << "max rectangles is alive\n";

    std::vector<rectangle_data<int> > rv;
    rv.push_back(rect);
    set_points(rect, point_data<int>(0, 0), point_data<int>(10, 10));
    rv.push_back(rect);
    self_intersect(rv);
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }

    assign(rv, rv + 1);
    std::cout << rv.size() << std::endl;
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    assign(rv, rv - 1);
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    rv += 1;
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    rv -= 1;
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    rv.clear();
    set_points(rect, point_data<int>(0, 0), point_data<int>(10, 10));
    rv.push_back(rect);
    set_points(rect, point_data<int>(12, 12), point_data<int>(20, 20));
    rv.push_back(rect);
    grow_and(rv, 7);
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    std::cout << area(rv) << std::endl;
    std::cout << area(rv) << std::endl;

    scale_up(rv, 10);
    std::cout << area(rv) << std::endl;
    scale_down(rv, 7);
    std::cout << area(rv) << std::endl;
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    keep(rv, 290, 300, 7, 24, 7, 24);
    if(rv.size() == 1) {
      assign(rect, rv.back());
      std::cout << rect << std::endl;
    }
    keep(rv, 300, 310, 7, 24, 7, 24);
    if(rv.empty()) std::cout << "keep is alive\n";
  }
  {
//     typedef int Unit;
//     typedef point_data<int> Point;
//     typedef interval_data<int> Interval;
//     typedef rectangle_data<int> Rectangle;
//     typedef polygon_90_data<int> Polygon;
//     typedef polygon_90_with_holes_data<int> PolygonWithHoles;
//     typedef polygon_45_data<int> Polygon45;
//     typedef polygon_45_with_holes_data<int> Polygon45WithHoles;
//     typedef polygon_90_set_data<int> PolygonSet;
//     //typedef polygon_45_set_data<int> Polygon45Set;
//     typedef axis_transformation AxisTransform;
//     typedef transformation<int> Transform;
    //test polygon45 area, polygon45 with holes area
    std::vector<Point> pts;
    pts.clear();
    pts.push_back(Point(10, 10));
    pts.push_back(Point(15, 10));
    pts.push_back(Point(10, 15));
    Polygon45 polyHole;
    polyHole.set(pts.begin(), pts.end());
    pts.clear();
    pts.push_back(Point(10, 0));
    pts.push_back(Point(20, 10));
    pts.push_back(Point(20, 30));
    pts.push_back(Point(0, 50));
    pts.push_back(Point(0, 10));
    Polygon45WithHoles polyWHoles;
    polyWHoles.set(pts.begin(), pts.end());
    polyWHoles.set_holes(&polyHole, (&polyHole)+1);
     std::cout << polyWHoles << std::endl;
    std::cout << area(polyWHoles) << std::endl;
    std::cout << area(polyWHoles) << std::endl;
     //test polygon45, polygon45with holes transform
     AxisTransform atr(AxisTransform::EAST_SOUTH);
     Polygon45WithHoles p45wh(polyWHoles);
     transform(polyWHoles, atr);
     std::cout << polyWHoles << std::endl;
     Transform tr(atr);
     tr.invert();
     transform(polyWHoles, tr);
     std::cout << polyWHoles << std::endl;
     if(area(polyWHoles) != 687.5) return 1;
     //test polygon, polygon with holes transform
     Polygon ph;
     assign(ph, Rectangle(10, 10, 20, 20));
     PolygonWithHoles pwh;
     assign(pwh, Rectangle(0, 0, 100, 100));
     pwh.set_holes(&ph, (&ph)+1);
     std::cout << area(pwh) << std::endl;
     transform(pwh, atr);
     std::cout << pwh << std::endl;
     std::cout << area(pwh) << std::endl;
     transform(pwh, tr);
     std::cout << pwh << std::endl;
     std::cout << area(pwh) << std::endl;
     if(area(pwh) != 9900) return 1;

    //test point scale up / down
    Point pt(10, 10);
    scale_up(pt, 25);
    if(pt != Point(250, 250)) return 1;
    std::cout << pt << std::endl;
    scale_down(pt, 25);
    if(pt != Point(10, 10)) return 1;
    std::cout << pt << std::endl;
    scale_down(pt, 25);
    if(pt != Point(0, 0)) return 1;
    std::cout << pt << std::endl;

    //test polygon, polygon with holes scale up down
    PolygonWithHoles tmpPwh(pwh);
    scale_up(pwh, 25);
    std::cout << pwh << std::endl;
    scale_down(pwh, 25);
    if(area(pwh) != area(tmpPwh)) return 1;
    std::cout << pwh << std::endl;
    scale_down(pwh, 25);
    std::cout << pwh << std::endl;
    //test polygon45, polygon45 with holes is45
    std::cout << is_45(polyHole) << std::endl;
    if(is_45(polyHole) != true) return 1;
    pts.clear();
    pts.push_back(Point(10, 10));
    pts.push_back(Point(15, 10));
    pts.push_back(Point(10, 16));
    polyHole.set(pts.begin(), pts.end());
    std::cout << is_45(polyHole) << std::endl;
    if(is_45(polyHole) != false) return 1;
    //test polygon45, polygon45 with holes snap 45
    snap_to_45(polyHole);
    std::cout << is_45(polyHole) << std::endl;
    if(is_45(polyHole) != true) return 1;
    std::cout << polyHole << std::endl;
    //test polygon45, polygon45 with holes scalue up down
    scale_up(polyHole, 10000);
    std::cout << polyHole << std::endl;
    scale_down(polyHole, 3);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 5);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 7);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 13);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_up(polyHole, 3);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    pts.clear();
    pts.push_back(Point(11, 1));
    pts.push_back(Point(21, 11));
    pts.push_back(Point(11, 21));
    pts.push_back(Point(1, 11));
    polyHole.set(pts.begin(), pts.end());
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 3);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_up(polyHole, 10000);
    std::cout << polyHole << std::endl;
    scale_down(polyHole, 3);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 5);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 7);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 13);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_up(polyHole, 3);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;
    scale_down(polyHole, 2);
    std::cout << is_45(polyHole) << " " << polyHole << std::endl;
    if(is_45(polyHole) != true) return 1;

    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_up(polyWHoles, 100013);
    std::cout << polyWHoles << std::endl;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 3);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;
    scale_down(polyWHoles, 2);
    std::cout << is_45(polyWHoles) << " " << polyWHoles << std::endl;
    if(is_45(polyWHoles) != true) return 1;

    std::cout << (boolean_op_45<Unit>::testScan45(std::cout)) << std::endl;
    std::cout << (polygon_45_formation<Unit>::testPolygon45Formation(std::cout)) << std::endl;
    std::cout << (polygon_45_formation<Unit>::testPolygon45Tiling(std::cout)) << std::endl;


    {
      PolygonSet ps;
      Rectangle rect;
      ps.insert(Rectangle(0, 0, 10, 10));
      std::cout << area(ps) << std::endl;
      if(area(ps) != 100) return 1;
      scale_up(ps, 3);
      std::cout << area(ps) << std::endl;
      if(area(ps) != 900) return 1;
      scale_down(ps, 2);
      std::cout << area(ps) << std::endl;
      if(area(ps) != 225) return 1;
      transform(ps, atr);
      std::vector<Rectangle> rv;
      rv.clear();
      ps.get(rv);
      if(rv.size() == 1) {
        assign(rect, rv.back());
        std::cout << rect << std::endl;
      }
      transform(ps, tr);
      rv.clear();
      ps.get(rv);
      if(rv.size() == 1) {
        assign(rect, rv.back());
        std::cout << rect << std::endl;
      }
    }
    //test polygon45set transform
    pts.clear();
    pts.push_back(Point(10, 10));
    pts.push_back(Point(15, 10));
    pts.push_back(Point(10, 15));
    polyHole.set(pts.begin(), pts.end());
    Polygon45Set ps451, ps452;
    ps451.insert(polyHole);
    ps452 = ps451;
    std::cout << (ps451 == ps452) << std::endl;
    if(ps451 != ps452) return 1;
    ps451.transform(atr);
    std::cout << (ps451 == ps452) << std::endl;
    if(ps451 == ps452) return 1;
    ps451.transform(tr);
    std::cout << (ps451 == ps452) << std::endl;
    if(ps451 != ps452) return 1;

    //test polygon45set area
    std::cout << area(ps451) << std::endl;
    if(area(ps451) != 12.5) return 1;
    //test polygon45set scale up down
    ps451.scale_up(3);
    std::cout << area(ps451) << std::endl;
    if(area(ps451) != 112.5) return 1;
    ps451.scale_down(2);
    std::cout << area(ps451) << std::endl;
    if(area(ps451) != 32) return 1;
    //test polygonset scalue up down
  }
  {
    std::cout << (testPolygon45SetRect()) << std::endl;
    testPolygon45SetPerterbation(); //re-enable after non-intersection fix
    testPolygon45Set();
    testPolygon45SetDORA();  //re-enable after non-intersection fix
    polygon_45_set_data<int> ps45_1, ps45_2, ps45_3;
    ps45_1.insert(rectangle_data<int>(0, 0, 10, 10));
    ps45_2.insert(rectangle_data<int>(5, 5, 15, 15));
    std::vector<polygon_45_data<int> > p45s;
    ps45_3 = ps45_1 | ps45_2;
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    p45s.clear();
    ps45_3 = ps45_1 + ps45_2;
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    p45s.clear();
    ps45_3 = ps45_1 * ps45_2;
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    p45s.clear();
    ps45_3 = ps45_1 - ps45_2;
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    p45s.clear();
    ps45_3 = ps45_1 ^ ps45_2;
    ps45_3.get(p45s);
    if(p45s.size() == 2) std::cout << p45s[0] << " " << p45s[1] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    std::vector<point_data<int> > pts;
    pts.clear();
    pts.push_back(point_data<int>(7, 0));
    pts.push_back(point_data<int>(20, 13));
    pts.push_back(point_data<int>(0, 13));
    pts.push_back(point_data<int>(0, 0));
    polygon_45_data<int> p45_1(pts.begin(), pts.end());
    ps45_3.clear();
    ps45_3.insert(p45_1);
    p45s.clear();
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    ps45_3 += 1;
    p45s.clear();
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
    ps45_3 -= 1;
    p45s.clear();
    ps45_3.get(p45s);
    if(p45s.size()) std::cout << p45s[0] << std::endl;
    else {
      std::cout << "test failed\n";
      return 1;
    }
  }
  {
    polygon_90_set_data<int> p90sd;
    p90sd.insert(rectangle_data<int>(0, 0, 10, 10));
    std::vector<rectangle_data<int> > rects;
    std::vector<polygon_90_data<int> > polys90;
    std::vector<polygon_90_with_holes_data<int> > pwhs90;
    assign(rects, p90sd);
    assign(polys90, p90sd);
    assign(pwhs90, p90sd);
    std::cout << equivalence(rects, polys90) << std::endl;
    std::cout << equivalence(pwhs90, polys90) << std::endl;
    pwhs90.clear();
    assign(pwhs90, polys90);
    std::cout << equivalence(pwhs90, polys90) << std::endl;
  }
  {
    polygon_45_set_data<int> p45sd;
    p45sd.insert(rectangle_data<int>(0, 0, 10, 10));
    std::vector<rectangle_data<int> > rects;
    std::vector<polygon_45_data<int> > polys45;
    std::vector<polygon_45_with_holes_data<int> > pwhs45;
    get_trapezoids(polys45, p45sd);
    assign(polys45, p45sd);
    assign(pwhs45, p45sd);
    std::cout << equivalence(pwhs45, polys45) << std::endl;
    pwhs45.clear();
    assign(pwhs45, polys45);
    std::cout << equivalence(pwhs45, polys45) << std::endl;
  }
  {
    polygon_set_data<int> psd;
    psd.insert(rectangle_data<int>(0, 0, 10, 10));
    std::vector<polygon_data<int> > polys;
    std::vector<polygon_with_holes_data<int> > pwhs;
    assign(polys, psd);
    assign(pwhs, psd);
    std::cout << equivalence(pwhs, polys) << std::endl;
    pwhs.clear();
    assign(pwhs, polys);
    std::cout << equivalence(pwhs, polys) << std::endl;
  }
  {
    polygon_90_set_data<int> ps1(HORIZONTAL), ps2(VERTICAL);
    ps1 += rectangle_data<int>(0, 0, 10, 120);
    assign(ps1, ps2);
    std::cout << equivalence(ps1, ps2) << std::endl;
  }
  {
    std::vector<rectangle_data<polygon_long_long_type> > lobs, input;
    input.push_back(rectangle_data<polygon_long_long_type>(0, 0, 10, 10));
    input.push_back(rectangle_data<polygon_long_long_type>(10, 5, 15, 15));
    get_max_rectangles(lobs, input);
    if(lobs.size() == 3) std::cout << "max rectangles is correct\n";
  }
  {
    polygon_set_data<int> ps1, ps2, ps3;
    ps1.insert(rectangle_data<int>(0, 0, 10, 10));
    ps2.insert(rectangle_data<int>(0, 0, 15, 5));
    ps3.insert(rectangle_data<int>(0, 0, 20, 2));
    std::cout << area(ps1 + ps2) << std::endl;
    keep(ps1, 0, 100, 0, 100, 0, 100);
    if(empty(ps1)) return 1;
    rectangle_data<int> bbox;
    extents(bbox, ps1);
    std::cout << bbox << std::endl;
    //resize(ps1, 1);
    //shrink(ps1, 1);
    //bloat(ps1, 1);
    scale_up(ps1, 2);
    scale_down(ps1, 2);
    axis_transformation atr;
    transform(ps1, atr);
    std::cout << area(ps1) << std::endl;
    if(area(ps1) != 100) return 1;
    clear(ps1);
    if(!empty(ps1)) return 1;
    ps1 = ps2 * ps3;
    ps1 *= ps2;
    ps1 - ps2;
    ps1 -= ps2;
    ps1 ^ ps2;
    ps1 ^= ps2;
    ps1 | ps2;
    ps1 |= ps2;
  }
  {
    polygon_45_set_data<int> ps45_1, ps45_2;
    ps45_1.insert(rectangle_data<int>(0, 0, 10, 10));
    keep(ps45_1, 0, 1000, 0, 1000, 0, 1000);
    std::cout << area(ps45_1) << std::endl;
    std::cout << empty(ps45_1) << std::endl;
    rectangle_data<int> bbox;
    extents(bbox, ps45_1);
    std::cout << bbox << std::endl;
    resize(ps45_1, 1);
    shrink(ps45_1, 1);
    bloat(ps45_1, 1);
    scale_up(ps45_1, 2);
    scale_down(ps45_1, 2);
    axis_transformation atr;
    transform(ps45_1, atr);
    std::cout << area(ps45_1) << std::endl;
    if(area(ps45_1) != 144) return 1;
    clear(ps45_1);
    if(!empty(ps45_1)) return 1;
  }
  {
    std::vector<polygon_45_data<int> > p45v;
    p45v + p45v;
    p45v *= p45v;
    p45v += p45v;
    p45v - p45v;
    p45v -= p45v;
    p45v ^ p45v;
    p45v ^= p45v;
    p45v | p45v;
    p45v |= p45v;
    p45v + 1;
    p45v += 1;
    p45v - 1;
    p45v -= 1;
    p45v + (p45v + p45v);
  }
  {
    polygon_45_set_data<int> ps45;
    polygon_90_set_data<int> ps90;
    std::vector<polygon_90_with_holes_data<int> > p90whv;
    ps45.insert(ps90);
    ps45.insert(p90whv);
    ps45.insert(p90whv + p90whv);

    ps45.insert(polygon_90_with_holes_data<int>());
    polygon_with_holes_data<int> pwh;
    snap_to_45(pwh);
  }
  {
    polygon_90_set_data<int> ps90_1, ps90_2;
    ps90_1.insert(rectangle_data<int>(0, 0, 10, 10));
    keep(ps90_1, 0, 1000, 0, 1000, 0, 1000);
    std::cout << area(ps90_1) << std::endl;
    std::cout << empty(ps90_1) << std::endl;
    rectangle_data<int> bbox;
    extents(bbox, ps90_1);
    std::cout << bbox << std::endl;
    resize(ps90_1, 1);
    shrink(ps90_1, 1);
    bloat(ps90_1, 1);
    scale_up(ps90_1, 2);
    scale_down(ps90_1, 2);
    scale(ps90_1, anisotropic_scale_factor<double>(2, 2));
    scale(ps90_1, anisotropic_scale_factor<double>(0.5, 0.5));
    axis_transformation atr;
    transform(ps90_1, atr);
    std::cout << area(ps90_1) << std::endl;
    if(area(ps90_1) != 144) return 1;
    clear(ps90_1);
    if(!empty(ps90_1)) return 1;
  }
  if(!nonInteger45StessTest()) return 1;
  {
  using namespace gtl;
  typedef polygon_45_property_merge<int, int> p45pm;
  p45pm::MergeSetData msd;
  polygon_45_set_data<int> ps;
  ps += rectangle_data<int>(0, 0, 10, 10);
  p45pm::populateMergeSetData(msd, ps.begin(), ps.end(), 444);
  ps.clear();
  ps += rectangle_data<int>(5, 5, 15, 15);
  p45pm::populateMergeSetData(msd, ps.begin(), ps.end(), 333);
  std::map<std::set<int>, polygon_45_set_data<int> > result;
  p45pm::performMerge(result, msd);
  int i = 0;
  for(std::map<std::set<int>, polygon_45_set_data<int> >::iterator itr = result.begin();
      itr != result.end(); ++itr) {
    for(std::set<int>::const_iterator itr2 = (*itr).first.begin();
        itr2 != (*itr).first.end(); ++itr2) {
      std::cout << *itr2 << " ";
    } std::cout << " : ";
    std::cout << area((*itr).second) << std::endl;
    if(i == 1) {
      if(area((*itr).second) != 100) return 1;
    } else
      if(area((*itr).second) != 300) return 1;
    ++i;
  }


  property_merge_45<int, int> pm;
  pm.insert(rectangle_data<int>(0, 0, 10, 10), 444);
  pm.insert(rectangle_data<int>(5, 5, 15, 15), 333);
  std::map<std::set<int>, polygon_45_set_data<int> > mp;
  pm.merge(mp);
  i = 0;
  for(std::map<std::set<int>, polygon_45_set_data<int> >::iterator itr = mp.begin();
      itr != mp.end(); ++itr) {
    for(std::set<int>::const_iterator itr2 = (*itr).first.begin();
        itr2 != (*itr).first.end(); ++itr2) {
      std::cout << *itr2 << " ";
    } std::cout << " : ";
    std::cout << area((*itr).second) << std::endl;
    if(i == 1) {
      if(area((*itr).second) != 25) return 1;
    } else
      if(area((*itr).second) != 75) return 1;
    ++i;
  }
  std::map<std::vector<int>, polygon_45_set_data<int> > mp2;
  pm.merge(mp2);
  i = 0;
  for(std::map<std::vector<int>, polygon_45_set_data<int> >::iterator itr = mp2.begin();
      itr != mp2.end(); ++itr) {
    for(std::vector<int>::const_iterator itr2 = (*itr).first.begin();
        itr2 != (*itr).first.end(); ++itr2) {
      std::cout << *itr2 << " ";
    } std::cout << " : ";
    std::cout << area((*itr).second) << std::endl;
    if(i == 1) {
      if(area((*itr).second) != 25) return 1;
    } else
      if(area((*itr).second) != 75) return 1;
    ++i;
  }
  }
  {
  std::cout << trapezoid_arbitrary_formation<int>::testTrapezoidArbitraryFormationRect(std::cout) << std::endl;
  std::cout << trapezoid_arbitrary_formation<int>::testTrapezoidArbitraryFormationP1(std::cout) << std::endl;
  std::cout << trapezoid_arbitrary_formation<int>::testTrapezoidArbitraryFormationP2(std::cout) << std::endl;
  std::cout << trapezoid_arbitrary_formation<int>::testTrapezoidArbitraryFormationPolys(std::cout) << std::endl;
  std::cout << polygon_arbitrary_formation<int>::testPolygonArbitraryFormationSelfTouch1(std::cout) << std::endl;
  std::cout << trapezoid_arbitrary_formation<int>::testTrapezoidArbitraryFormationSelfTouch1(std::cout) << std::endl;
  typedef rectangle_data<int> Rectangle;
  polygon_set_data<int> ps;
  ps += Rectangle(0, 1, 10, 11);
  ps += Rectangle(5, 6, 15, 16);
  std::vector<polygon_data<int> > polys;
  ps.get_trapezoids(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  ps.transform(axis_transformation(axis_transformation::FLIP_X));
  polys.clear();
  ps.get_trapezoids(polys);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  polys.clear();
  ps.get_trapezoids(polys, HORIZONTAL);
  for(unsigned int i = 0; i < polys.size(); ++i) {
    std::cout << polys[i] << std::endl;
  }
  }

  if(!test_aa_touch()) {
    std::cout << "test_aa_touch failed\n";
    return 1;
  }
  if(!test_aa_touch_ur()) {
    std::cout << "test_aa_touch_ur failed\n";
    return 1;
  }
  if(!test_aa_touch_ur()) {
    std::cout << "test_aa_touch_ur failed\n";
    return 1;
  }
  if(!test_aa_touch_r()) {
    std::cout << "test_aa_touch_r failed\n";
    return 1;
  }
  if(!test_aa_touch_boundaries()) {
    std::cout << "test_aa_touch_boundaries failed\n";
    return 1;
  }
  if(!test_aa_concept_interact()) {
    std::cout << "test_aa_concept_interact failed\n";
    return 1;
  }

  {
    polygon_set_data<int> ps;
    polygon_90_set_data<int> ps90;
    rectangle_data<int> rect(0, 1, 10, 100);
    std::vector<polygon_data<int> > rupolys, rupolys45;
    ps.insert(rect);
    ps90.insert(rect);
    ps.bloat(10);
    ps90.bloat(10, 10, 10, 10);
    rupolys.clear();
    rupolys45.clear();
    ps.get(rupolys);
    ps90.get(rupolys45);
    std::cout << rupolys[0] << std::endl;
    std::cout << rupolys45[0] << std::endl;
    if(!equivalence(ps, ps90)) {
      std::cout << "test manhattan vs general resize up failed\n";
      return 1;
    }
    ps.shrink(10);
    ps90.shrink(10, 10, 10, 10);
    if(!equivalence(ps, rect)) {
      std::cout << "test manhattan vs general resize down failed\n";
      return 1;
    }
    rectangle_data<int> rect2(3, 4, 6, 80);
    ps -= rect2;
    ps90 -= rect2;
    ps.bloat(1);
    ps90.bloat(1, 1, 1, 1);
    if(!equivalence(ps, ps90)) {
      std::cout << "test manhattan vs general with hole resize up failed\n";
      return 1;
    }
    ps.shrink(1);
    ps90.shrink(1, 1, 1, 1);
    if(!equivalence(ps, ps90)) {
      std::cout << "test manhattan vs general with hole resize down failed\n";
      return 1;
    }
    ps.clear();
    polygon_45_data<int> poly;
    std::vector<point_data<int> > pts;
    pts.push_back(point_data<int>(0, 0));
    pts.push_back(point_data<int>(10, 0));
    pts.push_back(point_data<int>(0, 10));
    polygon_45_set_data<int> ps45;
    set_points(poly, pts.begin(), pts.end());
    ps.insert(poly);
    ps45.insert(poly);
    ps.bloat(9);
    ps45.resize(9);
    rupolys.clear();
    rupolys45.clear();
    ps.get(rupolys);
    ps45.get(rupolys45);
    std::cout << rupolys[0] << std::endl;
    std::cout << rupolys45[0] << std::endl;
    pts.clear();
    pts.push_back(point_data<int>(32, -9));
    pts.push_back(point_data<int>(-9, 32));
    pts.push_back(point_data<int>(-9, -9));
    set_points(poly, pts.begin(), pts.end());
    if(!equivalence(ps, poly)) {
      std::cout << "test general resize up failed\n";
      return 1;
    }
    // this test is waived due to rounding differences between 45 and general resizing
    // general resizing is computing floating point coordinates for the intersection
    // and rounding those to closest while 45 is computing the normal point and rounding
    // that to closest, it turns out to result in different intersection point
    // we want the general to be more accurate to avoid artifacts
    //if(!equivalence(ps, ps45)) {
    //  std::cout << "test 45 vs general resize up failed\n";
    //  return 1;
    //}
    ps.shrink(9);
    ps45.resize(-9);
    if(!equivalence(ps, ps45)) {
      std::cout << "test 45 vs general resize down failed\n";
      return 1;
    }
    pts.clear();
    pts.push_back(point_data<int>(1, 1));
    pts.push_back(point_data<int>(7, 1));
    pts.push_back(point_data<int>(1, 7));
    set_points(poly, pts.begin(), pts.end());
    ps.insert(poly, true);
    ps45.insert(poly, true);
    ps.bloat(1);
    ps45.resize(1);
    rupolys.clear();
    rupolys45.clear();
    ps.get(rupolys);
    ps45.get(rupolys45);
    std::cout << rupolys[0] << std::endl;
    std::cout << rupolys45[0] << std::endl;
    pts.clear();
    pts.push_back(point_data<int>(12, -1));
    pts.push_back(point_data<int>(5, 6));
    pts.push_back(point_data<int>(5, 2));
    pts.push_back(point_data<int>(2, 2));
    pts.push_back(point_data<int>(2, 5));
    pts.push_back(point_data<int>(5, 2));
    pts.push_back(point_data<int>(5, 6));
    pts.push_back(point_data<int>(-1, 12));
    pts.push_back(point_data<int>(-1, -1));
    pts.push_back(point_data<int>(12, -1));
    set_points(poly, pts.begin(), pts.end());
    //waived
    //if(!equivalence(ps, poly)) {
    //  std::cout << "test general resize up with holes failed\n";
    //  return 1;
    //}
    //waived
    //if(!equivalence(ps, ps45)) {
    //  std::cout << "test 45 vs general resize up with holes failed\n";
    //  return 1;
    //}
    ps.shrink(1);
    ps45.resize(-1);
    if(!equivalence(ps, ps45)) {
      std::cout << "test 45 vs general resize down with holes failed\n";
      return 1;
    }
    ps.shrink(10);
    ps45.resize(-10);
    if(!equivalence(ps, ps45)) {
      std::cout << "test 45 vs general resize down 2 with holes failed\n";
      return 1;
    }
  }

  {

    Point pts[] = {construct<Point>(1565, 5735),
                   construct<Point>(915, 5735),
                   construct<Point>(915, 7085),
                   construct<Point>(1565, 7085) };
    Polygon poly;
    set_points(poly, pts, pts+4);
    bool ret=gtl::contains(poly,gtl::construct<Point>(920, 7080));
    if(!ret) {
      std::cout << "contains failed!" << std::endl;
      return 1;
    }
    polygon_data<int> poly_aa;
    set_points(poly_aa, pts, pts+4);
    ret=gtl::contains(poly,gtl::construct<Point>(920, 7080));
    if(!ret) {
      std::cout << "contains 90 failed!" << std::endl;
      return 1;
    }
    polygon_with_holes_data<int> pwh;
    polygon_90_with_holes_data<int> p90wh;
    Point pts2[] = {construct<Point>(565, 15735),
                   construct<Point>(15, 15735),
                   construct<Point>(15, 17085),
                   construct<Point>(565, 17085) };
    set_points(pwh, pts2, pts2+4);
    set_points(p90wh, pts2, pts2+4);
    pwh.set_holes(&poly_aa, (&poly_aa)+1);
    p90wh.set_holes(&poly, (&poly)+1);
    ret=gtl::contains(pwh,gtl::construct<Point>(920, 7080));
    if(ret) {
      std::cout << "contains wh failed!" << std::endl;
      return 1;
    }
    ret=gtl::contains(p90wh,gtl::construct<Point>(920, 7080));
    if(ret) {
      std::cout << "contains 90wh failed!" << std::endl;
      return 1;
    }
    std::reverse(pts, pts+4);
    set_points(poly, pts, pts+4);
    ret=gtl::contains(poly,gtl::construct<Point>(920, 7080));
    if(!ret) {
      std::cout << "reverse contains failed!" << std::endl;
      return 1;
    }
  }
  {
//     //MULTIPOLYGON
//     (
//      ((200 400,100 400,100 300,200 400)),
//      ((300 100,200 100,200 0,300 0,300 100)),
//      ((600 700,500 700,500 600,600 700)),
//      ((700 300,600 300,600 200,700 300)),
//      ((800 500,700 600,700 500,800 500)),
//      ((900 800,800 700,900 700,900 800)),
//      ((1000 200,900 100,1000 100,1000 200)),
//      ((1000 800,900 900,900 800,1000 800))),
    int mp1 [7][2*4] = {
      {200,400,100,400,100,300,200,400},
      {600,700,500,700,500,600,600,700},
      {700,300,600,300,600,200,700,300},
      {800,500,700,600,700,500,800,500},
      {900,800,800,700,900,700,900,800},
      {1000,200,900,100,1000,100,1000,200},
      {1000,800,900,900,900,800,1000,800}
    };
    int mp11 [2*5] = {300,100,200,100,200,0,300,0,300,100};
    polygon_45_set_data<int> pset1;
    polygon_45_set_data<int> pset2;
    for(int i = 0; i < 7; ++i) {
      addpoly(pset1, mp1[i], 4);
    }
    addpoly(pset1, mp11, 5);
//     //MULTIPOLYGON
//       (
//        ((200 800,100 800,100 700,200 700,200 800)),
//        ((400 200,300 100,400 100,400 200)),
//        ((400 800,300 700,400 700,400 800)),
//        ((700 100,600 0,700 0,700 100)),
//        ((700 200,600 200,600 100,700 200)),
//        ((900 200,800 200,800 0,900 0,900 200)),
//        ((1000 300,900 200,1000 200,1000 300)))
    int mp2 [5][2*4] = {
      {400,200,300,100,400,100,400,200},
      {400,800,300,700,400,700,400,800},
      {700,100,600,0,700,0,700,100},
      {700,200,600,200,600,100,700,200},
      {1000,300,900,200,1000,200,1000,300},
    };
    int mp21 [2*5] = {200,800,100,800,100,700,200,700,200,800};
    int mp22 [2*5] = {900,200,800,200,800,0,900,0,900,200};
    for(int i = 0; i < 5; ++i) {
      addpoly(pset2, mp2[i], 4);
    }
    addpoly(pset2, mp21, 5);
    addpoly(pset2, mp22, 5);
    polygon_45_set_data<int> orr = pset1 + pset2;
    polygon_45_set_data<int> inr = pset1 & pset2;
    std::cout << area(orr)<<std::endl;;
    std::cout << area(inr)<<std::endl;;
    std::vector<polygon_45_with_holes_data<int> > polys;
    assign(polys, orr);
    std::cout << area(polys) << std::endl;
    polygon_set_data<int> testbug;
    testbug.insert(orr);
    std::cout << area(testbug) << std::endl;
    polygon_set_data<int> testbug2;
    for(size_t i = 0; i < polys.size(); ++i) {
      for(size_t j = 0; j < polys.size(); ++j) {
        testbug2.clear();
        testbug2.insert(polys[i]);
        testbug2.insert(polys[j]);
        std::cout << i << " " << j << std::endl;
        std::cout << polys[i] << std::endl;
        std::cout << polys[j] << std::endl;
        if(area(testbug2) == 0.0) {
          std::cout << area(testbug2) << std::endl;
          std::cout << "Self touch 45 through general interface failed!\n";
          return 1;
        }
      }
    }
  }

  {
    polygon_set_data<int> t_eq;
    t_eq.insert(rectangle_data<int>(0, 0, 5, 10));
    t_eq.insert(rectangle_data<int>(0, 5, 5, 10));
    std::cout << t_eq <<std::endl;
    polygon_set_data<int> t_eq2;
    t_eq2 += rectangle_data<int>(0, 0, 5, 10);
    std::cout << area(t_eq) <<std::endl;
    std::cout << area(t_eq2) <<std::endl;   
    std::cout << t_eq <<std::endl;
    std::cout << t_eq2 <<std::endl;   
    if(t_eq != t_eq2) {
      std::cout << "equivalence failed" << std::endl;
      return 1;
    }    
  }

  {
    using namespace boost::polygon;
    typedef point_data<int> Point;
    typedef segment_data<int> Dls;
    Point pt1(0, 0);
    Point pt2(10, 10);
    Point pt3(20, 20);
    Point pt4(20, 0);
    Dls dls1(pt1, pt2);
    Dls dls2(pt1, pt3);
    Dls dls3(pt1, pt4);
    Dls dls4(pt2, pt1);
    typedef std::vector<segment_data<int> > Dlss;
    Dlss dlss, result;
    dlss.push_back(dls1);
    dlss.push_back(dls2);
    dlss.push_back(dls3);
    dlss.push_back(dls4);
    rectangle_data<int> rect;
    envelope_segments(rect, dlss.begin(), dlss.end());
    assert_s(area(rect) == 400.0, "envelope");
    intersect_segments(result, dlss.begin(), dlss.end());
    dlss.swap(result);
    for (Dlss::iterator itr = dlss.begin(); itr != dlss.end(); ++itr) {
      std::cout << *itr << std::endl;
    }
    assert_s(dlss.size() == 5, "intersection");
    Dls dls5(Point(0,5), Point(5,0));
    dlss.push_back(dls5);
    std::cout << std::endl;
    result.clear();
    intersect_segments(result, dlss.begin(), dlss.end());
    dlss.swap(result);
    for (Dlss::iterator itr = dlss.begin(); itr != dlss.end(); ++itr) {
      std::cout << *itr << std::endl;
    }
    assert_s(dlss.size() == 11, "intersection2");
  }
  
  {
    using namespace boost::polygon;
    std::vector<std::pair<std::size_t, segment_data<int> > > segs;
    segment_data<int> sarray[2];
    sarray[0] = segment_data<int>(point_data<int>(0,0), point_data<int>(10,10));
    sarray[1] = segment_data<int>(point_data<int>(10,0), point_data<int>(0,10));
    intersect_segments(segs, sarray, sarray+2);
    std::cout << segs.size() << std::endl;
    assert_s(segs.size() == 4, "intersection3");
  }


  /*New polygon_formation tests*/ 
  if(test_new_polygon_formation(0,NULL)){
     std::cerr << "[test_new_polygon_formation] failed" << std::endl;
     return 1;
  }

  if(test_new_polygon_formation_marginal_threshold(0,NULL)){
     std::cerr << "[test_new_polygon_formation_marginal_threshold] failed" 
         << std::endl;
     return 1;
  }

  std::cout << "ALL TESTS COMPLETE\n";
  return 0;
}

// Boost.Polygon library voronoi_benchmark.cpp file

//          Copyright Andrii Sydorchuk 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for updates, documentation, and revision history.

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <numeric>
#include <vector>
#include <utility>

#include <boost/random/mersenne_twister.hpp>
#include <boost/timer/timer.hpp>

typedef boost::int32_t int32;
using boost::timer::cpu_times;
using boost::timer::nanosecond_type;

// Include for the Boost.Polygon Voronoi library.
#include <boost/polygon/voronoi.hpp>
typedef boost::polygon::default_voronoi_builder VB_BOOST;
typedef boost::polygon::voronoi_diagram<double> VD_BOOST;

// Includes for the CGAL library.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
typedef CGAL::Exact_predicates_inexact_constructions_kernel CGAL_KERNEL;
typedef CGAL::Delaunay_triangulation_2<CGAL_KERNEL> DT_CGAL;
typedef CGAL_KERNEL::Point_2 POINT_CGAL;

// Includes for the S-Hull library.
#include <s_hull.h>

const int RANDOM_SEED = 27;
const int NUM_TESTS = 6;
const int NUM_POINTS[] = {10, 100, 1000, 10000, 100000, 1000000};
const int NUM_RUNS[] = {100000, 10000, 1000, 100, 10, 1};
std::ofstream bf("benchmark_points.txt",
                 std::ios_base::out | std::ios_base::app);
boost::timer::cpu_timer timer;

void format_line(int num_points, int num_tests, double time_per_test) {
  bf << "| " << std::setw(16) << num_points << " ";
  bf << "| " << std::setw(15) << num_tests << " ";
  bf << "| " << std::setw(17) << time_per_test << " ";
  bf << "|" << std::endl;
}

double get_elapsed_secs() {
  cpu_times elapsed_times(timer.elapsed());
  return 1E-9 * static_cast<double>(
      elapsed_times.system + elapsed_times.user);
}

void run_boost_voronoi_test() {
  boost::mt19937 gen(RANDOM_SEED);
  bf << "Boost.Polygon Voronoi Diagram of Points:\n";
  for (int i = 0; i < NUM_TESTS; ++i) {
    timer.start();
    for (int j = 0; j < NUM_RUNS[i]; ++j) {
      VB_BOOST vb;
      VD_BOOST vd;
      for (int k = 0; k < NUM_POINTS[i]; ++k) {
        int32 x = static_cast<int32>(gen());
        int32 y = static_cast<int32>(gen());
        vb.insert_point(x, y);
      }
      vb.construct(&vd);
    }
    double time_per_test = get_elapsed_secs() / NUM_RUNS[i];
    format_line(NUM_POINTS[i], NUM_RUNS[i], time_per_test);
  }
  bf << "\n";
}

void run_cgal_delaunay_test() {
  boost::mt19937 gen(RANDOM_SEED);
  bf << "CGAL Delaunay Triangulation of Points:\n";
  for (int i = 0; i < NUM_TESTS; ++i) {
    timer.start();
    for (int j = 0; j < NUM_RUNS[i]; ++j) {
      DT_CGAL dt;
      std::vector<POINT_CGAL> points;
      for (int k = 0; k < NUM_POINTS[i]; ++k) {
        int32 x = static_cast<int32>(gen());
        int32 y = static_cast<int32>(gen());
        points.push_back(POINT_CGAL(x, y));
      }
      // CGAL's implementation sorts points along
      // the Hilbert curve implicitly to improve
      // spatial ordering of the input geometries.
      dt.insert(points.begin(), points.end());
    }
    double time_per_test = get_elapsed_secs() / NUM_RUNS[i];
    format_line(NUM_POINTS[i], NUM_RUNS[i], time_per_test);
  }
  bf << "\n";
}

void run_shull_delaunay_test() {
  boost::mt19937 gen(RANDOM_SEED);
  bf << "S-Hull Delaunay Triangulation of Points:\n";
  // This value is required by S-Hull as it doesn't seem to support properly
  // coordinates with the absolute value higher than 100.
  float koef = 100.0 / (1 << 16) / (1 << 15);
  for (int i = 0; i < NUM_TESTS; ++i) {
    timer.start();
    for (int j = 0; j < NUM_RUNS[i]; ++j) {
      // S-Hull doesn't deal properly with duplicates so we need
      // to eliminate them before running the algorithm.
      std::vector< pair<int32, int32> > upts;
      std::vector<Shx> pts;
      std::vector<Triad> triads;
      Shx pt;
      for (int k = 0; k < NUM_POINTS[i]; ++k) {
        int32 x = static_cast<int32>(gen());
        int32 y = static_cast<int32>(gen());
        upts.push_back(std::make_pair(x, y));
      }
      // Absolutely the same code is used by the Boost.Polygon Voronoi library.
      std::sort(upts.begin(), upts.end());
      upts.erase(std::unique(upts.begin(), upts.end()), upts.end());
      for (int k = 0; k < upts.size(); ++k) {
        pt.r = koef * upts[k].first;
        pt.c = koef * upts[k].second;
        pt.id = k;
        pts.push_back(pt);
      }
      s_hull_del_ray2(pts, triads);
    }
    double time_per_test = get_elapsed_secs() / NUM_RUNS[i];
    format_line(NUM_POINTS[i], NUM_RUNS[i], time_per_test);
  }
  bf << "\n";
}

int main() {
  bf << std::setiosflags(std::ios::right | std::ios::fixed)
     << std::setprecision(6);
  run_boost_voronoi_test();
  run_cgal_delaunay_test();
  run_shull_delaunay_test();
  bf.close();
  return 0;
}

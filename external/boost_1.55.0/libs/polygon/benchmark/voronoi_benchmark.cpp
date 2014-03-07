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
#include <list>
#include <numeric>
#include <vector>

#define BOOST_TEST_MODULE benchmark_test
#include <boost/mpl/list.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/timer.hpp>

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
using boost::polygon::point_data;
using boost::polygon::segment_data;
using boost::polygon::voronoi_diagram;

typedef boost::mpl::list<int> test_types;
const char *BENCHMARK_FILE = "voronoi_benchmark.txt";
const char *POINT_INPUT_FILE = "input_data/voronoi_point.txt";
const char *SEGMENT_INPUT_FILE = "input_data/voronoi_segment.txt";
const int POINT_RUNS = 10;
const int SEGMENT_RUNS = 10;

boost::mt19937 gen(static_cast<unsigned int>(time(NULL)));
boost::timer timer;

BOOST_AUTO_TEST_CASE_TEMPLATE(benchmark_test_random, T, test_types) {
    typedef T coordinate_type;
    typedef point_data<coordinate_type> point_type;
    voronoi_diagram<double> test_output;

    std::ofstream bench_file(BENCHMARK_FILE, std::ios_base::out | std::ios_base::app);
    bench_file << "Voronoi Benchmark Test (time in seconds):" << std::endl;
    bench_file << "| Number of points | Number of tests | Time per one test |" << std::endl;
    bench_file << std::setiosflags(std::ios::right | std::ios::fixed) << std::setprecision(6);
    int max_points = 100000;
    std::vector<point_type> points;
    coordinate_type x, y;
    for (int num_points = 10; num_points <= max_points; num_points *= 10) {
        points.resize(num_points);
        timer.restart();
        int num_tests = max_points / num_points;
        for (int cur = 0; cur < num_tests; cur++) {
            test_output.clear();
            for (int cur_point = 0; cur_point < num_points; cur_point++) {
                x = static_cast<coordinate_type>(gen());
                y = static_cast<coordinate_type>(gen());
                points[cur_point] = point_type(x, y);
            }
            construct_voronoi(points.begin(), points.end(), &test_output);
        }
        double elapsed_time = timer.elapsed();
        double time_per_test = elapsed_time / num_tests;

        bench_file << "| " << std::setw(16) << num_points << " ";
        bench_file << "| " << std::setw(15) << num_tests << " ";
        bench_file << "| " << std::setw(17) << time_per_test << " ";
        bench_file << "| " << std::endl;
    }
    bench_file.close();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(benchmark_test_points, T, test_types) {
    typedef T coordinate_type;
    typedef point_data<coordinate_type> point_type;

    std::vector<point_type> points;
    {
        std::ifstream input_file(POINT_INPUT_FILE);
        int num_points;
        coordinate_type x, y;
        input_file >> num_points;
        points.reserve(num_points);
        for (int i = 0; i < num_points; ++i) {
            input_file >> x >> y;
            points.push_back(point_type(x, y));
        }
        input_file.close();
    }

    std::vector<double> periods;
    {
        for (int i = 0; i < POINT_RUNS; ++i) {
            voronoi_diagram<double> test_output;
            timer.restart();
            construct_voronoi(points.begin(), points.end(), &test_output);
            periods.push_back(timer.elapsed());
        }
    }
    std::sort(periods.begin(), periods.end());
    // Using olympic system to evaluate average time.
    double elapsed_time =
        std::accumulate(periods.begin() + 2, periods.end() - 2, 0.0);
    elapsed_time /= (periods.size() - 4);

    std::ofstream bench_file(
        BENCHMARK_FILE, std::ios_base::out | std::ios_base::app);
    bench_file << std::setiosflags(std::ios::right | std::ios::fixed) << std::setprecision(6);
    bench_file << "Static test of " << points.size() << " points: " << elapsed_time << std::endl;
    bench_file.close();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(benchmark_test_segments, T, test_types) {
    typedef T coordinate_type;
    typedef point_data<coordinate_type> point_type;
    typedef segment_data<coordinate_type> segment_type;

    std::vector<segment_type> segments;
    {
        std::ifstream input_file(SEGMENT_INPUT_FILE);
        int num_segments;
        coordinate_type x0, y0, x1, y1;
        input_file >> num_segments;
        segments.reserve(num_segments);
        for (int i = 0; i < num_segments; ++i) {
            input_file >> x0 >> y0 >> x1 >> y1;
            point_type p0(x0, y0), p1(x1, y1);
            segments.push_back(segment_type(p0, p1));
        }
        input_file.close();
    }

    std::vector<double> periods;
    {
        for (int i = 0; i < SEGMENT_RUNS; ++i) {
            voronoi_diagram<double> test_output;
            timer.restart();
            construct_voronoi(segments.begin(), segments.end(), &test_output);
            periods.push_back(timer.elapsed());
        }
    }
    std::sort(periods.begin(), periods.end());
    // Using olympic system to evaluate average time.
    double elapsed_time =
        std::accumulate(periods.begin() + 2, periods.end() - 2, 0.0);
    elapsed_time /= (periods.size() - 4);

    std::ofstream bench_file(
        BENCHMARK_FILE, std::ios_base::out | std::ios_base::app);
    bench_file << std::setiosflags(std::ios::right | std::ios::fixed) << std::setprecision(6);
    bench_file << "Static test of " << segments.size() << " segments: " << elapsed_time << std::endl;
    bench_file.close();
}

//
//  Copyright (c) 2010 Athanasios Iliopoulos
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/timer.hpp>

using namespace boost::numeric::ublas;

int main() {

    boost::timer timer;

    unsigned int iterations = 1000000000;
    double elapsed_exp, elapsed_assigner;

    std::cout << "Ublas vector<double> Benchmarks------------------------ " << "\n";

    {
    std::cout << "Size 2 vector: " << "\n";
    vector<double> a(2);

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++) {
        a(0)=0; a(1)=1;
    }
    elapsed_exp = timer.elapsed();
    std::cout << "Explicit element assign time: " << elapsed_exp << " secs" << "\n";

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++)
        a <<= 0, 1;
    elapsed_assigner = timer.elapsed();
    std::cout << "Assigner time: " << elapsed_assigner << " secs" << "\n";
    std::cout << "Difference: " << (elapsed_assigner/elapsed_exp-1)*100 << "%" << std::endl;
    }

    {
    std::cout << "Size 3 vector: " << "\n";
    vector<double> a(3);

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++) {
        a(0)=0; a(1)=1; a(2)=2;
    }
    elapsed_exp = timer.elapsed();
    std::cout << "Explicit element assign time: " << elapsed_exp << " secs" << "\n";

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++)
        a <<= 0, 1, 2;
    elapsed_assigner = timer.elapsed();
    std::cout << "Assigner time: " << elapsed_assigner << " secs" << "\n";
    std::cout << "Difference: " << (elapsed_assigner/elapsed_exp-1)*100 << "%" << std::endl;
    }

    iterations = 100000000;

    {
    std::cout << "Size 8 vector: " << "\n";
    vector<double> a(8);

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++) {
        a(0)=0; a(1)=1; a(2)=2; a(3)=3; a(4)=4; a(5)=5; a(6)=6; a(7)=7;
    }
    elapsed_exp = timer.elapsed();
    std::cout << "Explicit element assign time: " << elapsed_exp << " secs" << "\n";

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++)
        a <<= 0, 1, 2, 3, 4, 5, 6, 7;
    elapsed_assigner = timer.elapsed();
    std::cout << "Assigner time: " << elapsed_assigner << " secs" << "\n";
    std::cout << "Difference: " << (elapsed_assigner/elapsed_exp-1)*100 << "%" << std::endl;
    }


    std::cout << "Ublas matrix<double> Benchmarks------------------------ " << "\n";

    iterations = 200000000;
    {
    std::cout << "Size 3x3 matrix: " << "\n";
    matrix<double> a(3,3);

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++) {
        a(0,0)=0; a(0,1)=1; a(0,2)=2;
        a(1,0)=3; a(1,1)=4; a(1,2)=5;
        a(2,0)=6; a(2,1)=7; a(2,2)=8;
    }
    elapsed_exp = timer.elapsed();
    std::cout << "Explicit element assign time: " << elapsed_exp << " secs" << "\n";

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++)
        a <<= 0, 1, 2, 3, 4, 5, 6, 7, 8;
    elapsed_assigner = timer.elapsed();
    std::cout << "Assigner time: " << elapsed_assigner << " secs" << "\n";
    std::cout << "Difference: " << (elapsed_assigner/elapsed_exp-1)*100 << "%" << std::endl;
    }

    std::cout << "Size 2x2 matrix: " << "\n";
    iterations = 500000000;
    {
    matrix<double> a(2,2);

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++) {
        a(0,0)=0; a(0,1)=1;
        a(1,0)=3; a(1,1)=4;
    }
    elapsed_exp = timer.elapsed();
    std::cout << "Explicit element assign time: " << elapsed_exp << " secs" << "\n";

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++)
        a <<= 0, 1, 3, 4;
    elapsed_assigner = timer.elapsed();
    std::cout << "Assigner time: " << elapsed_assigner << " secs" << "\n";

    std::cout << "Difference: " << (elapsed_assigner/elapsed_exp-1)*100 << "%" << std::endl;

    timer.restart();
    for(unsigned int i=0; i!=iterations; i++)
        a <<= traverse_policy::by_row_no_wrap(), 0, 1, next_row(), 3, 4;
    elapsed_assigner = timer.elapsed();
    std::cout << "Assigner time no_wrap: " << elapsed_assigner << " secs" << "\n";
    std::cout << "Difference: " << (elapsed_assigner/elapsed_exp-1)*100 << "%" << std::endl;
    }

    return 0;
}


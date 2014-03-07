//
//  Copyright (c) 2010 Athanasios Iliopoulos
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector_sparse.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>

using namespace boost::numeric::ublas;

int main() {
        // Simple vector fill
    vector<double> a(3);
    a <<= 0, 1, 2;
    std::cout << a << std::endl;
    // [ 0 1 2]

    // Vector from vector
    vector<double> b(7);
    b <<= a, 10, a;
    std::cout << b << std::endl;
    // [ 0 1 2 10 0 1 2]

    // Simple matrix fill
    matrix<double> A(3,3);
    A <<= 0, 1, 2,
         3, 4, 5,
         6, 7, 8;
    std::cout << A << std::endl;
    // [ 0 1 2 ]
    // [ 3 4 5 ]
    // [ 6 7 8 ]

    // Matrix from vector
    A <<= 0, 1, 2,
         3, 4, 5,
         a;
    std::cout << A << std::endl;
    // [ 0 1 2 ]
    // [ 3 4 5 ]
    // [ 0 1 2 ]

    // Matrix from vector - column assignment
    A <<= move(0,2), traverse_policy::by_column(),
         a;
    std::cout << A << std::endl;
    // [ 0 1 0 ]
    // [ 3 4 1 ]
    // [ 0 1 2 ]

    // Another matrix from vector example (watch the wraping);
    vector<double> c(9); c <<= 1, 2, 3, 4, 5, 6, 7, 8, 9;
    A <<= c;
    std::cout << A << std::endl;
    // [ 1 2 3 ]
    // [ 4 5 6 ]
    // [ 7 8 9 ]

    // If for performance(Benchmarks are not definite about that) or consistency reasons you need to disable wraping:
    static next_row_manip endr; //This can be defined globally
    A <<= traverse_policy::by_row_no_wrap(),
            1, 2, 3, endr,
            4, 5, 6, endr,
            7, 8, 9, endr;
    // [ 1 2 3 ]
    // [ 4 5 6 ]
    // [ 7 8 9 ]
    // If by default you need to disable wraping define
    // BOOST_UBLAS_DEFAULT_NO_WRAP_POLICY, in the compilation options,
    // so that you avoid typing the "traverse_policy::by_row_no_wrap()".

    //  Plus and minus assign:
    A <<= fill_policy::index_plus_assign(),
         3,2,1;
    std::cout << A << std::endl;
    // [ 4 4 4 ]
    // [ 4 5 6 ]
    // [ 7 8 9 ]

    // Matrix from proxy
    A <<= 0, 1, 2,
         project(b, range(3,6)),
         a;
    std::cout << A << std::endl;
    // [ 0 1 2 ]
    // [10 0 1 ]
    // [ 6 7 8 ]

    // Matrix from matrix
    matrix<double> B(6,6);
    B <<= A, A,
         A, A;
    std::cout << B << std::endl;
    // [ A A ]
    // [ A A ]

    // Matrix range (vector is similar)
    B = zero_matrix<double>(6,6);
    matrix_range<matrix<double> > mrB (B, range (1, 4), range (1, 4));
    mrB <<= 1,2,3,4,5,6,7,8,9;
    std::cout << B << std::endl;
    // [ 0 0 0 0 0 0]
    // [ 0 1 2 3 0 0]
    // [ 0 4 5 6 0 0]
    // [ 0 0 0 0 0 0]
    // [ 0 0 0 0 0 0]
    // [ 0 0 0 0 0 0]

    // Horizontal concatenation can be achieved using this trick:
    matrix<double> BH(3,9);
    BH <<= A, A, A;
    std::cout << BH << std::endl;
    // [ A A A]

    // Vertical concatenation can be achieved using this trick:
    matrix<double> BV(9,3);
    BV <<= A,
          A,
          A;
    std::cout << BV << std::endl;
    // [ A ]
    // [ A ]
    // [ A ]

    // Watch the difference when assigning matrices for different traverse policies:
    matrix<double> BR(9,9, 0);
    BR <<= traverse_policy::by_row(), // This is the default, so this might as well be omitted.
          A, A, A;
    std::cout << BR << std::endl;
    // [ A A A]
    // [ 0 0 0]
    // [ 0 0 0]

    matrix<double> BC(9,9, 0);
    BC <<= traverse_policy::by_column(),
          A, A, A;
    std::cout << BC << std::endl;
    // [ A 0 0]
    // [ A 0 0]
    // [ A 0 0]

    // The following will throw a run-time exception in debug mode (matrix mid-assignment wrap is not allowed) :
    // matrix<double> C(7,7);
    // C <<= A, A, A;

    // Matrix from matrix with index manipulators
    matrix<double> C(6,6,0);
    C <<= A, move(3,0), A;
    // [ A 0 ]
    // [ 0 A ]

    // A faster way for to construct this dense matrix.
    matrix<double> D(6,6);
    D <<= A, zero_matrix<double>(3,3),
         zero_matrix<double>(3,3), A;
    // [ A 0 ]
    // [ 0 A ]

    // The next_row and next_column index manipulators:
    // note: next_row and next_column functions return
    // a next_row_manip and and next_column_manip object.
    // This is the manipulator we used earlier when we disabled
    // wrapping.
    matrix<double> E(2,4,0);
    E <<= 1, 2, next_row(),
         3, 4, next_column(),5;
    std::cout << E << std::endl;
    // [ 1 2 0 5 ]
    // [ 3 4 0 0 ]

    // The begin1 (moves to the begining of the column) index manipulator, begin2 does the same for the row:
    matrix<double> F(2,4,0);
    F <<= 1, 2, next_row(),
         3, 4, begin1(),5;
    std::cout << F << std::endl;
    // [ 1 2 5 0 ]
    // [ 3 4 0 0 ]

    // The move (relative) and move_to(absolute) index manipulators (probably the most useful manipulators):
    matrix<double> G(2,4,0);
    G <<= 1, 2, move(0,1), 3,
         move_to(1,3), 4;
    std::cout << G << std::endl;
    // [ 1 2 0 3 ]
    // [ 0 0 0 4 ]

    // Static equivallents (faster) when sizes are known at compile time:
    matrix<double> Gs(2,4,0);
    Gs <<= 1, 2, move<0,1>(), 3,
         move_to<1,3>(), 4;
    std::cout << Gs << std::endl;
    // [ 1 2 0 3 ]
    // [ 0 0 0 4 ]

    // Choice of traverse policy (default is "row by row" traverse):

    matrix<double> H(2,4,0);
    H <<= 1, 2, 3, 4,
         5, 6, 7, 8;
    std::cout << H << std::endl;
    // [ 1 2 3 4 ]
    // [ 5 6 7 8 ]

    H <<= traverse_policy::by_column(),
        1, 2, 3, 4,
        5, 6, 7, 8;
    std::cout << H << std::endl;
    // [ 1 3 5 7 ]
    // [ 2 4 6 8 ]

    // traverse policy can be changed mid assignment if desired.
     matrix<double> H1(4,4,0);
     H1 <<= 1, 2, 3, traverse_policy::by_column(), 1, 2, 3;

    std::cout << H << std::endl;
    // [1 2 3 1]
    // [0 0 0 2]
    // [0 0 0 3]
    // [0 0 0 0]

    // note: fill_policy and traverse_policy are namespaces, so you can use them
    // by a using statement.

    // For compressed and coordinate matrix types a push_back or insert fill policy can be chosen for faster assginment:
    compressed_matrix<double> I(2, 2);
    I <<=    fill_policy::sparse_push_back(),
            0, 1, 2, 3;
    std::cout << I << std::endl;
    // [ 0 1 ]
    // [ 2 3 ]

    coordinate_matrix<double> J(2,2);
    J<<=fill_policy::sparse_insert(),
        1, 2, 3, 4;
    std::cout << J << std::endl;
    // [ 1 2 ]
    // [ 3 4 ]

    // A sparse matrix from another matrix works as with other types.
    coordinate_matrix<double> K(3,3);
    K<<=fill_policy::sparse_insert(),
        J;
    std::cout << K << std::endl;
    // [ 1 2 0 ]
    // [ 3 4 0 ]
    // [ 0 0 0 ]

    // Be careful this will not work:
    //compressed_matrix<double> J2(4,4);
    //J2<<=fill_policy::sparse_push_back(),
     //   J,J;
    // That's because the second J2's elements
    // are attempted to be assigned at positions
    // that come before the elements already pushed.
    // Unfortunatelly that's the only thing you can do in this case
    // (or of course make a custom agorithm):
    compressed_matrix<double> J2(4,4);
    J2<<=fill_policy::sparse_push_back(),
        J, fill_policy::sparse_insert(),
        J;

    std::cout << J2 << std::endl;
    // [  J   J  ]
    // [ 0 0 0 0 ]
    // [ 0 0 0 0 ]

    // A different traverse policy doesn't change the result, only they order it is been assigned.
    coordinate_matrix<double> L(3,3);
    L<<=fill_policy::sparse_insert(), traverse_policy::by_column(),
        J;
    std::cout << L << std::endl;
    // (same as previous)
    // [ 1 2 0 ]
    // [ 3 4 0 ]
    // [ 0 0 0 ]

    typedef coordinate_matrix<double>::size_type cmst;
    const cmst size = 30;
    //typedef fill_policy::sparse_push_back spb;
    // Although the above could have been used the following is may be faster if
    //  you use the policy often and for relatively small containers.
    static fill_policy::sparse_push_back spb;

    // A block diagonal sparse using a loop:
    compressed_matrix<double> M(size, size, 4*15);
    for (cmst i=0; i!=size; i+=J.size1())
        M <<= spb, move_to(i,i), J;


    // If typedef was used above the last expression should start
    // with M <<= spb()...

    // Displaying so that blocks can be easily seen:
    for (unsigned int i=0; i!=M.size1(); i++) {
        std::cout << M(i,0);
        for (unsigned int j=1; j!=M.size2(); j++) std::cout << ", " << M(i,j);
        std::cout << "\n";
    }
    // [ J 0 0 0 ... 0]
    // [ 0 J 0 0 ... 0]
    // [ 0 . . . ... 0]
    // [ 0 0 ... 0 0 J]


    // A "repeat" trasverser may by provided so that this becomes faster and an on-liner like:
    // M <<= spb, repeat(0, size, J.size1(), 0, size, J.size1()), J;
    // An alternate would be to create a :repeater" matrix and vector expression that can be used in other places as well. The latter is probably better,
    return 0;
}


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
#include <boost/numeric/ublas/vector_sparse.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include "libs/numeric/ublas/test/utils.hpp"
#include <boost/timer.hpp>
#include <ctime>

using namespace boost::numeric::ublas;

namespace tans {
template <class AE>
typename AE::value_type mean_square(const matrix_expression<AE> &me) {
    typename AE::value_type s(0);
    typename AE::size_type i, j;
    for (i=0; i!= me().size1(); i++) {
        for (j=0; j!= me().size2(); j++) {
          s+= scalar_traits<typename AE::value_type>::type_abs(me()(i,j));
        }
    }
    return s/me().size1()*me().size2();
}


template <class AE>
typename AE::value_type mean_square(const vector_expression<AE> &ve) {
    // We could have use norm2 here, but ublas' ABS does not support unsigned types.
    typename AE::value_type s(0);
    typename AE::size_type i;
    for (i=0; i!= ve().size(); i++) {
            s+=scalar_traits<typename AE::value_type>::type_abs(ve()(i));
        }
    return s/ve().size();
}
const double TOL=0.0;

}

template <class V>
bool test_vector() {
    bool pass = true;
    using namespace tans;

    V a(3), ra(3);
    a <<=  1, 2, 3;
    ra(0) = 1; ra(1) = 2; ra(2) = 3;
    pass &= (mean_square(a-ra)<=TOL);

    V b(7), rb(7);
    b<<= a, 10, a;
    rb(0) = 1; rb(1) = 2; rb(2) = 3; rb(3)=10, rb(4)= 1; rb(5)=2; rb(6)=3;
    pass &= (mean_square(b-rb)<=TOL);

    V c(6), rc(6);
    c <<= 1, move(2), 3 ,4, 5, move(-5), 10, 10;
    rc(0) = 1; rc(1) = 10; rc(2) = 10; rc(3) = 3; rc(4) = 4; rc(5) = 5;
    pass &= (mean_square(c-rc)<=TOL);

    V d(6), rd(6);
    d <<= 1, move_to(3), 3 ,4, 5, move_to(1), 10, 10;
    rd(0) = 1; rd(1) = 10; rd(2) = 10; rd(3) = 3; rd(4) = 4; rd(5) = 5;
    pass &= (mean_square(d-rd)<=TOL);

    {
    V c(6), rc(6);
    c <<= 1, move<2>(), 3 ,4, 5, move<-5>(), 10, 10;
    rc(0) = 1; rc(1) = 10; rc(2) = 10; rc(3) = 3; rc(4) = 4; rc(5) = 5;
    pass &= (mean_square(c-rc)<=TOL);

    V d(6), rd(6);
    d <<= 1, move_to<3>(), 3 ,4, 5, move_to<1>(), 10, 10;
    rd(0) = 1; rd(1) = 10; rd(2) = 10; rd(3) = 3; rd(4) = 4; rd(5) = 5;
    pass &= (mean_square(d-rd)<=TOL);
    }


    {
    V f(6), rf(6);
    f <<= 5, 5, 5, 5, 5, 5;
    V fa(3); fa<<= 1, 2, 3;
    f <<= fill_policy::index_plus_assign(), fa;
    rf <<= 6,7,8, 5, 5, 5;
    pass &= (mean_square(f-rf)<=TOL);
    }

    {
    V f(6), rf(6);
    f <<= 5, 5, 5, 5, 5, 5;
    V fa(3); fa<<= 1, 2, 3;
    f <<= fill_policy::index_minus_assign(), fa;
    rf <<= 4,3,2, 5, 5, 5;
    pass &= (mean_square(f-rf)<=TOL);
    }

    return pass;
}

template <class V>
bool test_vector_sparse_push_back() {
    bool pass = true;
    using namespace tans;

    V a(3), ra(3);
    a <<= fill_policy::sparse_push_back(), 1, 2, 3;
    ra(0) = 1; ra(1) = 2; ra(2) = 3;
    pass &= (mean_square(a-ra)<=TOL);

    V b(7), rb(7);
    b<<= fill_policy::sparse_push_back(), a, 10, a;
    rb(0) = 1; rb(1) = 2; rb(2) = 3; rb(3)=10, rb(4)= 1; rb(5)=2; rb(6)=3;
    pass &= (mean_square(b-rb)<=TOL);

    V c(6), rc(6);
    c <<= fill_policy::sparse_push_back(), 1, move(2), 3 ,4, 5; // Move back (i.e. negative is dangerous for push_back)
    rc(0) = 1; rc(1) = 0; rc(2) = 0; rc(3) = 3; rc(4) = 4; rc(5) = 5;
    pass &= (mean_square(c-rc)<=TOL);

    V d(6), rd(6);
    d <<= fill_policy::sparse_push_back(), 1, move_to(3), 3 ,4, 5; // Move back (i.e. before current index is dangerous for push_back)
    rd(0) = 1; rd(1) = 0; rd(2) = 0; rd(3) = 3; rd(4) = 4; rd(5) = 5;
    pass &= (mean_square(d-rd)<=TOL);

    V e(6), re(6);
    e <<= fill_policy::sparse_push_back(), 1, move_to(3), 3 ,4, 5, fill_policy::sparse_insert(), move_to(1), 10, 10; // If you want to move back, use this
    re(0) = 1; re(1) = 10; re(2) = 10; re(3) = 3; re(4) = 4; re(5) = 5;
    pass &= (mean_square(e-re)<=TOL);

    return pass;
}


template <class V>
bool test_vector_sparse_insert() {
    bool pass = true;
    using namespace tans;

    V a(3), ra(3);
    a <<= fill_policy::sparse_insert(), 1, 2, 3;
    ra(0) = 1; ra(1) = 2; ra(2) = 3;
    pass &= (mean_square(a-ra)<=TOL);

    V b(7), rb(7);
    b<<= fill_policy::sparse_insert(), a, 10, a;
    rb(0) = 1; rb(1) = 2; rb(2) = 3; rb(3)=10, rb(4)= 1; rb(5)=2; rb(6)=3;
    pass &= (mean_square(b-rb)<=TOL);

    V c(6), rc(6);
    c <<= fill_policy::sparse_insert(), 1, move(2), 3 ,4, 5, move(-5), 10, 10; // Move back (i.e. negative is dangerous for sparse)
    rc(0) = 1; rc(1) = 10; rc(2) = 10; rc(3) = 3; rc(4) = 4; rc(5) = 5;
    pass &= (mean_square(c-rc)<=TOL);


    V d(6), rd(6);
    d <<= fill_policy::sparse_insert(), 1, move_to(3), 3 ,4, 5, move_to(1), 10, 10; // Move back (i.e.before is dangerous for sparse)
    rd(0) = 1; rd(1) = 10; rd(2) = 10; rd(3) = 3; rd(4) = 4; rd(5) = 5;
    pass &= (mean_square(d-rd)<=TOL);


    return pass;
}


template <class V>
bool test_matrix() {
    bool pass = true;
    using namespace tans;

    V A(3,3), RA(3,3);
    A <<= 1, 2, 3, 4, 5, 6, 7, 8, 9;
    RA(0,0)= 1; RA(0,1)=2; RA(0,2)=3;
    RA(1,0)= 4; RA(1,1)=5; RA(1,2)=6;
    RA(2,0)= 7; RA(2,1)=8; RA(2,2)=9;
    pass &= (mean_square(A-RA)<=TOL);

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(3);
    b<<= 4,5,6;
    B<<= 1, 2, 3, b, 7, project(b, range(1,3));
    RB<<=1, 2, 3, 4, 5, 6, 7, 5, 6; // If the first worked we can now probably use it.
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(3);
    b<<= 4,5,6;
    B<<= move(1,0), b, move_to(0,0), 1, 2, 3, move(1,0), 7, project(b, range(1,3));
    RB<<=1, 2, 3, 4, 5, 6, 7, 5, 6;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(9);
    b<<= 1, 2, 3, 4, 5, 6, 7, 8, 9;
    B<<=b;
    RB<<=1, 2, 3, 4, 5, 6, 7, 8, 9;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<=    2, 3,
            4, 5;
    B<<= C,C,
        C,C;
    RB <<=   2,3,2,3,
            4,5,4,5,
            2,3,2,3,
            4,5,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<= C, zero_matrix<typename V::value_type>(2,2),
        zero_matrix<typename V::value_type>(2,2), C;
    RB<<=    2,3,0,0,
            4,5,0,0,
            0,0,2,3,
            0,0,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<= C, zero_matrix<typename V::value_type>(2,2),
        zero_matrix<typename V::value_type>(2,2), C;
    RB<<=    2,3,0,0,
            4,5,0,0,
            0,0,2,3,
            0,0,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4); // We need that because of the non-zero instatiation of dense types.
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<= move(1,1), C;
    RB<<=    0,0,0,0,
            0,2,3,0,
            0,4,5,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<= move_to(0,1), 2, 3, next_row(), 1, 2, next_row(), 4, 5;
    RB<<=    0,2,3,0,
            1,2,0,0,
            4,5,0,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=traverse_policy::by_column(), move_to(0,1), 2, 3, 6, next_column(), 4, 5;
    RB<<=    0,2,4,0,
            0,3,5,0,
            0,6,0,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=traverse_policy::by_column(), move_to(0,1), 2, 3, next_row(), traverse_policy::by_row(), 4, 5;
    RB<<=    0,2,0,0,
            0,3,0,0,
            0,0,0,0,
            4,5,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=traverse_policy::by_column(), move_to(0,1), 2, 3, begin2(), traverse_policy::by_row(), 4, 5, 6, 7, 8;
    RB<<=    0,2,0,0,
            0,3,0,0,
            4,5,6,7,
            8,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=traverse_policy::by_column(), move_to(0,1), 2, 3, begin2(), traverse_policy::by_row(), 4, 5, 6, 7, 8,9, begin1(), 1, 2;
    RB<<=    0,2,1,2,
            0,3,0,0,
            4,5,6,7,
            8,9,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = scalar_matrix<typename V::value_type>(4,4,1);
    V C(2,2);
    C <<= 1, 2, 3, 4;
    B<<= fill_policy::index_plus_assign(), move(1,1), C;
    RB<<=    1,1,1,1,
            1,2,3,1,
            1,4,5,1,
            1,1,1,1;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = scalar_matrix<typename V::value_type>(4,4,5);
    V C(2,2);
    C <<= 1, 2, 3, 4;
    B<<= fill_policy::index_minus_assign(), move(1,1), C;
    RB<<=    5,5,5,5,
            5,4,3,5,
            5,2,1,5,
            5,5,5,5;
    pass &= (mean_square(B-RB)<=TOL);
    }


    return pass;
}

template <class V>
bool test_matrix_sparse_push_back() {
    bool pass = true;
    using namespace tans;

    V A(3,3), RA(3,3);
    A <<= fill_policy::sparse_push_back(), 1, 2, 3, 4, 5, 6, 7, 8, 9;
    RA(0,0)= 1; RA(0,1)=2; RA(0,2)=3;
    RA(1,0)= 4; RA(1,1)=5; RA(1,2)=6;
    RA(2,0)= 7; RA(2,1)=8; RA(2,2)=9;
    pass &= (mean_square(A-RA)<=TOL);

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(3);
    b<<= 4,5,6;
    B<<=fill_policy::sparse_push_back(), 1, 2, 3, b, 7, project(b, range(1,3));
    RB<<= 1, 2, 3, 4, 5, 6, 7, 5, 6; // If the first worked we can now probably use it.
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(3);
    b<<= 4,5,6;
    B<<=fill_policy::sparse_push_back(), move(1,0), b, fill_policy::sparse_insert(), move_to(0,0), 1, 2, 3, move(1,0), 7, project(b, range(1,3));
    RB<<=1, 2, 3, 4, 5, 6, 7, 5, 6;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(9);
    b<<= 1, 2, 3, 4, 5, 6, 7, 8, 9;
    B<<=b;
    RB<<=1, 2, 3, 4, 5, 6, 7, 8, 9;
    pass &= (mean_square(B-RB)<=TOL);
    }


    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<=    2, 3,
            4, 5;
    // It might get complicated for sparse push_back, this must go into the tutorial. (This way is not convient nor fast)
    B<<=fill_policy::sparse_push_back(), C, move_to(2,2), C, fill_policy::sparse_insert(), move_to(0,2), C, C;
    RB <<=   2,3,2,3,
            4,5,4,5,
            2,3,2,3,
            4,5,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }


    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<=fill_policy::sparse_push_back(), C, move_to(2,2), C;
    RB<<=    2,3,0,0,
            4,5,0,0,
            0,0,2,3,
            0,0,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<=fill_policy::sparse_push_back(), move(1,1), C;
    RB<<=    0,0,0,0,
            0,2,3,0,
            0,4,5,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_push_back(), move_to(0,1), 2, 3, next_row(), 1, 2, next_row(), 4, 5;
    RB<<=    0,2,3,0,
            1,2,0,0,
            4,5,0,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }
    // The next will not work with sparse push_back because elements that are prior to the ones already in are attempted to be added
/*
    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_push_back(),traverse_policy::by_column(), move_to(0,1), 2, 3, 6, next_column(), 4, 5;
    RB<<=    0,2,4,0,
            0,3,5,0,
            0,6,0,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }
*/
    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_push_back(),traverse_policy::by_column(), move_to(0,1), 2, 3, next_row(), traverse_policy::by_row(), 4, 5;
    RB<<=    0,2,0,0,
            0,3,0,0,
            0,0,0,0,
            4,5,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_push_back(),traverse_policy::by_column(), move_to(0,1), 2, 3, begin2(), traverse_policy::by_row(), 4, 5, 6, 7, 8;
    RB<<=    0,2,0,0,
            0,3,0,0,
            4,5,6,7,
            8,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    // The next will not work with sparse push_back because elements that are prior to the ones already in are attempted to be added
/*
    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_push_back(),traverse_policy::by_column(), move_to(0,1), 2, 3, begin2(), traverse_policy::by_row(), 4, 5, 6, 7, 8,9, begin1(), 1, 2;
    RB<<=    0,2,1,2,
            0,3,0,0,
            4,5,6,7,
            8,9,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }
*/
    return pass;
}

template <class V>
bool test_matrix_sparse_insert() {
    bool pass = true;
    using namespace tans;

    V A(3,3), RA(3,3);
    A <<= fill_policy::sparse_insert(), 1, 2, 3, 4, 5, 6, 7, 8, 9;
    RA(0,0)= 1; RA(0,1)=2; RA(0,2)=3;
    RA(1,0)= 4; RA(1,1)=5; RA(1,2)=6;
    RA(2,0)= 7; RA(2,1)=8; RA(2,2)=9;
    pass &= (mean_square(A-RA)<=TOL);

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(3);
    b<<= 4,5,6;
    B<<=fill_policy::sparse_insert(), 1, 2, 3, b, 7, project(b, range(1,3));
    RB<<=1, 2, 3, 4, 5, 6, 7, 5, 6; // If the first worked we can now probably use it.
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(3);
    b<<= 4,5,6;
    B<<=fill_policy::sparse_insert(), move(1,0), b, fill_policy::sparse_insert(), move_to(0,0), 1, 2, 3, move(1,0), 7, project(b, range(1,3));
    RB<<=1, 2, 3, 4, 5, 6, 7, 5, 6;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(3,3), RB(3,3);
    vector<typename V::value_type>  b(9);
    b<<= 1, 2, 3, 4, 5, 6, 7, 8, 9;
    B<<=b;
    RB<<=1, 2, 3, 4, 5, 6, 7, 8, 9;
    pass &= (mean_square(B-RB)<=TOL);
    }


    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<=    2, 3,
            4, 5;
    B<<=fill_policy::sparse_insert(), C, C, C, C;
    RB <<=   2,3,2,3,
            4,5,4,5,
            2,3,2,3,
            4,5,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }


    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<=fill_policy::sparse_insert(), C, move_to(2,2), C;
    RB<<=    2,3,0,0,
            4,5,0,0,
            0,0,2,3,
            0,0,4,5;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    V C(2,2);
    C <<= 2, 3, 4, 5;
    B<<=fill_policy::sparse_insert(), move(1,1), C;
    RB<<=    0,0,0,0,
            0,2,3,0,
            0,4,5,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_insert(), move_to(0,1), 2, 3, next_row(), 1, 2, next_row(), 4, 5;
    RB<<=    0,2,3,0,
            1,2,0,0,
            4,5,0,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_insert(),traverse_policy::by_column(), move_to(0,1), 2, 3, 6, next_column(), 4, 5;
    RB<<=    0,2,4,0,
            0,3,5,0,
            0,6,0,0,
            0,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_insert(),traverse_policy::by_column(), move_to(0,1), 2, 3, next_row(), traverse_policy::by_row(), 4, 5;
    RB<<=    0,2,0,0,
            0,3,0,0,
            0,0,0,0,
            4,5,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_insert(),traverse_policy::by_column(), move_to(0,1), 2, 3, begin2(), traverse_policy::by_row(), 4, 5, 6, 7, 8;
    RB<<=    0,2,0,0,
            0,3,0,0,
            4,5,6,7,
            8,0,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    {
    V B(4,4), RB(4,4);
    B = zero_matrix<typename V::value_type>(4,4);
    B<<=fill_policy::sparse_insert(),traverse_policy::by_column(), move_to(0,1), 2, 3, begin2(), traverse_policy::by_row(), 4, 5, 6, 7, 8,9, begin1(), 1, 2;
    RB<<=    0,2,1,2,
            0,3,0,0,
            4,5,6,7,
            8,9,0,0;
    pass &= (mean_square(B-RB)<=TOL);
    }

    return pass;
}


BOOST_UBLAS_TEST_DEF (test_vector) {

    BOOST_UBLAS_DEBUG_TRACE( "Starting operator \"<<= \" vector assignment tests" );

    BOOST_UBLAS_TEST_CHECK(test_vector<vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<vector<char> >());

    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<double,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<float,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<long,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<unsigned long,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<int,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<unsigned int,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<std::size_t,7> >()));
    BOOST_UBLAS_TEST_CHECK((test_vector<bounded_vector<char,7> >()));

    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<unsigned int> >())
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<std::size_t> >())
    BOOST_UBLAS_TEST_CHECK(test_vector<mapped_vector<char> >());

    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<compressed_vector<char> >());

    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<long> >())
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<unsigned long> >())
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector<coordinate_vector<char> >());

    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<compressed_vector<char> >());

    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_push_back<coordinate_vector<char> >());

    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<compressed_vector<char> >());

    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<double> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<float> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_vector_sparse_insert<coordinate_vector<char> >());
}

BOOST_UBLAS_TEST_DEF (test_matrix) {

    BOOST_UBLAS_DEBUG_TRACE( "Starting operator \"<<= \" matrix assignment tests" );

    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<matrix<char> >());

    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<double,7, 7> >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<float,7, 7> >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<long,7, 7> >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<unsigned long,7, 7> >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<int,7,7 > >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<unsigned int,7, 7> >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<char,7, 7> >()));
    BOOST_UBLAS_TEST_CHECK((test_matrix<bounded_matrix<std::size_t,7, 7> >()));

    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<unsigned int> >())
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<std::size_t> >())
    BOOST_UBLAS_TEST_CHECK(test_matrix<mapped_matrix<char> >());

    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<compressed_matrix<char> >());

    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<long> >())
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<unsigned long> >())
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix<coordinate_matrix<char> >());

    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<compressed_matrix<char> >());

    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_push_back<coordinate_matrix<char> >());


    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<compressed_matrix<char> >());

    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<double> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<float> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<unsigned long> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<unsigned int> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<std::size_t> >());
    BOOST_UBLAS_TEST_CHECK(test_matrix_sparse_insert<coordinate_matrix<char> >());
}


int main () {
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_vector );
    BOOST_UBLAS_TEST_DO( test_matrix );

    BOOST_UBLAS_TEST_END();

    return EXIT_SUCCESS;;
}

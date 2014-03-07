//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#include <iostream>

#include <boost/numeric/interval.hpp>
#include <boost/numeric/interval/io.hpp>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "test7.hpp"

// this testcase requires fix of task #2473 

int main () {
    test_vector ();
    test_matrix_vector ();
    test_matrix ();
    return 0;
}

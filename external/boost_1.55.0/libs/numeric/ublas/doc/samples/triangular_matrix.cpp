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

#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/io.hpp>

int main () {
    using namespace boost::numeric::ublas;
    triangular_matrix<double, lower> ml (3, 3);
    for (unsigned i = 0; i < ml.size1 (); ++ i)
        for (unsigned j = 0; j <= i; ++ j)
            ml (i, j) = 3 * i + j;
    std::cout << ml << std::endl;
    triangular_matrix<double, upper> mu (3, 3);
    for (unsigned i = 0; i < mu.size1 (); ++ i)
        for (unsigned j = i; j < mu.size2 (); ++ j)
            mu (i, j) = 3 * i + j;
    std::cout << mu << std::endl;
}


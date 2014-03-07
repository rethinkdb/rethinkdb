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

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/io.hpp>

int main () {
    using namespace boost::numeric::ublas;
    matrix<double> m (3, 3);
    symmetric_adaptor<matrix<double>, lower> sal (m);
    for (unsigned i = 0; i < sal.size1 (); ++ i)
        for (unsigned j = 0; j <= i; ++ j)
            sal (i, j) = 3 * i + j;
    std::cout << sal << std::endl;
    symmetric_adaptor<matrix<double>, upper> sau (m);
    for (unsigned i = 0; i < sau.size1 (); ++ i)
        for (unsigned j = i; j < sau.size2 (); ++ j)
            sau (i, j) = 3 * i + j;
    std::cout << sau << std::endl;
}


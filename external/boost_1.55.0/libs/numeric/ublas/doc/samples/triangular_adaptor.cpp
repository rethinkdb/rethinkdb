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
    matrix<double> m (3, 3);
    triangular_adaptor<matrix<double>, lower> tal (m);
    for (unsigned i = 0; i < tal.size1 (); ++ i)
        for (unsigned j = 0; j <= i; ++ j)
            tal (i, j) = 3 * i + j;
    std::cout << tal << std::endl;
    triangular_adaptor<matrix<double>, upper> tau (m);
    for (unsigned i = 0; i < tau.size1 (); ++ i)
        for (unsigned j = i; j < tau.size2 (); ++ j)
            tau (i, j) = 3 * i + j;
    std::cout << tau << std::endl;
}


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

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>

int main () {
    using namespace boost::numeric::ublas;
    matrix<double> m (3, 3);
    matrix_slice<matrix<double> > ms (m, slice (0, 1, 3), slice (0, 1, 3));
    for (unsigned i = 0; i < ms.size1 (); ++ i)
        for (unsigned j = 0; j < ms.size2 (); ++ j)
            ms (i, j) = 3 * i + j;
    std::cout << ms << std::endl;
}


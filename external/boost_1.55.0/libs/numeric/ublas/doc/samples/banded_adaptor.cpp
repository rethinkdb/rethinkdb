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

#include <boost/numeric/ublas/banded.hpp>
#include <boost/numeric/ublas/io.hpp>

int main () {
    using namespace boost::numeric::ublas;
    matrix<double> m (3, 3);
    banded_adaptor<matrix<double> > ba (m, 1, 1);
    for (signed i = 0; i < signed (ba.size1 ()); ++ i)
        for (signed j = (std::max) (i - 1, 0); j < (std::min) (i + 2, signed (ba.size2 ())); ++ j)
            ba (i, j) = 3 * i + j;
    std::cout << ba << std::endl;
}


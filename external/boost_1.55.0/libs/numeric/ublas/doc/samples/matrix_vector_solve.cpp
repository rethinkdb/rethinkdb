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
    vector<double> v (3);
    for (unsigned i = 0; i < (std::min) (m.size1 (), v.size ()); ++ i) {
        for (unsigned j = 0; j <= i; ++ j)
            m (i, j) = 3 * i + j + 1;
        v (i) = i;
    }

    std::cout << solve (m, v, lower_tag ()) << std::endl;
    std::cout << solve (v, m, lower_tag ()) << std::endl;
}


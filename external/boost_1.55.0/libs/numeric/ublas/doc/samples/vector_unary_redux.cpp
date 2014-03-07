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

#include <boost/numeric/ublas/vector.hpp>

int main () {
    using namespace boost::numeric::ublas;
    vector<double> v (3);
    for (unsigned i = 0; i < v.size (); ++ i)
        v (i) = i;

    std::cout << sum (v) << std::endl;
    std::cout << norm_1 (v) << std::endl;
    std::cout << norm_2 (v) << std::endl;
    std::cout << norm_inf (v) << std::endl;
    std::cout << index_norm_inf (v) << std::endl;
}


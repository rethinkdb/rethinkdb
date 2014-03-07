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

#include <boost/numeric/ublas/storage_sparse.hpp>

int main () {
    using namespace boost::numeric::ublas;
    map_array<int, double> a;
    a.reserve (3);
    for (unsigned i = 0; i < 3; ++ i) {
        a [i] = i;
        std::cout << a [i] << std::endl;
    }
}


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

#include <boost/numeric/ublas/storage.hpp>

int main () {
    using namespace boost::numeric::ublas;
    bounded_array<double, 3> a (3);
    for (unsigned i = 0; i < a.size (); ++ i) {
        a [i] = i;
        std::cout << a [i] << std::endl;
    }
}


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
#include <boost/numeric/ublas/io.hpp>

int main () {
    using namespace boost::numeric::ublas;
    vector<std::complex<double> > v (3);
    for (unsigned i = 0; i < v.size (); ++ i)
        v (i) = std::complex<double> (i, i);

    std::cout << - v << std::endl;
    std::cout << conj (v) << std::endl;
    std::cout << real (v) << std::endl;
    std::cout << imag (v) << std::endl;
    std::cout << trans (v) << std::endl;
    std::cout << herm (v) << std::endl;
}


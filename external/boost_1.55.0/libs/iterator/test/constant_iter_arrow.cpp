// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/iterator_adaptor.hpp>
#include <utility>

struct my_iter : boost::iterator_adaptor<my_iter, std::pair<int,int> const*>
{
    my_iter(std::pair<int,int> const*);
    my_iter();
};

std::pair<int,int> const x(1,1);
my_iter p(&x);
int y = p->first; // operator-> attempts to return an non-const pointer

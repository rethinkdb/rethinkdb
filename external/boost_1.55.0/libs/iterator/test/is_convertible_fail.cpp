//
// Copyright (c) Thomas Witt 2002.
//
// Use, modification and distribution is subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/cstdlib.hpp>

int main()
{
    typedef boost::reverse_iterator<int*>  rev_iter1;
    typedef boost::reverse_iterator<char*> rev_iter2;

    return boost::is_convertible<rev_iter1, rev_iter2>::value
        ? boost::exit_failure : boost::exit_success;
}

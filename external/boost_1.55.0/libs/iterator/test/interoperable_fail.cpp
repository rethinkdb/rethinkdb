// Copyright Thomas Witt 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/iterator/reverse_iterator.hpp>
#include <boost/concept_check.hpp>
#include <boost/cstdlib.hpp>
#include <list>

int main()
{
  {
    typedef boost::reverse_iterator<std::list<int*>::iterator>   rev_iter;
    typedef boost::indirect_iterator<std::list<int*>::iterator>  ind_iter;

    ind_iter() == rev_iter();
  }

  return boost::exit_success;
}

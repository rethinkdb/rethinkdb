
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/function_types/function_type.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;
using boost::is_same;

typedef int expected(int,int);

BOOST_MPL_ASSERT((
  is_same< ft::function_type< mpl::vector<int,int,int> >::type, expected >
));


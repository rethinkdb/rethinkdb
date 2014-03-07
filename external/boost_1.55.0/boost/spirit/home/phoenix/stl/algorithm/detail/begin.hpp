// Copyright 2005 Daniel Wallin. 
// Copyright 2005 Joel de Guzman.
//
// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler

#ifndef PHOENIX_ALGORITHM_DETAIL_BEGIN_HPP
#define PHOENIX_ALGORITHM_DETAIL_BEGIN_HPP

#include <boost/range/result_iterator.hpp>
#include <boost/range/begin.hpp>

namespace boost { namespace phoenix {
namespace detail
{
  template<class R>
  typename range_result_iterator<R>::type
  begin_(R& r)
  {
      return boost::begin(r);
  }
}
}}

#endif

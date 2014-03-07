// Copyright (C) 2007  Douglas Gregor  <doug.gregor@gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/detail/tag_allocator.hpp>

namespace boost { namespace graph { namespace distributed { namespace detail {

tag_allocator::token tag_allocator::get_tag() 
{
  int tag;

  if (!freed.empty()) {
    // Grab the tag at the top of the stack.
    tag = freed.back();
    freed.pop_back();
  } else
    // Grab the bottom tag and move the bottom down
    tag = bottom--;

  return token(this, tag);
}

tag_allocator::token::token(const token& other)
  : allocator(other.allocator), tag_(other.tag_)
{
  // other no longer owns this tag
  other.tag_ = -1;
}

tag_allocator::token::~token()
{
  if (tag_ != -1) {
    if (tag_ == allocator->bottom + 1)
      // This is the bottommost tag: just bump up the bottom
      ++allocator->bottom;
    else
      // This tag isn't the bottom, so push it only the freed stack
      allocator->freed.push_back(tag_);
  }
}

} } } } // end namespace boost::graph::distributed::detail

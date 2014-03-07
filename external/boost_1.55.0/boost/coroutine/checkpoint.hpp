
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_CHECKPOINT_H
#define BOOST_COROUTINES_CHECKPOINT_H

#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

class checkpoint
{
public:
private:
    context::fcontext_t ctx_;
    void            *   sp_;
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_CHECKPOINT_H

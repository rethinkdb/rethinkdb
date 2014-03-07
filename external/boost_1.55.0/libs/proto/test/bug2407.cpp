///////////////////////////////////////////////////////////////////////////////
// bug2407.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/proto/proto.hpp>

namespace mpl = boost::mpl;
namespace proto = boost::proto;
using proto::_;

template<class E>
struct e;

struct g
  : proto::or_<
        proto::terminal<int>
      , proto::plus<g,g>
    >
{};

struct d
  : proto::domain<proto::generator<e>, g>
{};

template<class E>
struct e
  : proto::extends<E, e<E>, d>
{
    BOOST_MPL_ASSERT((proto::matches<E, g>));

    e(E const &x = E())
      : proto::extends<E, e<E>, d>(x)
    {}
};

e<proto::terminal<int>::type> i;

template<class E>
std::ostream &operator<<(std::ostream &sout, e<E> const &x)
{
    return sout;
}

int main()
{
    std::cout << (i+i);
}

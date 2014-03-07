/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>

namespace phoenix = boost::phoenix;
namespace proto   = boost::proto;


// define the expression
namespace expression
{
    template <typename Lhs, typename Rhs>
    struct plus
        : phoenix::expr<proto::tag::plus, Lhs, Rhs>
    {};
}

// extend the grammar, to recognice the expression
namespace boost { namespace phoenix {

    template <>
    struct meta_grammar::case_<proto::tag::plus>
        : enable_rule<
            ::expression::plus<
                meta_grammar
              , meta_grammar
            >
        >
    {};

}}

// build a generator
template <typename Lhs, typename Rhs>
typename expression::plus<Lhs, Rhs>::type
plus(Lhs const & lhs, Rhs const & rhs)
{
    return expression::plus<Lhs, Rhs>::make(lhs, rhs);
}

#include <boost/proto/proto.hpp>
#include <iostream>

int main()
{

    plus(6, 5);

    proto::display_expr(plus(6, 5));

    std::cout << plus(5, 6)() << "\n";
}



/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_special_ops.hpp>

using namespace std;
using namespace phoenix;

//////////////////////////////////
template <typename CondT, typename TrueT, typename FalseT>
struct if_else_composite {

    typedef if_else_composite<CondT, TrueT, FalseT> self_t;

    template <typename TupleT>
    struct result {

        typedef typename higher_rank<
            typename actor_result<TrueT, TupleT>::plain_type,
            typename actor_result<FalseT, TupleT>::plain_type
        >::type type;
    };

    if_else_composite(
        CondT const& cond_, TrueT const& true__, FalseT const& false__)
    :   cond(cond_), true_(true__), false_(false__) {}

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& args) const
    {
        return cond.eval(args) ? true_.eval(args) : false_.eval(args);
    }

    CondT cond; TrueT true_; FalseT false_; //  actors
};

//////////////////////////////////
template <typename CondT, typename TrueT, typename FalseT>
inline actor<if_else_composite<
    typename as_actor<CondT>::type,
    typename as_actor<TrueT>::type,
    typename as_actor<FalseT>::type> >
if_else_(CondT const& cond, TrueT const& true_, FalseT const& false_)
{
    typedef if_else_composite<
        typename as_actor<CondT>::type,
        typename as_actor<TrueT>::type,
        typename as_actor<FalseT>::type>
    result;

    return result(
        as_actor<CondT>::convert(cond),
        as_actor<TrueT>::convert(true_),
        as_actor<FalseT>::convert(false_));
}

//////////////////////////////////
int
main()
{
    int init[] = { 2, 10, 4, 5, 1, 6, 8, 3, 9, 7 };
    vector<int> c(init, init + 10);
    typedef vector<int>::iterator iterator;

    //  Print all contents of an stl container c and
    //  prefix " is odd" or " is even" appropriately.

    for_each(c.begin(), c.end(),
        cout
            << arg1
            << if_else_(arg1 % 2 == 1, " is odd", " is even")
            << val('\n')
    );

    return 0;
}

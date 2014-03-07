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

#define PHOENIX_LIMIT 5
#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_special_ops.hpp>
#include <boost/spirit/include/phoenix1_statements.hpp>

namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  local_tuple
//
//      This *is a* tuple like the one we see in TupleT in any actor
//      base class' eval member function. local_tuple should look and
//      feel the same as a tupled-args, that's why it is derived from
//      TupleArgsT. It has an added member, locs which is another tuple
//      where the local variables will be stored. locs is mutable to
//      allow read-write access to our locals regardless of
//      local_tuple's constness (The eval member function accepts it as
//      a const argument).
//
///////////////////////////////////////////////////////////////////////////////
template <typename TupleArgsT, typename TupleLocsT>
struct local_tuple : public TupleArgsT {

    typedef TupleLocsT local_vars_t;

    local_tuple(TupleArgsT const& args, TupleLocsT const& locs_)
    :   TupleArgsT(args), locs(locs_) {}

    mutable TupleLocsT locs;
};

///////////////////////////////////////////////////////////////////////////////
//
//  local_var_result
//
//      This is a return type computer. Given a constant integer N and a
//      tuple, get the Nth local variable type. If TupleT is not really
//      a local_tuple, we just return nil_t. Otherwise we get the Nth
//      local variable type.
//
///////////////////////////////////////////////////////////////////////////////
template <int N, typename TupleT>
struct local_var_result {

    typedef nil_t type;
};

//////////////////////////////////
template <int N, typename TupleArgsT, typename TupleLocsT>
struct local_var_result<N, local_tuple<TupleArgsT, TupleLocsT> > {

    typedef typename tuple_element<N, TupleLocsT>::type& type;
};

///////////////////////////////////////////////////////////////////////////////
//
//  local_var
//
//      This class looks so curiously like the argument class. local_var
//      provides access to the Nth local variable packed in the tuple
//      duo local_tuple above. Note that the member function eval
//      expects a local_tuple argument. Otherwise the expression
//      'tuple.locs' will fail (compile-time error). local_var
//      primitives only work within the context of a locals_composite
//      (see below).
//
//      Provided are some predefined local_var actors for 0..N local
//      variable access: loc1..locN.
//
///////////////////////////////////////////////////////////////////////////////
template <int N>
struct local_var {

    template <typename TupleT>
    struct result {

        typedef typename local_var_result<N, TupleT>::type type;
    };

    template <typename TupleT>
    typename local_var_result<N, TupleT>::type
    eval(TupleT const& tuple) const
    {
        return tuple.locs[tuple_index<N>()];
    }
};

//////////////////////////////////
actor<local_var<0> > const loc1 = local_var<0>();
actor<local_var<1> > const loc2 = local_var<1>();
actor<local_var<2> > const loc3 = local_var<2>();
actor<local_var<3> > const loc4 = local_var<3>();
actor<local_var<4> > const loc5 = local_var<4>();

///////////////////////////////////////////////////////////////////////////////
//
//  locals_composite
//
//      This class encapsulates an actor and some local variable
//      initializers packed in a tuple.
//
//      locals_composite is just like a proxy and delegates the actual
//      evaluation to the actor. The actor does the actual work. In the
//      eval member function, before invoking the embedded actor's eval
//      member function, we first stuff an instance of our locals and
//      bundle both 'args' and 'locals' in a local_tuple. This
//      local_tuple instance is created in the stack initializing it
//      with our locals member. We then pass this local_tuple instance
//      as an argument to the actor's eval member function.
//
///////////////////////////////////////////////////////////////////////////////
template <typename ActorT, typename LocsT>
struct locals_composite {

    typedef locals_composite<ActorT, LocsT> self_t;

    template <typename TupleT>
    struct result { typedef typename actor_result<ActorT, TupleT>::type type; };

    locals_composite(ActorT const& actor_, LocsT const& locals_)
    :   actor(actor_), locals(locals_) {}

    template <typename TupleT>
    typename actor_result<ActorT, TupleT>::type
    eval(TupleT const& args) const
    {
        actor.eval(local_tuple<TupleT, LocsT>(args, locals));
    }

    ActorT actor;
    LocsT locals;
};

///////////////////////////////////////////////////////////////////////////////
//
//  locals_gen
//
//      At construction time, this class is given some local var-
//      initializers packed in a tuple. We just store this for later.
//      The operator[] of this class creates the actual locals_composite
//      given an actor. This is responsible for the construct
//      locals<types>[actor].
//
///////////////////////////////////////////////////////////////////////////////
template <typename LocsT>
struct locals_gen {

    locals_gen(LocsT const& locals_)
    :   locals(locals_) {}

    template <typename ActorT>
    actor<locals_composite<typename as_actor<ActorT>::type, LocsT> >
    operator[](ActorT const& actor)
    {
        return locals_composite<typename as_actor<ActorT>::type, LocsT>
            (as_actor<ActorT>::convert(actor), locals);
    }

    LocsT locals;
};

///////////////////////////////////////////////////////////////////////////////
//
//    Front end generator functions. These generators are overloaded for
//    1..N local variables. locals<T0,... TN>(i0,...iN) generate locals_gen
//    objects (see above).
//
///////////////////////////////////////////////////////////////////////////////
template <typename T0>
inline locals_gen<tuple<T0> >
locals(
    T0 const& _0 = T0()
)
{
    typedef tuple<T0> tuple_t;
    return locals_gen<tuple_t>(tuple_t(_0));
}

//////////////////////////////////
template <typename T0, typename T1>
inline locals_gen<tuple<T0, T1> >
locals(
    T0 const& _0 = T0(),
    T1 const& _1 = T1()
)
{
    typedef tuple<T0, T1> tuple_t;
    return locals_gen<tuple_t>(tuple_t(_0, _1));
}

//////////////////////////////////
template <typename T0, typename T1, typename T2>
inline locals_gen<tuple<T0, T1, T2> >
locals(
    T0 const& _0 = T0(),
    T1 const& _1 = T1(),
    T2 const& _2 = T2()
)
{
    typedef tuple<T0, T1, T2> tuple_t;
    return locals_gen<tuple_t>(tuple_t(_0, _1, _2));
}

//////////////////////////////////
template <typename T0, typename T1, typename T2, typename T3>
inline locals_gen<tuple<T0, T1, T2, T3> >
locals(
    T0 const& _0 = T0(),
    T1 const& _1 = T1(),
    T2 const& _2 = T2(),
    T3 const& _3 = T3()
)
{
    typedef tuple<T0, T1, T2, T3> tuple_t;
    return locals_gen<tuple_t>(tuple_t(_0, _1, _2, _3));
}

//////////////////////////////////
template <typename T0, typename T1, typename T2, typename T3, typename T4>
inline locals_gen<tuple<T0, T1, T2, T3, T4> >
locals(
    T0 const& _0 = T0(),
    T1 const& _1 = T1(),
    T2 const& _2 = T2(),
    T3 const& _3 = T3(),
    T4 const& _4 = T4()
)
{
    typedef tuple<T0, T1, T2, T3> tuple_t;
    return locals_gen<tuple_t>(tuple_t(_0, _1, _2, _3, _4));
}

///////////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////
using namespace std;
using namespace phoenix;

//////////////////////////////////
int
main()
{
    int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    vector<int> c(init, init + 10);
    typedef vector<int>::iterator iterator;

    for_each(c.begin(), c.end(),
        locals<int, char const*>(0, "...That's all\n")
        [
            for_(loc1 = 0, loc1 < arg1, ++loc1)
            [
                cout << loc1 << ", "
            ],
            cout << loc2
        ]
    );

    return 0;
}

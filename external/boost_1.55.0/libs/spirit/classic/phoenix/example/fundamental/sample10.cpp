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
#include <boost/spirit/include/phoenix1_functions.hpp>

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

    local_tuple(TupleArgsT const& args, TupleLocsT const& locs_)
    :   TupleArgsT(args), locs(locs_) {}

    mutable TupleLocsT locs;
};

///////////////////////////////////////////////////////////////////////////////
//
//  local_var_result
//
//      This is a return type computer. Given a constant integer N, a
//      parent index and a tuple, get the Nth local variable type. The
//      parent index is an integer specifying which parent scope to
//      access; 0==current scope, 1==parent scope, 2==parent's parent
//      scope etc.
//
//      This is a metaprogram with partial specializations. There is a
//      general case, a special case for local_tuples and a terminating
//      special case for local_tuples.
//
//      General case: If TupleT is not really a local_tuple, we just return nil_t.
//
//      local_tuples case:
//          Parent index is 0: We get the Nth local variable.
//          Otherwise: We subclass from local_tuples<N, Parent-1, TupleArgsT>
//
///////////////////////////////////////////////////////////////////////////////
template <int N, int Parent, typename TupleT>
struct local_var_result {

    typedef nil_t type;
};

//////////////////////////////////
template <int N, int Parent, typename TupleArgsT, typename TupleLocsT>
struct local_var_result<N, Parent, local_tuple<TupleArgsT, TupleLocsT> >
:   public local_var_result<N, Parent-1, TupleArgsT> {};

//////////////////////////////////
template <int N, typename TupleArgsT, typename TupleLocsT>
struct local_var_result<N, 0, local_tuple<TupleArgsT, TupleLocsT> > {

    typedef typename tuple_element<N, TupleLocsT>::type& type;

    static type get(local_tuple<TupleArgsT, TupleLocsT> const& tuple)
    { return tuple.locs[tuple_index<N>()]; }
};

///////////////////////////////////////////////////////////////////////////////
//
//  local_var
//
//      This class looks so curiously like the argument class. local_var
//      provides access to the Nth local variable packed in the tuple
//      duo local_tuple above. Parent specifies the Nth parent scope.
//      0==current scope, 1==parent scope, 2==parent's parent scope etc.
//      The member function parent<N>() may be called to provide access
//      to outer scopes.
//
//      Note that the member function eval expects a local_tuple
//      argument. Otherwise there will be acompile-time error. local_var
//      primitives only work within the context of a context_composite
//      (see below).
//
//      Provided are some predefined local_var actors for 0..N local
//      variable access: lvar1..locN.
//
///////////////////////////////////////////////////////////////////////////////
template <int N, int Parent = 0>
struct local_var {

    typedef local_var<N, Parent> self_t;

    template <typename TupleT>
    struct result {

        typedef typename local_var_result<N, Parent, TupleT>::type type;
    };

    template <typename TupleT>
    typename actor_result<self_t, TupleT>::type
    eval(TupleT const& tuple) const
    {
        return local_var_result<N, Parent, TupleT>::get(tuple);
    }

    template <int PIndex>
    actor<local_var<N, Parent+PIndex> >
    parent() const
    {
        return local_var<N, Parent+PIndex>();
    }
};

//////////////////////////////////
namespace locals {

    actor<local_var<0> > const result   = local_var<0>();
    actor<local_var<1> > const lvar1    = local_var<1>();
    actor<local_var<2> > const lvar2    = local_var<2>();
    actor<local_var<3> > const lvar3    = local_var<3>();
    actor<local_var<4> > const lvar4    = local_var<4>();
}









///////////////////////////////////////////////////////////////////////////////
template <int N, int Parent, typename TupleT>
struct local_func_result {

    typedef nil_t type;
};

//////////////////////////////////
template <int N, int Parent, typename TupleArgsT, typename TupleLocsT>
struct local_func_result<N, Parent, local_tuple<TupleArgsT, TupleLocsT> >
:   public local_func_result<N, Parent-1, TupleArgsT> {};

//////////////////////////////////
template <int N, typename TupleArgsT, typename TupleLocsT>
struct local_func_result<N, 0, local_tuple<TupleArgsT, TupleLocsT> > {

    typedef typename actor_result<
        typename tuple_element<N, TupleLocsT>::type,
        local_tuple<TupleArgsT, TupleLocsT>
    >::type type;

    template <typename ArgsT>
    static type eval(local_tuple<ArgsT, TupleLocsT> const& tuple)
    { return tuple.locs[tuple_index<N>()].eval(tuple); }
};








template <
    int N, int Parent,
    typename A0 = nil_t,
    typename A1 = nil_t,
    typename A2 = nil_t,
    typename A3 = nil_t,
    typename A4 = nil_t
>
struct local_function_actor;

//////////////////////////////////
template <int N, int Parent>
struct local_function_base {

    template <typename TupleT>
    struct result {

        typedef typename local_func_result<N, Parent, TupleT>::type type;
    };
};

//////////////////////////////////
template <int N, int Parent>
struct local_function_actor<N, Parent, nil_t, nil_t, nil_t, nil_t, nil_t>
:   public local_function_base<N, Parent> {

    template <typename TupleArgsT, typename TupleLocsT>
    typename local_func_result<
        N, Parent, local_tuple<TupleArgsT, TupleLocsT> >::type
    eval(local_tuple<TupleArgsT, TupleLocsT> const& args) const
    {
        typedef local_tuple<TupleArgsT, TupleLocsT> local_tuple_t;
        typedef tuple<> tuple_t;
        tuple_t local_args;

        local_tuple<tuple_t, TupleLocsT> local_context(local_args, args.locs);
        return local_func_result<
            N, Parent, local_tuple_t>
        ::eval(local_context);
    }
};

//////////////////////////////////
template <int N, int Parent,
    typename A0>
struct local_function_actor<N, Parent, A0, nil_t, nil_t, nil_t, nil_t>
:   public local_function_base<N, Parent> {

    local_function_actor(
        A0 const& _0)
    :   a0(_0) {}

    template <typename TupleArgsT, typename TupleLocsT>
    typename local_func_result<
        N, Parent, local_tuple<TupleArgsT, TupleLocsT> >::type
    eval(local_tuple<TupleArgsT, TupleLocsT> const& args) const
    {
        typedef local_tuple<TupleArgsT, TupleLocsT> local_tuple_t;
        typename actor_result<A0, local_tuple_t>::type r0 = a0.eval(args);

        typedef tuple<
            typename actor_result<A0, local_tuple_t>::type
        > tuple_t;
        tuple_t local_args(r0);

        local_tuple<tuple_t, TupleLocsT> local_context(local_args, args.locs);
        return local_func_result<
            N, Parent, local_tuple_t>
        ::eval(local_context);
    }

    A0 a0; //  actors
};

namespace impl {

    template <
        int N, int Parent,
        typename T0 = nil_t,
        typename T1 = nil_t,
        typename T2 = nil_t,
        typename T3 = nil_t,
        typename T4 = nil_t
    >
    struct make_local_function_actor {

        typedef local_function_actor<
            N, Parent,
            typename as_actor<T0>::type,
            typename as_actor<T1>::type,
            typename as_actor<T2>::type,
            typename as_actor<T3>::type,
            typename as_actor<T4>::type
        > composite_type;

        typedef actor<composite_type> type;
    };
}



template <int N, int Parent = 0>
struct local_function {

    actor<local_function_actor<N, Parent> >
    operator()() const
    {
        return local_function_actor<N, Parent>();
    }

    template <typename T0>
    typename impl::make_local_function_actor<N, Parent, T0>::type
    operator()(T0 const& _0) const
    {
        return impl::make_local_function_actor<N, Parent, T0>::composite_type(_0);
    }

    template <int PIndex>
    local_function<N, Parent+PIndex>
    parent() const
    {
        return local_function<N, Parent+PIndex>();
    }
};

//////////////////////////////////
namespace locals {

    local_function<1> const lfun1     = local_function<1>();
    local_function<2> const lfun2     = local_function<2>();
    local_function<3> const lfun3     = local_function<3>();
    local_function<4> const lfun4     = local_function<4>();
}






///////////////////////////////////////////////////////////////////////////////
//
//  context_composite
//
//      This class encapsulates an actor and some local variable
//      initializers packed in a tuple.
//
//      context_composite is just like a proxy and delegates the actual
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
struct context_composite {

    typedef context_composite<ActorT, LocsT> self_t;

    template <typename TupleT>
    struct result { typedef typename tuple_element<0, LocsT>::type type; };

    context_composite(ActorT const& actor_, LocsT const& locals_)
    :   actor(actor_), locals(locals_) {}

    template <typename TupleT>
    typename tuple_element<0, LocsT>::type
    eval(TupleT const& args) const
    {
        local_tuple<TupleT, LocsT>  local_context(args, locals);
        actor.eval(local_context);
        return local_context.locs[tuple_index<0>()];
    }

    ActorT actor;
    LocsT locals;
};

///////////////////////////////////////////////////////////////////////////////
//
//  context_gen
//
//      At construction time, this class is given some local var-
//      initializers packed in a tuple. We just store this for later.
//      The operator[] of this class creates the actual context_composite
//      given an actor. This is responsible for the construct
//      context<types>[actor].
//
///////////////////////////////////////////////////////////////////////////////
template <typename LocsT>
struct context_gen {

    context_gen(LocsT const& locals_)
    :   locals(locals_) {}

    template <typename ActorT>
    actor<context_composite<typename as_actor<ActorT>::type, LocsT> >
    operator[](ActorT const& actor)
    {
        return context_composite<typename as_actor<ActorT>::type, LocsT>
            (as_actor<ActorT>::convert(actor), locals);
    }

    LocsT locals;
};

///////////////////////////////////////////////////////////////////////////////
//
//    Front end generator functions. These generators are overloaded for
//    1..N local variables. context<T0,... TN>(i0,...iN) generate
//    context_gen objects (see above).
//
///////////////////////////////////////////////////////////////////////////////
template <typename T0>
inline context_gen<tuple<T0> >
context()
{
    typedef tuple<T0> tuple_t;
    return context_gen<tuple_t>(tuple_t(T0()));
}

//////////////////////////////////
template <typename T0, typename T1>
inline context_gen<tuple<T0, T1> >
context(
    T1 const& _1 = T1()
)
{
    typedef tuple<T0, T1> tuple_t;
    return context_gen<tuple_t>(tuple_t(T0(), _1));
}

//////////////////////////////////
template <typename T0, typename T1, typename T2>
inline context_gen<tuple<T0, T1, T2> >
context(
    T1 const& _1 = T1(),
    T2 const& _2 = T2()
)
{
    typedef tuple<T0, T1, T2> tuple_t;
    return context_gen<tuple_t>(tuple_t(T0(), _1, _2));
}

//////////////////////////////////
template <typename T0, typename T1, typename T2, typename T3>
inline context_gen<tuple<T0, T1, T2, T3> >
context(
    T1 const& _1 = T1(),
    T2 const& _2 = T2(),
    T3 const& _3 = T3()
)
{
    typedef tuple<T0, T1, T2, T3> tuple_t;
    return context_gen<tuple_t>(tuple_t(T0(), _1, _2, _3));
}

//////////////////////////////////
template <typename T0, typename T1, typename T2, typename T3, typename T4>
inline context_gen<tuple<T0, T1, T2, T3, T4> >
context(
    T1 const& _1 = T1(),
    T2 const& _2 = T2(),
    T3 const& _3 = T3(),
    T4 const& _4 = T4()
)
{
    typedef tuple<T0, T1, T2, T3, T4> tuple_t;
    return context_gen<tuple_t>(tuple_t(T0(), _1, _2, _3, _4));
}












///////////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////
using namespace std;
using namespace phoenix;
using namespace phoenix::locals;

//////////////////////////////////
int
main()
{
    int _10 = 10;

#ifndef __BORLANDC__

    context<nil_t>
    (
        1000,                   //  lvar1: local int variable
        cout << arg1 << '\n',   //  lfun2: local function w/ 1 argument (arg1)
        lvar1 * 2,              //  lfun3: local function that accesses local variable lvar1
        lfun2(2 * arg1)         //  lfun4: local function that calls local function lfun2
    )
    [
        lfun2(arg1 + 2000),
        lfun2(val(5000) * 2),
        lfun2(lvar1 + lfun3()),
        lfun4(val(55)),
        cout << lvar1 << '\n',
        cout << lfun3() << '\n',
        cout << val("bye bye\n")
    ]

    (_10);

#else   //  Borland does not like local variables w/ local functions
        //  we can have local variables (see sample 7..9) *OR*
        //  local functions (this: sample 10) but not both
        //  Sigh... Borland :-{

    context<nil_t>
    (
        12345,
        cout << arg1 << '\n'
    )
    [
        lfun2(arg1 + 687),
        lfun2(val(9999) * 2),
        cout << val("bye bye\n")
    ]

    (_10);

#endif

    return 0;
}

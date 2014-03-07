
//  (C) Copyright Eric Niebler 2011

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for more information.

//  MACRO:         BOOST_NO_CXX11_DECLTYPE_N3276
//  TITLE:         C++0x decltype v1.1 unavailable
//  DESCRIPTION:   The compiler does not support extensions to C++0x
//                 decltype as described in N3276 and accepted in Madrid,
//                 March 2011:
//                 <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3276.pdf>

namespace boost_no_cxx11_decltype_n3276 {

// A simplified result_of implementation.
// that uses decltype.
template<typename Sig>
struct result_of;

template<typename T>
T& declvar();

// use decltype
template<typename Fun, typename T>
struct result_of<Fun(T)>
{
    typedef decltype(declvar<Fun>()(declvar<T>())) type;
};

template<typename Fun, typename T, typename U>
struct result_of<Fun(T, U)>
{
    typedef decltype(declvar<Fun>()(declvar<T>(), declvar<U>())) type;
};

// simple tuple type
template<typename A0 = void, typename A1 = void, typename A2 = void>
struct tuple;

template<typename A0>
struct tuple<A0, void, void>
{
    A0 a0_;

    tuple(A0 const &a0)
      : a0_(a0)
    {}
};

template<typename A0, typename A1>
struct tuple<A0, A1>
{
    A0 a0_;
    A1 a1_;

    tuple(A0 const &a0, A1 const & a1)
      : a0_(a0)
      , a1_(a1)
    {}
};

// A node in an expression tree
template<class Tag, class Args> // Args is a tuple.
struct Expr;

// A function object that builds expression nodes
template<class Tag>
struct MakeExpr
{
    template<class T>
    Expr<Tag, tuple<T> > operator()(T const & t) const
    {
        return Expr<Tag, tuple<T> >(tuple<T>(t));
    }

    template<class T, typename U>
    Expr<Tag, tuple<T, U> > operator()(T const & t, U const & u) const
    {
        return Expr<Tag, tuple<T, U> >(tuple<T, U>(t, u));
    }
};

// Here are tag types that encode in an expression node
// what operation created the node.
struct Terminal;
struct BinaryPlus;
struct FunctionCall;

typedef MakeExpr<Terminal> MakeTerminal;
typedef MakeExpr<BinaryPlus> MakeBinaryPlus;
typedef MakeExpr<FunctionCall> MakeFunctionCall;

template<class Tag, class Args>
struct Expr
{
    Args args_;

    explicit Expr(Args const & t) : args_(t) {}

    // An overloaded operator+ that creates a binary plus node
    template<typename RTag, typename RArgs>
    typename result_of<MakeBinaryPlus(Expr, Expr<RTag, RArgs>)>::type
    operator+(Expr<RTag, RArgs> const &right) const
    {
        return MakeBinaryPlus()(*this, right);
    }

    // An overloaded function call operator that creates a unary
    // function call node
    typename result_of<MakeFunctionCall(Expr)>::type
    operator()() const
    {
        return MakeFunctionCall()(*this);
    }
};

int test()
{
    // This is a terminal in an expression tree
    Expr<Terminal, tuple<int> > i = MakeTerminal()(42);

    i + i; // OK, this creates a binary plus node.

    i(); // OK, this creates a unary function-call node.
         // NOTE: If N3276 has not been implemented, this
         // line will set off an infinite cascade of template
         // instantiations that will run the compiler out of
         // memory.

    return 0;
}

}

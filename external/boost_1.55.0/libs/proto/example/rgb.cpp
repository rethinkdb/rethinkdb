//[ RGB
///////////////////////////////////////////////////////////////////////////////
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This is a simple example of doing arbitrary type manipulations with proto
// transforms. It takes some expression involving primary colors and combines
// the colors according to arbitrary rules. It is a port of the RGB example
// from PETE (http://www.codesourcery.com/pooma/download.html).

#include <iostream>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
namespace proto = boost::proto;

struct RedTag
{
    friend std::ostream &operator <<(std::ostream &sout, RedTag)
    {
        return sout << "This expression is red.";
    }
};

struct BlueTag
{
    friend std::ostream &operator <<(std::ostream &sout, BlueTag)
    {
        return sout << "This expression is blue.";
    }
};

struct GreenTag
{
    friend std::ostream &operator <<(std::ostream &sout, GreenTag)
    {
        return sout << "This expression is green.";
    }
};

typedef proto::terminal<RedTag>::type RedT;
typedef proto::terminal<BlueTag>::type BlueT;
typedef proto::terminal<GreenTag>::type GreenT;

struct Red;
struct Blue;
struct Green;

///////////////////////////////////////////////////////////////////////////////
// A transform that produces new colors according to some arbitrary rules:
// red & green give blue, red & blue give green, blue and green give red.
struct Red
  : proto::or_<
        proto::plus<Green, Blue>
      , proto::plus<Blue, Green>
      , proto::plus<Red, Red>
      , proto::terminal<RedTag>
    >
{};

struct Green
  : proto::or_<
        proto::plus<Red, Blue>
      , proto::plus<Blue, Red>
      , proto::plus<Green, Green>
      , proto::terminal<GreenTag>
    >
{};

struct Blue
  : proto::or_<
        proto::plus<Red, Green>
      , proto::plus<Green, Red>
      , proto::plus<Blue, Blue>
      , proto::terminal<BlueTag>
    >
{};

struct RGB
  : proto::or_<
        proto::when< Red, RedTag() >
      , proto::when< Blue, BlueTag() >
      , proto::when< Green, GreenTag() >
    >
{};

template<typename Expr>
void printColor(Expr const & expr)
{
    int i = 0; // dummy state and data parameter, not used
    std::cout << RGB()(expr, i, i) << std::endl;
}

int main()
{
    printColor(RedT() + GreenT());
    printColor(RedT() + GreenT() + BlueT());
    printColor(RedT() + (GreenT() + BlueT()));

    return 0;
}
//]

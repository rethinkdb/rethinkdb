//[ HelloWorld
////////////////////////////////////////////////////////////////////
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
// This #include is only needed for compilers that use typeof emulation:
#include <boost/typeof/std/ostream.hpp>
namespace proto = boost::proto;

proto::terminal< std::ostream & >::type cout_ = {std::cout};

template< typename Expr >
void evaluate( Expr const & expr )
{
    proto::default_context ctx;
    proto::eval(expr, ctx);
}

int main()
{
    evaluate( cout_ << "hello" << ',' << " world" );
    return 0;
}
//]

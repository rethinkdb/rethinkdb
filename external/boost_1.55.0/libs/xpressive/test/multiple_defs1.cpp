///////////////////////////////////////////////////////////////////////////////
// multiple_defs1.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/xpressive/xpressive.hpp>
extern int f();
int main()
{
    using namespace boost::xpressive;
    sregex srx = +_;
    sregex drx = sregex::compile(".+");
    return f();
}

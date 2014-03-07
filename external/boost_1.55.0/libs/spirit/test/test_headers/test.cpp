// Copyright (c) 2003-2008 Matthias Christian Schabel
// Copyright (c) 2007-2008 Steven Watanabe
// Copyright (c) 2010 Joel de Guzman
// Copyright (c) 2010 Hartmut Kaiser
// Copyright (c) 2009 Francois Barel
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_SPIRIT_STRINGIZE_IMPL(x) #x
#define BOOST_SPIRIT_STRINGIZE(x) BOOST_SPIRIT_STRINGIZE_IMPL(x)

#define BOOST_SPIRIT_HEADER BOOST_SPIRIT_STRINGIZE(BOOST_SPIRIT_HEADER_NAME)

#include BOOST_SPIRIT_HEADER
#include BOOST_SPIRIT_HEADER

int main() 
{
    return 0;
}

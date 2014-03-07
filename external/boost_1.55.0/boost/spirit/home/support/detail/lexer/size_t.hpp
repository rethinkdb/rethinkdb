// size_t.h
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_LEXER_SIZE_T_H
#define BOOST_LEXER_SIZE_T_H

#include <stddef.h> // ptrdiff_t

#if defined _MSC_VER && _MSC_VER <= 1200
namespace std
{
    using ::ptrdiff_t;
    using ::size_t;
}
#else
#include <string>
#endif

#endif

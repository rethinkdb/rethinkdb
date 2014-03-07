/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_LEXER_CONFIG_HPP)
#define BOOST_SPIRIT_CONJURE_LEXER_CONFIG_HPP

///////////////////////////////////////////////////////////////////////////////
// The conjure lexer example can be built in 3 different variations:
//
//    - With a lexer using runtime generated DFA tables
//    - With a lexer using pre-generated (static) DFA tables
//    - With a lexer using a pre-generated custom switch based state machine
//
// Use one of the following preprocessor constants to define, which of those 
// will be built:

///////////////////////////////////////////////////////////////////////////////
// Use the lexer based on runtime generated DFA tables
// #define CONJURE_LEXER_DYNAMIC_TABLES 1

///////////////////////////////////////////////////////////////////////////////
// Use the lexer based on pre-generated static DFA tables
// #define CONJURE_LEXER_STATIC_TABLES 1

///////////////////////////////////////////////////////////////////////////////
// Use the lexer based on runtime generated DFA tables
// #define CONJURE_LEXER_STATIC_SWITCH 1

///////////////////////////////////////////////////////////////////////////////
// The default is to use the dynamic table driven lexer
#if CONJURE_LEXER_DYNAMIC_TABLES == 0 && \
    CONJURE_LEXER_STATIC_TABLES == 0 && \
    CONJURE_LEXER_STATIC_SWITCH == 0

#define CONJURE_LEXER_DYNAMIC_TABLES 1
#endif

///////////////////////////////////////////////////////////////////////////////
// Make sure we have only one lexer type selected
#if (CONJURE_LEXER_DYNAMIC_TABLES != 0 && CONJURE_LEXER_STATIC_TABLES != 0) || \
    (CONJURE_LEXER_DYNAMIC_TABLES != 0 && CONJURE_LEXER_STATIC_SWITCH != 0) || \
    (CONJURE_LEXER_STATIC_TABLES != 0 && CONJURE_LEXER_STATIC_SWITCH != 0)

#error "Configuration problem: please select exactly one type of lexer to build"
#endif

#endif


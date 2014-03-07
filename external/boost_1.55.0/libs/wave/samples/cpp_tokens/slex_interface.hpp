/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the abstract lexer interface
    
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(SLEX_INTERFACE_HPP_E83F52A4_90AC_4FBE_A9A7_B65F7F94C497_INCLUDED)
#define SLEX_INTERFACE_HPP_E83F52A4_90AC_4FBE_A9A7_B65F7F94C497_INCLUDED

#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/cpplexer/cpp_lex_interface.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace slex {

#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION != 0
#define BOOST_WAVE_NEW_LEXER_DECL BOOST_WAVE_DECL
#else
#define BOOST_WAVE_NEW_LEXER_DECL
#endif 

///////////////////////////////////////////////////////////////////////////////
//  
//  new_lexer_gen: generates a new instance of the required C++ lexer
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename IteratorT, 
    typename PositionT = boost::wave::util::file_position_type
>
struct BOOST_WAVE_NEW_LEXER_DECL new_lexer_gen
{
//  The NewLexer function allows the opaque generation of a new lexer object.
//  It is coupled to the token type to allow to decouple the lexer/token 
//  configurations at compile time.
    static lex_input_interface<slex_token<PositionT> > *
    new_lexer(IteratorT const &first, IteratorT const &last, 
        PositionT const &pos, boost::wave::language_support language);
};

#undef BOOST_WAVE_NEW_LEXER_DECL

///////////////////////////////////////////////////////////////////////////////
//
//  The slex_input_interface helps to instantiate a concrete lexer to be used 
//  by the Wave preprocessor module.
//  This is done to allow compile time reduction.
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT>
struct slex_input_interface 
:   lex_input_interface<TokenT>
{
    typedef typename lex_input_interface<TokenT>::position_type position_type;

    ~slex_input_interface() {}

//  The new_lexer function allows the opaque generation of a new lexer object.
//  It is coupled to the token type to allow to distinguish different 
//  lexer/token configurations at compile time.
    template <typename IteratorT>
    static lex_input_interface<TokenT> *
    new_lexer(IteratorT const &first, IteratorT const &last, 
        position_type const &pos, boost::wave::language_support language)
    { 
        return new_lexer_gen<IteratorT, position_type>::new_lexer (first, last, 
            pos, language); 
    }
};

///////////////////////////////////////////////////////////////////////////////
}   // namespace slex
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost 

#endif // !defined(SLEX_INTERFACE_HPP_E83F52A4_90AC_4FBE_A9A7_B65F7F94C497_INCLUDED)

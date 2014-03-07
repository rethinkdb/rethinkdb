/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Definition of the abstract lexer interface for the lexertl based C++ lexer
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_LEXERTL_INTERFACE_HPP_INCLUDED)
#define BOOST_WAVE_LEXERTL_INTERFACE_HPP_INCLUDED

#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/cpplexer/cpp_lex_interface.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace wave { namespace cpplexer { namespace lexertl 
{

///////////////////////////////////////////////////////////////////////////////
//  
//  new_lexer_gen: generates a new instance of the required C++ lexer
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename IteratorT, 
    typename PositionT = wave::util::file_position_type
>
struct new_lexer_gen
{
//  The NewLexer function allows the opaque generation of a new lexer object.
//  It is coupled to the token type to allow to decouple the lexer/token 
//  configurations at compile time.
    static wave::cpplexer::lex_input_interface<
        wave::cpplexer::lex_token<PositionT> 
    > *
    new_lexer(IteratorT const &first, IteratorT const &last, 
        PositionT const &pos, wave::language_support language);
};

///////////////////////////////////////////////////////////////////////////////
//
//  The lexertl_input_interface helps to instantiate a concrete lexer
//  to be used by the Wave preprocessor module.
//  This is done to allow compile time reduction by separation of the lexer 
//  iterator exposed to the Wave library and the lexer implementation.
//
///////////////////////////////////////////////////////////////////////////////

template <typename TokenT>
struct lexertl_input_interface 
:   wave::cpplexer::lex_input_interface<TokenT>
{
    typedef typename wave::cpplexer::lex_input_interface<TokenT>::position_type 
        position_type;
    
//  The new_lexer function allows the opaque generation of a new lexer object.
//  It is coupled to the token type to allow to distinguish different 
//  lexer/token configurations at compile time.
    template <typename IteratorT>
    static wave::cpplexer::lex_input_interface<TokenT> *
    new_lexer(IteratorT const &first, IteratorT const &last, 
        position_type const &pos, wave::language_support language)
    { 
        return new_lexer_gen<IteratorT, position_type>::new_lexer (first, last, 
            pos, language); 
    }
};

///////////////////////////////////////////////////////////////////////////////
}}}}   // namespace boost::wave::cpplexer::lexertl

#endif // !defined(BOOST_WAVE_LEXERTL_INTERFACE_HPP_INCLUDED)

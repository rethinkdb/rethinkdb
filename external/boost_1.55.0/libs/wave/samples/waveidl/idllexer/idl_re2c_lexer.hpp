/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Re2C based IDL lexer
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(IDL_RE2C_LEXER_HPP_B81A2629_D5B1_4944_A97D_60254182B9A8_INCLUDED)
#define IDL_RE2C_LEXER_HPP_B81A2629_D5B1_4944_A97D_60254182B9A8_INCLUDED

#include <string>
#include <cstdio>
#include <cstdarg>
#if defined(BOOST_SPIRIT_DEBUG)
#include <iostream>
#endif // defined(BOOST_SPIRIT_DEBUG)

#include <boost/concept_check.hpp>
#include <boost/assert.hpp>
#include <boost/spirit/include/classic_core.hpp>

#include <boost/wave/token_ids.hpp>
#include <boost/wave/language_support.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/cpplexer/validate_universal_char.hpp>
#include <boost/wave/cpplexer/cpplexer_exceptions.hpp>

// reuse the default token type and re2c lexer helpers
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_interface.hpp>
#include <boost/wave/cpplexer/re2clex/scanner.hpp>

#include "idl_re.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace idllexer {
namespace re2clex {

///////////////////////////////////////////////////////////////////////////////
// 
//  encapsulation of the re2c based idl lexer
//
///////////////////////////////////////////////////////////////////////////////

template <
    typename IteratorT, 
    typename PositionT = boost::wave::util::file_position_type
>
class lexer 
{
    typedef boost::wave::cpplexer::re2clex::Scanner scanner_t;

public:

    typedef char                                        char_t;
    typedef boost::wave::cpplexer::re2clex::Scanner     base_t;
    typedef boost::wave::cpplexer::lex_token<PositionT> token_type;
    typedef typename token_type::string_type            string_type;
    
    lexer(IteratorT const &first, IteratorT const &last, 
        PositionT const &pos, boost::wave::language_support language);
    ~lexer();

    token_type& get(token_type& t);
    void set_position(PositionT const &pos)
    {
        // set position has to change the file name and line number only
        filename = pos.get_file();
        scanner.line = pos.get_line();
        scanner.file_name = filename.c_str();
    }

// error reporting from the re2c generated lexer
    static int report_error(scanner_t const *s, int code, char const *, ...);

private:
    static char const *tok_names[];

    scanner_t scanner;
    string_type filename;
    bool at_eof;
    boost::wave::language_support language;
};

///////////////////////////////////////////////////////////////////////////////
// initialize cpp lexer 
template <typename IteratorT, typename PositionT>
inline
lexer<IteratorT, PositionT>::lexer(IteratorT const &first, 
        IteratorT const &last, PositionT const &pos, 
        boost::wave::language_support language) 
:   filename(pos.get_file()), at_eof(false), language(language)
{
    using namespace std;        // some systems have memset in std
    using namespace boost::wave::cpplexer::re2clex;

    memset(&scanner, '\0', sizeof(scanner_t));
    scanner.eol_offsets = aq_create();
    scanner.first = scanner.act = (uchar *)&(*first);
    scanner.last = scanner.first + std::distance(first, last);
    scanner.line = pos.get_line();
    scanner.error_proc = report_error;
    scanner.file_name = filename.c_str();

// not used by the lexer
    scanner.enable_ms_extensions = 0;
    scanner.act_in_c99_mode = 0;

    boost::ignore_unused_variable_warning(language);
}

template <typename IteratorT, typename PositionT>
inline
lexer<IteratorT, PositionT>::~lexer() 
{
    boost::wave::cpplexer::re2clex::aq_terminate(scanner.eol_offsets);
    free(scanner.bot);
}

///////////////////////////////////////////////////////////////////////////////
//  get the next token from the input stream
template <typename IteratorT, typename PositionT>
inline boost::wave::cpplexer::lex_token<PositionT>&
lexer<IteratorT, PositionT>::get(boost::wave::cpplexer::lex_token<PositionT>& t)
{
    using namespace boost::wave;    // to import token ids to this scope

    if (at_eof) 
        return t = boost::wave::cpplexer::lex_token<PositionT>();  // return T_EOI

    token_id id = token_id(scan(&scanner));
    string_type value((char const *)scanner.tok, scanner.cur-scanner.tok);

    if (T_IDENTIFIER == id) {
    // test identifier characters for validity (throws if invalid chars found)
        if (!boost::wave::need_no_character_validation(language)) {
            boost::wave::cpplexer::impl::validate_identifier_name(value, 
                scanner.line, -1, filename); 
        }
    }
    else if (T_STRINGLIT == id || T_CHARLIT == id) {
    // test literal characters for validity (throws if invalid chars found)
        if (!boost::wave::need_no_character_validation(language)) {
            boost::wave::cpplexer::impl::validate_literal(value, scanner.line, 
                -1, filename); 
        }
    }
    else if (T_EOF == id) {
    // T_EOF is returned as a valid token, the next call will return T_EOI,
    // i.e. the actual end of input
        at_eof = true;
        value.clear();
    }
    return t = boost::wave::cpplexer::lex_token<PositionT>(id, value, 
        PositionT(filename, scanner.line, -1));
}

template <typename IteratorT, typename PositionT>
inline int 
lexer<IteratorT, PositionT>::report_error(scanner_t const *s, int errcode,
    char const* msg, ...)
{
    BOOST_ASSERT(0 != s);
    BOOST_ASSERT(0 != msg);

    using namespace std;    // some system have vsprintf in namespace std
    
    char buffer[200];           // should be large enough
    va_list params;
    va_start(params, msg);
    vsprintf(buffer, msg, params);
    va_end(params);
    
    BOOST_WAVE_LEXER_THROW_VAR(boost::wave::cpplexer::lexing_exception, 
        errcode, buffer, s->line, -1, s->file_name);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//   
//  lex_functor
//   
///////////////////////////////////////////////////////////////////////////////
     
template <
    typename IteratorT, 
    typename PositionT = boost::wave::util::file_position_type
>
class lex_functor 
:   public lex_input_interface_generator<
        typename lexer<IteratorT, PositionT>::token_type
    >
{
public:

    typedef typename lexer<IteratorT, PositionT>::token_type   token_type;
    
    lex_functor(IteratorT const &first, IteratorT const &last, 
            PositionT const &pos, boost::wave::language_support language)
    :   lexer(first, last, pos, language)
    {}
    virtual ~lex_functor() {}

// get the next token from the input stream
    token_type& get(token_type& t) { return lexer.get(t); }
    void set_position(PositionT const &pos) 
    { lexer.set_position(pos); }
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    bool has_include_guards(std::string&) const { return false; }
#endif    

private:
    lexer<IteratorT, PositionT> lexer;
};

}   // namespace re2clex

///////////////////////////////////////////////////////////////////////////////
//  
//  The new_lexer_gen<>::new_lexer function (declared in cpp_slex_token.hpp)
//  should be defined inline, if the lex_functor shouldn't be instantiated 
//  separately from the lex_iterator.
//
//  Separate (explicit) instantiation helps to reduce compilation time.
//
///////////////////////////////////////////////////////////////////////////////

#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION != 0
#define BOOST_WAVE_RE2C_NEW_LEXER_INLINE
#else
#define BOOST_WAVE_RE2C_NEW_LEXER_INLINE inline
#endif 

///////////////////////////////////////////////////////////////////////////////
//
//  The 'new_lexer' function allows the opaque generation of a new lexer object.
//  It is coupled to the iterator type to allow to decouple the lexer/iterator 
//  configurations at compile time.
//
//  This function is declared inside the cpp_slex_token.hpp file, which is 
//  referenced by the source file calling the lexer and the source file, which
//  instantiates the lex_functor. But it is defined here, so it will be 
//  instantiated only while compiling the source file, which instantiates the 
//  lex_functor. While the cpp_re2c_token.hpp file may be included everywhere,
//  this file (cpp_re2c_lexer.hpp) should be included only once. This allows
//  to decouple the lexer interface from the lexer implementation and reduces 
//  compilation time.
//
///////////////////////////////////////////////////////////////////////////////

template <typename IteratorT, typename PositionT>
BOOST_WAVE_RE2C_NEW_LEXER_INLINE
cpplexer::lex_input_interface<cpplexer::lex_token<PositionT> > *
new_lexer_gen<IteratorT, PositionT>::new_lexer(IteratorT const &first,
    IteratorT const &last, PositionT const &pos, 
    wave::language_support language)
{
    return new re2clex::lex_functor<IteratorT, PositionT>(first, last, pos,
        language);
}

#undef BOOST_WAVE_RE2C_NEW_LEXER_INLINE

///////////////////////////////////////////////////////////////////////////////
}   // namespace idllexer
}   // namespace wave
}   // namespace boost
     
#endif // !defined(IDL_RE2C_LEXER_HPP_B81A2629_D5B1_4944_A97D_60254182B9A8_INCLUDED)

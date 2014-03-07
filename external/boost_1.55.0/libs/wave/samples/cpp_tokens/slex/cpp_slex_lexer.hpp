/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    SLex (Spirit Lex) based C++ lexer
    
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(SLEX_LEXER_HPP_5E8E1DF0_BB41_4938_B7E5_A4BB68222FF6_INCLUDED)
#define SLEX_LEXER_HPP_5E8E1DF0_BB41_4938_B7E5_A4BB68222FF6_INCLUDED

#include <string>
#if defined(BOOST_SPIRIT_DEBUG)
#include <iostream>
#endif // defined(BOOST_SPIRIT_DEBUG)

#include <boost/assert.hpp>
#include <boost/spirit/include/classic_core.hpp>

#include <boost/wave/wave_config.hpp>
#include <boost/wave/language_support.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/wave/util/file_position.hpp>
#include <boost/wave/util/time_conversion_helper.hpp>
#include <boost/wave/cpplexer/validate_universal_char.hpp>
#include <boost/wave/cpplexer/convert_trigraphs.hpp>
#include <boost/wave/cpplexer/cpplexer_exceptions.hpp>
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
#include <boost/wave/cpplexer/detect_include_guards.hpp>
#endif
#include <boost/wave/cpplexer/cpp_lex_interface.hpp>

#include "../slex_interface.hpp"
#include "../slex_token.hpp"
#include "../slex_iterator.hpp"

#include "lexer.hpp"   // "spirit/lexer.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace cpplexer {
namespace slex {
namespace lexer {

///////////////////////////////////////////////////////////////////////////////
//  The following numbers are the array sizes of the token regex's which we
//  need to specify to make the CW compiler happy (at least up to V9.5).
#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS != 0
#define INIT_DATA_SIZE              175
#else
#define INIT_DATA_SIZE              158
#endif
#define INIT_DATA_CPP_SIZE          15
#define INIT_DATA_PP_NUMBER_SIZE    2
#define INIT_DATA_CPP0X_SIZE        15

///////////////////////////////////////////////////////////////////////////////
// 
//  encapsulation of the boost::spirit::classic::slex based cpp lexer
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//  The following lexer_base class was necessary to workaround a CodeWarrior 
//  bug (at least up to CW V9.5).
template <typename IteratorT, typename PositionT>
class lexer_base 
:   public boost::spirit::classic::lexer<
        boost::wave::util::position_iterator<IteratorT, PositionT> >
{
protected:
    typedef boost::wave::util::position_iterator<IteratorT, PositionT> 
        iterator_type;
    typedef typename std::iterator_traits<IteratorT>::value_type  char_type;
    typedef boost::spirit::classic::lexer<iterator_type> base_type;

    lexer_base();

// initialization data (regular expressions for the token definitions)
    struct lexer_data {
        token_id tokenid;                       // token data
        char_type const *tokenregex;            // associated token to match
        typename base_type::callback_t tokencb; // associated callback function
        unsigned int lexerstate;                // valid for lexer state
    };
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorT, typename PositionT>
class lexer 
:   public lexer_base<IteratorT, PositionT>
{
public:
    typedef boost::wave::cpplexer::slex_token<PositionT>  token_type;
    
    void init_dfa(boost::wave::language_support language);

// get time of last compilation
    static std::time_t get_compilation_time() 
        { return compilation_time.get_time(); }

// helper for calculation of the time of last compilation
    static boost::wave::util::time_conversion_helper compilation_time;

private:
    typedef lexer_base<IteratorT, PositionT> base_type;

    static typename base_type::lexer_data const init_data[INIT_DATA_SIZE];          // common patterns
    static typename base_type::lexer_data const init_data_cpp[INIT_DATA_CPP_SIZE];  // C++ only patterns
    static typename base_type::lexer_data const init_data_pp_number[INIT_DATA_PP_NUMBER_SIZE];  // pp-number only patterns
    static typename base_type::lexer_data const init_data_cpp0x[INIT_DATA_CPP0X_SIZE];  // C++0X only patterns
};

///////////////////////////////////////////////////////////////////////////////
//  data required for initialization of the lexer (token definitions)
#define OR                  "|"
#define Q(c)                "\\" c
#define TRI(c)              Q("?") Q("?") c

// definition of some sub-token regexps to simplify the regex definitions
#define BLANK               "[ \\t]"
#define CCOMMENT            \
    Q("/") Q("*") "[^*]*" Q("*") "+" "(" "[^/*][^*]*" Q("*") "+" ")*" Q("/")
        
#define PPSPACE             "(" BLANK OR CCOMMENT ")*"

#define OCTALDIGIT          "[0-7]"
#define DIGIT               "[0-9]"
#define HEXDIGIT            "[0-9a-fA-F]"
#define OPTSIGN             "[-+]?"
#define EXPSTART            "[eE]" "[-+]"
#define EXPONENT            "(" "[eE]" OPTSIGN "[0-9]+" ")"
#define NONDIGIT            "[a-zA-Z_]"

#define INTEGER             \
    "(" "(0x|0X)" HEXDIGIT "+" OR "0" OCTALDIGIT "*" OR "[1-9]" DIGIT "*" ")"
            
#define INTEGER_SUFFIX      "(" "[uU][lL]?|[lL][uU]?" ")"
#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS != 0
#define LONGINTEGER_SUFFIX  "(" "[uU]" "(" "[lL][lL]" ")" OR \
                                "(" "[lL][lL]" ")" "[uU]" "?" OR \
                                "i64" \
                            ")" 
#else
#define LONGINTEGER_SUFFIX  "(" "[uU]" "(" "[lL][lL]" ")" OR \
                            "(" "[lL][lL]" ")" "[uU]" "?" ")"
#endif
#define FLOAT_SUFFIX        "(" "[fF][lL]?" OR "[lL][fF]?" ")"
#define CHAR_SPEC           "L?"
#define EXTCHAR_SPEC        "(" "[uU]" OR "u8" ")"

#define BACKSLASH           "(" Q("\\") OR TRI(Q("/")) ")"
#define ESCAPESEQ           "(" BACKSLASH "(" \
                                "[abfnrtv?'\"]" OR \
                                BACKSLASH OR \
                                "x" HEXDIGIT "+" OR \
                                OCTALDIGIT OCTALDIGIT "?" OCTALDIGIT "?" \
                            "))"
#define HEXQUAD             "(" HEXDIGIT HEXDIGIT HEXDIGIT HEXDIGIT ")"
#define UNIVERSALCHAR       "(" BACKSLASH "(" \
                                "u" HEXQUAD OR \
                                "U" HEXQUAD HEXQUAD \
                            "))" 

#define POUNDDEF            "(" "#" OR TRI("=") OR Q("%:") ")"
#define NEWLINEDEF          "(" "\n" OR "\r" OR "\r\n" ")"

#if BOOST_WAVE_SUPPORT_INCLUDE_NEXT != 0
#define INCLUDEDEF          "(include|include_next)"
#else
#define INCLUDEDEF          "include"
#endif

#define PP_NUMBERDEF        Q(".") "?" DIGIT "(" DIGIT OR NONDIGIT OR EXPSTART OR Q(".") ")*"

///////////////////////////////////////////////////////////////////////////////
//  lexer state constants
#define LEXER_STATE_NORMAL  0
#define LEXER_STATE_PP      1

#define NUM_LEXER_STATES    1

//  helper for initializing token data
#define TOKEN_DATA(id, regex)                                                 \
        { T_##id, regex, 0, LEXER_STATE_NORMAL }                              \
    /**/

#define TOKEN_DATA_EX(id, regex, callback)                                    \
        { T_##id, regex, callback, LEXER_STATE_NORMAL }                       \
    /**/

///////////////////////////////////////////////////////////////////////////////
// common C++/C99 token definitions
template <typename IteratorT, typename PositionT>
typename lexer_base<IteratorT, PositionT>::lexer_data const 
lexer<IteratorT, PositionT>::init_data[INIT_DATA_SIZE] = 
{
    TOKEN_DATA(AND, "&"),
    TOKEN_DATA(ANDAND, "&&"),
    TOKEN_DATA(ASSIGN, "="),
    TOKEN_DATA(ANDASSIGN, "&="),
    TOKEN_DATA(OR, Q("|")),
    TOKEN_DATA(OR_TRIGRAPH, TRI("!")),
    TOKEN_DATA(ORASSIGN, Q("|=")),
    TOKEN_DATA(ORASSIGN_TRIGRAPH, TRI("!=")),
    TOKEN_DATA(XOR, Q("^")),
    TOKEN_DATA(XOR_TRIGRAPH, TRI("'")),
    TOKEN_DATA(XORASSIGN, Q("^=")),
    TOKEN_DATA(XORASSIGN_TRIGRAPH, TRI("'=")),
    TOKEN_DATA(COMMA, ","),
    TOKEN_DATA(COLON, ":"),
    TOKEN_DATA(DIVIDEASSIGN, Q("/=")),
    TOKEN_DATA(DIVIDE, Q("/")),
    TOKEN_DATA(DOT, Q(".")),
    TOKEN_DATA(ELLIPSIS, Q(".") Q(".") Q(".")),
    TOKEN_DATA(EQUAL, "=="),
    TOKEN_DATA(GREATER, ">"),
    TOKEN_DATA(GREATEREQUAL, ">="),
    TOKEN_DATA(LEFTBRACE, Q("{")),
    TOKEN_DATA(LEFTBRACE_ALT, "<" Q("%")),
    TOKEN_DATA(LEFTBRACE_TRIGRAPH, TRI("<")),
    TOKEN_DATA(LESS, "<"),
    TOKEN_DATA(LESSEQUAL, "<="),
    TOKEN_DATA(LEFTPAREN, Q("(")),
    TOKEN_DATA(LEFTBRACKET, Q("[")),
    TOKEN_DATA(LEFTBRACKET_ALT, "<:"),
    TOKEN_DATA(LEFTBRACKET_TRIGRAPH, TRI(Q("("))),
    TOKEN_DATA(MINUS, Q("-")),
    TOKEN_DATA(MINUSASSIGN, Q("-=")),
    TOKEN_DATA(MINUSMINUS, Q("-") Q("-")),
    TOKEN_DATA(PERCENT, Q("%")),
    TOKEN_DATA(PERCENTASSIGN, Q("%=")),
    TOKEN_DATA(NOT, "!"),
    TOKEN_DATA(NOTEQUAL, "!="),
    TOKEN_DATA(OROR, Q("|") Q("|")),
    TOKEN_DATA(OROR_TRIGRAPH, TRI("!") Q("|") OR Q("|") TRI("!") OR TRI("!") TRI("!")),
    TOKEN_DATA(PLUS, Q("+")),
    TOKEN_DATA(PLUSASSIGN, Q("+=")),
    TOKEN_DATA(PLUSPLUS, Q("+") Q("+")),
    TOKEN_DATA(ARROW, Q("->")),
    TOKEN_DATA(QUESTION_MARK, Q("?")),
    TOKEN_DATA(RIGHTBRACE, Q("}")),
    TOKEN_DATA(RIGHTBRACE_ALT, Q("%>")),
    TOKEN_DATA(RIGHTBRACE_TRIGRAPH, TRI(">")),
    TOKEN_DATA(RIGHTPAREN, Q(")")),
    TOKEN_DATA(RIGHTBRACKET, Q("]")),
    TOKEN_DATA(RIGHTBRACKET_ALT, ":>"),
    TOKEN_DATA(RIGHTBRACKET_TRIGRAPH, TRI(Q(")"))),
    TOKEN_DATA(SEMICOLON, ";"),
    TOKEN_DATA(SHIFTLEFT, "<<"),
    TOKEN_DATA(SHIFTLEFTASSIGN, "<<="),
    TOKEN_DATA(SHIFTRIGHT, ">>"),
    TOKEN_DATA(SHIFTRIGHTASSIGN, ">>="),
    TOKEN_DATA(STAR, Q("*")),
    TOKEN_DATA(COMPL, Q("~")),
    TOKEN_DATA(COMPL_TRIGRAPH, TRI("-")),
    TOKEN_DATA(STARASSIGN, Q("*=")),
    TOKEN_DATA(ASM, "asm"),
    TOKEN_DATA(AUTO, "auto"),
    TOKEN_DATA(BOOL, "bool"),
    TOKEN_DATA(FALSE, "false"),
    TOKEN_DATA(TRUE, "true"),
    TOKEN_DATA(BREAK, "break"),
    TOKEN_DATA(CASE, "case"),
    TOKEN_DATA(CATCH, "catch"),
    TOKEN_DATA(CHAR, "char"),
    TOKEN_DATA(CLASS, "class"),
    TOKEN_DATA(CONST, "const"),
    TOKEN_DATA(CONSTCAST, "const_cast"),
    TOKEN_DATA(CONTINUE, "continue"),
    TOKEN_DATA(DEFAULT, "default"),
    TOKEN_DATA(DELETE, "delete"),
    TOKEN_DATA(DO, "do"),
    TOKEN_DATA(DOUBLE, "double"),
    TOKEN_DATA(DYNAMICCAST, "dynamic_cast"),
    TOKEN_DATA(ELSE, "else"),
    TOKEN_DATA(ENUM, "enum"),
    TOKEN_DATA(EXPLICIT, "explicit"),
    TOKEN_DATA(EXPORT, "export"),
    TOKEN_DATA(EXTERN, "extern"),
    TOKEN_DATA(FLOAT, "float"),
    TOKEN_DATA(FOR, "for"),
    TOKEN_DATA(FRIEND, "friend"),
    TOKEN_DATA(GOTO, "goto"),
    TOKEN_DATA(IF, "if"),
    TOKEN_DATA(INLINE, "inline"),
    TOKEN_DATA(INT, "int"),
    TOKEN_DATA(LONG, "long"),
    TOKEN_DATA(MUTABLE, "mutable"),
    TOKEN_DATA(NAMESPACE, "namespace"),
    TOKEN_DATA(NEW, "new"),
    TOKEN_DATA(OPERATOR, "operator"),
    TOKEN_DATA(PRIVATE, "private"),
    TOKEN_DATA(PROTECTED, "protected"),
    TOKEN_DATA(PUBLIC, "public"),
    TOKEN_DATA(REGISTER, "register"),
    TOKEN_DATA(REINTERPRETCAST, "reinterpret_cast"),
    TOKEN_DATA(RETURN, "return"),
    TOKEN_DATA(SHORT, "short"),
    TOKEN_DATA(SIGNED, "signed"),
    TOKEN_DATA(SIZEOF, "sizeof"),
    TOKEN_DATA(STATIC, "static"),
    TOKEN_DATA(STATICCAST, "static_cast"),
    TOKEN_DATA(STRUCT, "struct"),
    TOKEN_DATA(SWITCH, "switch"),
    TOKEN_DATA(TEMPLATE, "template"),
    TOKEN_DATA(THIS, "this"),
    TOKEN_DATA(THROW, "throw"),
    TOKEN_DATA(TRY, "try"),
    TOKEN_DATA(TYPEDEF, "typedef"),
    TOKEN_DATA(TYPEID, "typeid"),
    TOKEN_DATA(TYPENAME, "typename"),
    TOKEN_DATA(UNION, "union"),
    TOKEN_DATA(UNSIGNED, "unsigned"),
    TOKEN_DATA(USING, "using"),
    TOKEN_DATA(VIRTUAL, "virtual"),
    TOKEN_DATA(VOID, "void"),
    TOKEN_DATA(VOLATILE, "volatile"),
    TOKEN_DATA(WCHART, "wchar_t"),
    TOKEN_DATA(WHILE, "while"),
    TOKEN_DATA(PP_DEFINE, POUNDDEF PPSPACE "define"),
    TOKEN_DATA(PP_IF, POUNDDEF PPSPACE "if"),
    TOKEN_DATA(PP_IFDEF, POUNDDEF PPSPACE "ifdef"),
    TOKEN_DATA(PP_IFNDEF, POUNDDEF PPSPACE "ifndef"),
    TOKEN_DATA(PP_ELSE, POUNDDEF PPSPACE "else"),
    TOKEN_DATA(PP_ELIF, POUNDDEF PPSPACE "elif"),
    TOKEN_DATA(PP_ENDIF, POUNDDEF PPSPACE "endif"),
    TOKEN_DATA(PP_ERROR, POUNDDEF PPSPACE "error"),
    TOKEN_DATA(PP_QHEADER, POUNDDEF PPSPACE \
        INCLUDEDEF PPSPACE Q("\"") "[^\\n\\r\"]+" Q("\"")),
    TOKEN_DATA(PP_HHEADER, POUNDDEF PPSPACE \
        INCLUDEDEF PPSPACE "<" "[^\\n\\r>]+" ">"),
    TOKEN_DATA(PP_INCLUDE, POUNDDEF PPSPACE \
        INCLUDEDEF PPSPACE),
    TOKEN_DATA(PP_LINE, POUNDDEF PPSPACE "line"),
    TOKEN_DATA(PP_PRAGMA, POUNDDEF PPSPACE "pragma"),
    TOKEN_DATA(PP_UNDEF, POUNDDEF PPSPACE "undef"),
    TOKEN_DATA(PP_WARNING, POUNDDEF PPSPACE "warning"),
#if BOOST_WAVE_SUPPORT_MS_EXTENSIONS != 0
    TOKEN_DATA(MSEXT_INT8, "__int8"),
    TOKEN_DATA(MSEXT_INT16, "__int16"),
    TOKEN_DATA(MSEXT_INT32, "__int32"),
    TOKEN_DATA(MSEXT_INT64, "__int64"),
    TOKEN_DATA(MSEXT_BASED, "_?" "_based"),
    TOKEN_DATA(MSEXT_DECLSPEC, "_?" "_declspec"),
    TOKEN_DATA(MSEXT_CDECL, "_?" "_cdecl"),
    TOKEN_DATA(MSEXT_FASTCALL, "_?" "_fastcall"),
    TOKEN_DATA(MSEXT_STDCALL, "_?" "_stdcall"),
    TOKEN_DATA(MSEXT_TRY , "__try"),
    TOKEN_DATA(MSEXT_EXCEPT, "__except"),
    TOKEN_DATA(MSEXT_FINALLY, "__finally"),
    TOKEN_DATA(MSEXT_LEAVE, "__leave"),
    TOKEN_DATA(MSEXT_INLINE, "_?" "_inline"),
    TOKEN_DATA(MSEXT_ASM, "_?" "_asm"),
    TOKEN_DATA(MSEXT_PP_REGION, POUNDDEF PPSPACE "region"),
    TOKEN_DATA(MSEXT_PP_ENDREGION, POUNDDEF PPSPACE "endregion"),
#endif // BOOST_WAVE_SUPPORT_MS_EXTENSIONS != 0
//  TOKEN_DATA(OCTALINT, "0" OCTALDIGIT "*" INTEGER_SUFFIX "?"),
//  TOKEN_DATA(DECIMALINT, "[1-9]" DIGIT "*" INTEGER_SUFFIX "?"),
//  TOKEN_DATA(HEXAINT, "(0x|0X)" HEXDIGIT "+" INTEGER_SUFFIX "?"),
    TOKEN_DATA(LONGINTLIT, INTEGER LONGINTEGER_SUFFIX),
    TOKEN_DATA(INTLIT, INTEGER INTEGER_SUFFIX "?"),
    TOKEN_DATA(FLOATLIT, 
        "(" DIGIT "*" Q(".") DIGIT "+" OR DIGIT "+" Q(".") ")" 
        EXPONENT "?" FLOAT_SUFFIX "?" OR
        DIGIT "+" EXPONENT FLOAT_SUFFIX "?"),
    TOKEN_DATA(CCOMMENT, CCOMMENT),
    TOKEN_DATA(CPPCOMMENT, Q("/") Q("/[^\\n\\r]*") NEWLINEDEF ),
    TOKEN_DATA(CHARLIT, CHAR_SPEC "'" 
                "(" ESCAPESEQ OR UNIVERSALCHAR OR "[^\\n\\r\\\\']" ")+" "'"),
    TOKEN_DATA(STRINGLIT, CHAR_SPEC Q("\"") 
                "(" ESCAPESEQ OR UNIVERSALCHAR OR "[^\\n\\r\\\\\"]" ")*" Q("\"")),
#if BOOST_WAVE_USE_STRICT_LEXER != 0
    TOKEN_DATA(IDENTIFIER, "([a-zA-Z_]" OR UNIVERSALCHAR ")([a-zA-Z0-9_]" OR UNIVERSALCHAR ")*"),
#else
    TOKEN_DATA(IDENTIFIER, "([a-zA-Z_$]" OR UNIVERSALCHAR ")([a-zA-Z0-9_$]" OR UNIVERSALCHAR ")*"),
#endif
    TOKEN_DATA(SPACE, "[ \t\v\f]+"),
//    TOKEN_DATA(SPACE2, "[\\v\\f]+"),
    TOKEN_DATA(CONTLINE, Q("\\") "\n"), 
    TOKEN_DATA(NEWLINE, NEWLINEDEF),
    TOKEN_DATA(POUND_POUND, "##"),
    TOKEN_DATA(POUND_POUND_ALT, Q("%:") Q("%:")),
    TOKEN_DATA(POUND_POUND_TRIGRAPH, TRI("=") TRI("=")),
    TOKEN_DATA(POUND, "#"),
    TOKEN_DATA(POUND_ALT, Q("%:")),
    TOKEN_DATA(POUND_TRIGRAPH, TRI("=")),
    TOKEN_DATA(ANY_TRIGRAPH, TRI(Q("/"))),
    TOKEN_DATA(ANY, "."),     // this should be the last recognized token
    { token_id(0) }           // this should be the last entry
};

///////////////////////////////////////////////////////////////////////////////
// C++ only token definitions
template <typename IteratorT, typename PositionT>
typename lexer_base<IteratorT, PositionT>::lexer_data const 
lexer<IteratorT, PositionT>::init_data_cpp[INIT_DATA_CPP_SIZE] = 
{
    TOKEN_DATA(AND_ALT, "bitand"),
    TOKEN_DATA(ANDASSIGN_ALT, "and_eq"),
    TOKEN_DATA(ANDAND_ALT, "and"),
    TOKEN_DATA(OR_ALT, "bitor"),
    TOKEN_DATA(ORASSIGN_ALT, "or_eq"),
    TOKEN_DATA(OROR_ALT, "or"),
    TOKEN_DATA(XORASSIGN_ALT, "xor_eq"),
    TOKEN_DATA(XOR_ALT, "xor"),
    TOKEN_DATA(NOTEQUAL_ALT, "not_eq"),
    TOKEN_DATA(NOT_ALT, "not"),
    TOKEN_DATA(COMPL_ALT, "compl"),
#if BOOST_WAVE_SUPPORT_IMPORT_KEYWORD != 0
    TOKEN_DATA(IMPORT, "import"),
#endif
    TOKEN_DATA(ARROWSTAR, Q("->") Q("*")),
    TOKEN_DATA(DOTSTAR, Q(".") Q("*")),
    TOKEN_DATA(COLON_COLON, "::"),
    { token_id(0) }       // this should be the last entry
};

///////////////////////////////////////////////////////////////////////////////
// C++ only token definitions
template <typename IteratorT, typename PositionT>
typename lexer_base<IteratorT, PositionT>::lexer_data const 
lexer<IteratorT, PositionT>::init_data_pp_number[INIT_DATA_PP_NUMBER_SIZE] = 
{
    TOKEN_DATA(PP_NUMBER, PP_NUMBERDEF),
    { token_id(0) }       // this should be the last entry
};

///////////////////////////////////////////////////////////////////////////////
// C++ only token definitions

#define T_EXTCHARLIT      token_id(T_CHARLIT|AltTokenType)
#define T_EXTSTRINGLIT    token_id(T_STRINGLIT|AltTokenType)
#define T_EXTRAWSTRINGLIT token_id(T_RAWSTRINGLIT|AltTokenType)

template <typename IteratorT, typename PositionT>
typename lexer_base<IteratorT, PositionT>::lexer_data const 
lexer<IteratorT, PositionT>::init_data_cpp0x[INIT_DATA_CPP0X_SIZE] = 
{
    TOKEN_DATA(EXTCHARLIT, EXTCHAR_SPEC "'" 
                "(" ESCAPESEQ OR UNIVERSALCHAR OR "[^\\n\\r\\\\']" ")+" "'"),
    TOKEN_DATA(EXTSTRINGLIT, EXTCHAR_SPEC Q("\"") 
                "(" ESCAPESEQ OR UNIVERSALCHAR OR "[^\\n\\r\\\\\"]" ")*" Q("\"")),
    TOKEN_DATA(RAWSTRINGLIT, CHAR_SPEC "R" Q("\"") 
                "(" ESCAPESEQ OR UNIVERSALCHAR OR "[^\\\\\"]" ")*" Q("\"")),
    TOKEN_DATA(EXTRAWSTRINGLIT, EXTCHAR_SPEC "R" Q("\"") 
                "(" ESCAPESEQ OR UNIVERSALCHAR OR "[^\\\\\"]" ")*" Q("\"")),
    TOKEN_DATA(ALIGNAS, "alignas"),
    TOKEN_DATA(ALIGNOF, "alignof"),
    TOKEN_DATA(CHAR16_T, "char16_t"),
    TOKEN_DATA(CHAR32_T, "char32_t"),
    TOKEN_DATA(CONSTEXPR, "constexpr"),
    TOKEN_DATA(DECLTYPE, "decltype"),
    TOKEN_DATA(NOEXCEPT, "noexcept"),
    TOKEN_DATA(NULLPTR, "nullptr"),
    TOKEN_DATA(STATICASSERT, "static_assert"),
    TOKEN_DATA(THREADLOCAL, "threadlocal"),
    { token_id(0) }       // this should be the last entry
};

///////////////////////////////////////////////////////////////////////////////
//  undefine macros, required for regular expression definitions
#undef INCLUDEDEF
#undef POUNDDEF
#undef CCOMMENT
#undef PPSPACE
#undef DIGIT
#undef OCTALDIGIT
#undef HEXDIGIT
#undef NONDIGIT
#undef OPTSIGN
#undef EXPSTART
#undef EXPONENT
#undef LONGINTEGER_SUFFIX
#undef INTEGER_SUFFIX
#undef INTEGER
#undef FLOAT_SUFFIX
#undef CHAR_SPEC
#undef BACKSLASH    
#undef ESCAPESEQ    
#undef HEXQUAD      
#undef UNIVERSALCHAR
#undef PP_NUMBERDEF

#undef Q
#undef TRI
#undef OR

#undef TOKEN_DATA
#undef TOKEN_DATA_EX

///////////////////////////////////////////////////////////////////////////////
// initialize cpp lexer with token data
template <typename IteratorT, typename PositionT>
inline
lexer_base<IteratorT, PositionT>::lexer_base() 
:   base_type(NUM_LEXER_STATES)
{
}

template <typename IteratorT, typename PositionT>
inline void
lexer<IteratorT, PositionT>::init_dfa(boost::wave::language_support lang)
{
    if (this->has_compiled_dfa())
        return;

// if pp-numbers should be preferred, insert the corresponding rule first
    if (boost::wave::need_prefer_pp_numbers(lang)) {
        for (int j = 0; 0 != init_data_pp_number[j].tokenid; ++j) {
            this->register_regex(init_data_pp_number[j].tokenregex, 
                init_data_pp_number[j].tokenid, init_data_pp_number[j].tokencb, 
                init_data_pp_number[j].lexerstate);
        }
    }
        
// if in C99 mode, some of the keywords are not valid    
    if (!boost::wave::need_c99(lang)) {
        for (int j = 0; 0 != init_data_cpp[j].tokenid; ++j) {
            this->register_regex(init_data_cpp[j].tokenregex, 
                init_data_cpp[j].tokenid, init_data_cpp[j].tokencb, 
                init_data_cpp[j].lexerstate);
        }
    }
    
// if in C++0x mode, add all new keywords
#if BOOST_WAVE_SUPPORT_CPP0X != 0
    if (boost::wave::need_cpp0x(lang)) {
        for (int j = 0; 0 != init_data_cpp0x[j].tokenid; ++j) {
            this->register_regex(init_data_cpp0x[j].tokenregex, 
                init_data_cpp0x[j].tokenid, init_data_cpp0x[j].tokencb, 
                init_data_cpp0x[j].lexerstate);
        }
    }
#endif

    for (int i = 0; 0 != init_data[i].tokenid; ++i) {
        this->register_regex(init_data[i].tokenregex, init_data[i].tokenid, 
            init_data[i].tokencb, init_data[i].lexerstate);
    }
}

///////////////////////////////////////////////////////////////////////////////
// get time of last compilation of this file
template <typename IteratorT, typename PositionT>
boost::wave::util::time_conversion_helper 
    lexer<IteratorT, PositionT>::compilation_time(__DATE__ " " __TIME__);

///////////////////////////////////////////////////////////////////////////////
}   // namespace lexer

///////////////////////////////////////////////////////////////////////////////
//  
template <typename IteratorT, typename PositionT>
inline void 
init_lexer (lexer::lexer<IteratorT, PositionT> &lexer, 
    boost::wave::language_support language, bool force_reinit = false)
{
    if (lexer.has_compiled_dfa())
        return;     // nothing to do
        
    using std::ifstream;
    using std::ofstream;
    using std::ios;
    using std::cerr;
    using std::endl;
    
ifstream dfa_in("wave_slex_lexer.dfa", ios::in|ios::binary);

    lexer.init_dfa(language);
    if (force_reinit || !dfa_in.is_open() ||
        !lexer.load (dfa_in, (long)lexer.get_compilation_time()))
    {
#if defined(BOOST_SPIRIT_DEBUG)
        cerr << "Compiling regular expressions for slex ...";
#endif // defined(BOOST_SPIRIT_DEBUG)

        dfa_in.close();
        lexer.create_dfa();

    ofstream dfa_out ("wave_slex_lexer.dfa", ios::out|ios::binary|ios::trunc);

        if (dfa_out.is_open())
            lexer.save (dfa_out, (long)lexer.get_compilation_time());

#if defined(BOOST_SPIRIT_DEBUG)
        cerr << " Done." << endl;
#endif // defined(BOOST_SPIRIT_DEBUG)
    }
}

///////////////////////////////////////////////////////////////////////////////
//  
//  lex_functor
//
///////////////////////////////////////////////////////////////////////////////

template <typename IteratorT, typename PositionT = wave::util::file_position_type>
class slex_functor 
:   public slex_input_interface<
        typename lexer::lexer<IteratorT, PositionT>::token_type
    >
{
public:

    typedef boost::wave::util::position_iterator<IteratorT, PositionT>
          iterator_type;
    typedef typename std::iterator_traits<IteratorT>::value_type    char_type;
    typedef BOOST_WAVE_STRINGTYPE                                   string_type;
    typedef typename lexer::lexer<IteratorT, PositionT>::token_type token_type;

    slex_functor(IteratorT const &first_, IteratorT const &last_, 
            PositionT const &pos_, boost::wave::language_support language_)
    :   first(first_, last_, pos_), language(language_), at_eof(false)
    {
        // initialize lexer dfa tables
        init_lexer(lexer, language_);  
    }
    virtual ~slex_functor() {}

// get the next token from the input stream
    token_type& get(token_type& result)
    {
        if (!at_eof) {
            do {
            // generate and return the next token
            std::string value;
            PositionT pos = first.get_position();   // begin of token position
            token_id id = token_id(lexer.next_token(first, last, &value));

                if ((token_id)(-1) == id)
                    id = T_EOF;     // end of input reached

            string_type token_val(value.c_str());

                if (boost::wave::need_emit_contnewlines(language) ||
                    T_CONTLINE != id) 
                {
                //  The cast should avoid spurious warnings about missing case labels 
                //  for the other token ids's.
                    switch (static_cast<unsigned int>(id)) {   
                    case T_IDENTIFIER:
                    // test identifier characters for validity (throws if 
                    // invalid chars found)
                        if (!boost::wave::need_no_character_validation(language)) {
                            using boost::wave::cpplexer::impl::validate_identifier_name;
                            validate_identifier_name(token_val, 
                                pos.get_line(), pos.get_column(), pos.get_file()); 
                        }
                        break;

                    case T_EXTCHARLIT:
                    case T_EXTSTRINGLIT:
                    case T_EXTRAWSTRINGLIT:
                        id = token_id(id & ~AltTokenType);
                        BOOST_FALLTHROUGH;

                    case T_CHARLIT:
                    case T_STRINGLIT:
                    case T_RAWSTRINGLIT:
                    // test literal characters for validity (throws if invalid 
                    // chars found)
                        if (boost::wave::need_convert_trigraphs(language)) {
                            using boost::wave::cpplexer::impl::convert_trigraphs;
                            token_val = convert_trigraphs(token_val); 
                        }
                        if (!boost::wave::need_no_character_validation(language)) {
                            using boost::wave::cpplexer::impl::validate_literal;
                            validate_literal(token_val, 
                                pos.get_line(), pos.get_column(), pos.get_file()); 
                        }
                        break;

                    case T_LONGINTLIT:  // supported in C99 and long_long mode
                        if (!boost::wave::need_long_long(language)) {
                        // syntax error: not allowed in C++ mode
                            BOOST_WAVE_LEXER_THROW(
                                boost::wave::cpplexer::lexing_exception, 
                                invalid_long_long_literal, value.c_str(), 
                                pos.get_line(), pos.get_column(), 
                                pos.get_file().c_str());
                        }
                        break;

#if BOOST_WAVE_SUPPORT_INCLUDE_NEXT != 0
                    case T_PP_HHEADER:
                    case T_PP_QHEADER:
                    case T_PP_INCLUDE:
                    // convert to the corresponding ..._next token, if appropriate
                        {
                        // Skip '#' and whitespace and see whether we find an 
                        // 'include_next' here.
                            typename string_type::size_type start = value.find("include");
                            if (0 == value.compare(start, 12, "include_next", 12))
                                id = token_id(id | AltTokenType);
                            break;
                        }
#endif // BOOST_WAVE_SUPPORT_INCLUDE_NEXT != 0

                    case T_EOF:
                    // T_EOF is returned as a valid token, the next call will 
                    // return T_EOI, i.e. the actual end of input
                        at_eof = true;
                        token_val.clear();
                        break;

                    case T_OR_TRIGRAPH:
                    case T_XOR_TRIGRAPH:
                    case T_LEFTBRACE_TRIGRAPH:
                    case T_RIGHTBRACE_TRIGRAPH:
                    case T_LEFTBRACKET_TRIGRAPH:
                    case T_RIGHTBRACKET_TRIGRAPH:
                    case T_COMPL_TRIGRAPH:
                    case T_POUND_TRIGRAPH:
                    case T_ANY_TRIGRAPH:
                        if (boost::wave::need_convert_trigraphs(language))
                        {
                            using boost::wave::cpplexer::impl::convert_trigraph;
                            token_val = convert_trigraph(token_val);
                        }
                        break;
                    }

                    result = token_type(id, token_val, pos);
#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
                    return guards.detect_guard(result);
#else
                    return result;
#endif
                }

            // skip the T_CONTLINE token
            } while (true);
        }
        return result = token_type();   // return T_EOI
    }

    void set_position(PositionT const &pos) 
    { 
        // set position has to change the file name and line number only
        first.get_position().set_file(pos.get_file()); 
        first.get_position().set_line(pos.get_line()); 
    }

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    bool has_include_guards(std::string& guard_name) const 
        { return guards.detected(guard_name); }
#endif

private:
    iterator_type first;
    iterator_type last;
    boost::wave::language_support language;
    static lexer::lexer<IteratorT, PositionT> lexer;   // needed only once
    
    bool at_eof;

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    include_guards<token_type> guards;
#endif
};

template <typename IteratorT, typename PositionT>
lexer::lexer<IteratorT, PositionT> slex_functor<IteratorT, PositionT>::lexer;

#undef T_EXTCHARLIT     
#undef T_EXTSTRINGLIT   
#undef T_EXTRAWSTRINGLIT

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
//  lex_functor. While the cpp_slex_token.hpp file may be included everywhere,
//  this file (cpp_slex_lexer.hpp) should be included only once. This allows
//  to decouple the lexer interface from the lexer implementation and reduces 
//  compilation time.
//
///////////////////////////////////////////////////////////////////////////////

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
#define BOOST_WAVE_SLEX_NEW_LEXER_INLINE
#else
#define BOOST_WAVE_SLEX_NEW_LEXER_INLINE inline
#endif 

template <typename IteratorT, typename PositionT>
BOOST_WAVE_SLEX_NEW_LEXER_INLINE
lex_input_interface<slex_token<PositionT> > *
new_lexer_gen<IteratorT, PositionT>::new_lexer(IteratorT const &first,
    IteratorT const &last, PositionT const &pos, 
    boost::wave::language_support language)
{
    return new slex_functor<IteratorT, PositionT>(first, last, pos, 
        language);
}

#undef BOOST_WAVE_SLEX_NEW_LEXER_INLINE

///////////////////////////////////////////////////////////////////////////////
}   // namespace slex
}   // namespace cpplexer
}   // namespace wave
}   // namespace boost
     
#endif // !defined(SLEX_LEXER_HPP_5E8E1DF0_BB41_4938_B7E5_A4BB68222FF6_INCLUDED)

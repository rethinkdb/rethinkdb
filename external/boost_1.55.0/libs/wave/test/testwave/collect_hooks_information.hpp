/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_LIBS_WAVE_TEST_COLLECT_HOOKS_INFORMATION_HPP)
#define BOOST_WAVE_LIBS_WAVE_TEST_COLLECT_HOOKS_INFORMATION_HPP

#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

///////////////////////////////////////////////////////////////////////////////
// workaround for missing ostringstream
#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define BOOST_WAVETEST_OSSTREAM std::ostrstream
std::string BOOST_WAVETEST_GETSTRING(std::ostrstream& ss)
{
    ss << ends;
    std::string rval = ss.str();
    ss.freeze(false);
    return rval;
}
#else
#include <sstream>
#define BOOST_WAVETEST_GETSTRING(ss) ss.str()
#define BOOST_WAVETEST_OSSTREAM std::ostringstream
#endif

///////////////////////////////////////////////////////////////////////////////
template <typename String>
String handle_filepath(String const &name)
{
    using boost::wave::util::impl::unescape_lit;
    
    String unesc_name (unescape_lit(name));
    typename String::size_type p = unesc_name.find_last_of("/\\");
    if (p != unesc_name.npos)
        unesc_name = unesc_name.substr(p+1);
    return unesc_name;
}

///////////////////////////////////////////////////////////////////////////////
template <typename String>
inline String repr(boost::wave::util::file_position<String> const& pos)
{
    std::string linenum = boost::lexical_cast<std::string>(pos.get_line());
    return handle_filepath(pos.get_file()) + String("(") + linenum.c_str() + ")";
}

template <typename String>
inline String repr(String const& value)
{
    String result;
    typename String::const_iterator end = value.end();
    for (typename String::const_iterator it = value.begin(); it != end; ++it)
    {
        typedef typename String::value_type char_type;
        char_type c = *it;
        if (c == static_cast<char_type>('\a'))
            result.append("\\a");
        else if (c == static_cast<char_type>('\b'))
            result.append("\\b");
        else if (c == static_cast<char_type>('\f'))
            result.append("\\f");
        else if (c == static_cast<char_type>('\n'))
            result.append("\\n");
        else if (c == static_cast<char_type>('\r'))
            result.append("\\r");
        else if (c == static_cast<char_type>('\t'))
            result.append("\\t");
        else if (c == static_cast<char_type>('\v'))
            result.append("\\v");
        else
            result += static_cast<char_type>(c);
    }
    return result;
}

#if defined(BOOST_WINDOWS)
template <typename String>
inline String replace_slashes(String value, char const* lookfor = "\\",
    char replace_with = '/')
{
    typename String::size_type p = value.find_first_of(lookfor);
    while (p != value.npos) {
        value[p] = replace_with;
        p = value.find_first_of(lookfor, p+1);
    }
    return value;
}
#endif

///////////////////////////////////////////////////////////////////////////////
template <typename Token>
class collect_hooks_information 
  : public boost::wave::context_policies::eat_whitespace<Token>
{
    typedef boost::wave::context_policies::eat_whitespace<Token> base_type;

public:
    collect_hooks_information(std::string& trace)
      : hooks_trace(trace), skipped_token_hooks(false)
    {}

    void set_skipped_token_hooks(bool flag) 
    {
        skipped_token_hooks = flag;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'expanding_function_like_macro' is called, whenever a 
    //  function-like macro is to be expanded.
    //
    //  The parameter 'macrodef' marks the position, where the macro to expand 
    //  is defined.
    //
    //  The parameter 'formal_args' holds the formal arguments used during the
    //  definition of the macro.
    //
    //  The parameter 'definition' holds the macro definition for the macro to 
    //  trace.
    //
    //  The parameter 'macro_call' marks the position, where this macro invoked.
    //
    //  The parameter 'arguments' holds the macro arguments used during the 
    //  invocation of the macro
    //
    //  The parameters 'seqstart' and 'seqend' point into the input token 
    //  stream allowing to access the whole token sequence comprising the macro
    //  invocation (starting with the opening parenthesis and ending after the
    //  closing one).
    //
    //  The return value defines, whether the corresponding macro will be 
    //  expanded (return false) or will be copied to the output (return true).
    //  Note: the whole argument list is copied unchanged to the output as well
    //        without any further processing.
    //
    ///////////////////////////////////////////////////////////////////////////

    template <typename Context, typename Container, typename Iterator>
    bool 
    expanding_function_like_macro(Context const& ctx,
        Token const& macro, std::vector<Token> const& formal_args, 
        Container const& definition,
        Token const& macrocall, std::vector<Container> const& arguments,
        Iterator const& seqstart, Iterator const& seqend) 
    { 
        BOOST_WAVETEST_OSSTREAM strm;
        // trace real macro call
        strm << "00: " << repr(macrocall.get_position()) << ": " 
             << macrocall.get_value() << "("; 
        for (typename std::vector<Token>::size_type i = 0; 
            i < arguments.size(); ++i) 
        {
            strm << boost::wave::util::impl::as_string(arguments[i]);
            if (i < arguments.size()-1)
                strm << ",";
        }
        strm << "), ";
        
        // trace macro definition
        strm << "[" << repr(macro.get_position()) << ": "
             << macro.get_value() << "("; 
        for (typename std::vector<Token>::size_type i = 0; 
            i < formal_args.size(); ++i) 
        {
            strm << formal_args[i].get_value();
            if (i < formal_args.size()-1)
                strm << ", ";
        }
        strm << ")=" << boost::wave::util::impl::as_string(definition) << "]" 
             << std::endl;

        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;     // default is to normally expand the macro
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'expanding_object_like_macro' is called, whenever a 
    //  object-like macro is to be expanded .
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'macro' marks the position, where the macro to expand 
    //  is defined.
    //
    //  The definition 'definition' holds the macro definition for the macro to 
    //  trace.
    //
    //  The parameter 'macrocall' marks the position, where this macro invoked.
    //
    //  The return value defines, whether the corresponding macro will be 
    //  expanded (return false) or will be copied to the output (return true).
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    bool 
    expanding_object_like_macro(Context const& ctx, Token const& macro, 
        Container const& definition, Token const& macrocall)
    { 
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "01: " << repr(macro.get_position()) << ": " 
             << macro.get_value() << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;   // default is to normally expand the macro
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'expanded_macro' is called, whenever the expansion of a 
    //  macro is finished but before the rescanning process starts.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'result' contains the token sequence generated as the 
    //  result of the macro expansion.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    void expanded_macro(Context const& ctx, Container const& result)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "02: " << boost::wave::util::impl::as_string(result) << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'rescanned_macro' is called, whenever the rescanning of a 
    //  macro is finished.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'result' contains the token sequence generated as the 
    //  result of the rescanning.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    void rescanned_macro(Context const& ctx, Container const& result)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "03: " << boost::wave::util::impl::as_string(result) << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'found_include_directive' is called, whenever a #include
    //  directive was located.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'filename' contains the (expanded) file name found after 
    //  the #include directive. This has the format '<file>', '"file"' or 
    //  'file'.
    //  The formats '<file>' or '"file"' are used for #include directives found 
    //  in the preprocessed token stream, the format 'file' is used for files
    //  specified through the --force_include command line argument.
    //
    //  The parameter 'include_next' is set to true if the found directive was
    //  a #include_next directive and the BOOST_WAVE_SUPPORT_INCLUDE_NEXT
    //  preprocessing constant was defined to something != 0.
    //
    //  The return value defines, whether the found file will be included 
    //  (return false) or will be skipped (return true).
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    bool 
    found_include_directive(Context const& ctx, std::string filename, 
        bool include_next) 
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "04: " << filename;
        if (include_next)
            strm << " (include_next)";
        strm << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;    // ok to include this file
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'opened_include_file' is called, whenever a file referred 
    //  by an #include directive was successfully located and opened.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'filename' contains the file system path of the 
    //  opened file (this is relative to the directory of the currently 
    //  processed file or a absolute path depending on the paths given as the
    //  include search paths).
    //
    //  The include_depth parameter contains the current include file depth.
    //
    //  The is_system_include parameter denotes, whether the given file was 
    //  found as a result of a #include <...> directive.
    //  
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    void 
    opened_include_file(Context const& ctx, std::string relname, 
        std::string absname, bool is_system_include) 
    {
        using boost::wave::util::impl::escape_lit;

#if defined(BOOST_WINDOWS)
        relname = replace_slashes(relname);
        absname = replace_slashes(absname);
#endif

        BOOST_WAVETEST_OSSTREAM strm;
        strm << "05: " << escape_lit(relname) 
             << " (" << escape_lit(absname) << ")" << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'returning_from_include_file' is called, whenever an
    //  included file is about to be closed after it's processing is complete.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    void
    returning_from_include_file(Context const& ctx) 
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "06: " << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'interpret_pragma' is called, whenever a #pragma command 
    //  directive is found which isn't known to the core Wave library, where
    //  command is the value defined as the BOOST_WAVE_PRAGMA_KEYWORD constant
    //  which defaults to "wave".
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'pending' may be used to push tokens back into the input 
    //  stream, which are to be used as the replacement text for the whole 
    //  #pragma directive.
    //
    //  The parameter 'option' contains the name of the interpreted pragma.
    //
    //  The parameter 'values' holds the values of the parameter provided to 
    //  the pragma operator.
    //
    //  The parameter 'act_token' contains the actual #pragma token, which may 
    //  be used for error output.
    //
    //  If the return value is 'false', the whole #pragma directive is 
    //  interpreted as unknown and a corresponding error message is issued. A
    //  return value of 'true' signs a successful interpretation of the given 
    //  #pragma.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    bool 
    interpret_pragma(Context const& ctx, Container &pending, 
        Token const& option, Container const& values, Token const& act_token)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "07: " << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'defined_macro' is called, whenever a macro was defined
    //  successfully.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'name' is a reference to the token holding the macro name.
    //
    //  The parameter 'is_functionlike' is set to true, whenever the newly 
    //  defined macro is defined as a function like macro.
    //
    //  The parameter 'parameters' holds the parameter tokens for the macro
    //  definition. If the macro has no parameters or if it is a object like
    //  macro, then this container is empty.
    //
    //  The parameter 'definition' contains the token sequence given as the
    //  replacement sequence (definition part) of the newly defined macro.
    //
    //  The parameter 'is_predefined' is set to true for all macros predefined 
    //  during the initialization phase of the library.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    void
    defined_macro(Context const& ctx, Token const& macro, 
        bool is_functionlike, std::vector<Token> const& pars, 
        Container const& definition, bool is_predefined)
    {
        // do not trace the definition of the internal helper macros
        if (!is_predefined) {
            BOOST_WAVETEST_OSSTREAM strm;
            strm << "08: " << repr(macro.get_position()) << ": " 
                 << macro.get_value();
            if (is_functionlike) {
            // list the parameter names for function style macros
                strm << "(";
                for (typename std::vector<Token>::size_type i = 0; 
                     i < pars.size(); ++i)
                {
                    strm << pars[i].get_value();
                    if (i < pars.size()-1)
                        strm << ", ";
                }
                strm << ")";
            }
            strm << "=" << boost::wave::util::impl::as_string(definition)
                 << std::endl;
            hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'undefined_macro' is called, whenever a macro definition
    //  was removed successfully.
    //  
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'name' holds the name of the macro, which definition was 
    //  removed.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    void
    undefined_macro(Context const& ctx, Token const& macro)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "09: " << repr(macro.get_position()) << ": " 
             << macro.get_value() << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'found_directive' is called, whenever a preprocessor 
    //  directive was encountered, but before the corresponding action is 
    //  executed.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'directive' is a reference to the token holding the 
    //  preprocessing directive.
    //
    //  The return value defines, whether the given expression has to be 
    //  to be executed in a normal way (return 'false'), or if it has to be  
    //  skipped altogether (return 'true'), which means it gets replaced in the 
    //  output by a single newline.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    bool
    found_directive(Context const& ctx, Token const& directive)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "10: " << repr(directive.get_position()) << ": "
             << directive.get_value() << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;     // by default we never skip any directives
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'evaluated_conditional_expression' is called, whenever a 
    //  conditional preprocessing expression was evaluated (the expression
    //  given to a #if, #elif, #ifdef or #ifndef directive)
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'directive' is a reference to the token holding the 
    //  corresponding preprocessing directive.
    //
    //  The parameter 'expression' holds the non-expanded token sequence
    //  comprising the evaluated expression.
    //
    //  The parameter expression_value contains the result of the evaluation of
    //  the expression in the current preprocessing context.
    //
    //  The return value defines, whether the given expression has to be 
    //  evaluated again, allowing to decide which of the conditional branches
    //  should be expanded. You need to return 'true' from this hook function 
    //  to force the expression to be re-evaluated.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    bool
    evaluated_conditional_expression(Context const& ctx, 
        Token const& directive, Container const& expression, 
        bool expression_value)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "11: " << repr(directive.get_position()) << ": "
             << directive.get_value() << " "
             << boost::wave::util::impl::as_string(expression)  << ": "
             << expression_value << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;       // ok to continue, do not re-evaluate expression
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'skipped_token' is called, whenever a token is about to be
    //  skipped due to a false preprocessor condition (code fragments to be
    //  skipped inside the not evaluated conditional #if/#else/#endif branches).
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'token' refers to the token to be skipped.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    void
    skipped_token(Context const& ctx, Token const& token)
    {
        // this normally generates a lot of noise
        if (skipped_token_hooks) {
            BOOST_WAVETEST_OSSTREAM strm;
            strm << "12: " << repr(token.get_position()) << ": >" 
                 << repr(token.get_value()) << "<" << std::endl;
            hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'generated_token' will be called by the library whenever a
    //  token is about to be returned from the library.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 't' is the token about to be returned from the library.
    //  This function may alter the token, but in this case it must be 
    //  implemented with a corresponding signature: 
    //
    //      Token const&
    //      generated_token(Context const& ctx, Token& t);
    //
    //  which makes it possible to modify the token in place.
    //
    //  The default behavior is to return the token passed as the parameter 
    //  without modification.
    //  
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    Token const&
    generated_token(Context const& ctx, Token const& t)
    { 
// this generates a lot of noise
//        BOOST_WAVETEST_OSSTREAM strm;
//        strm << "13: " << std::endl;
//        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return t; 
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'may_skip_whitespace' will be called by the 
    //  library, whenever it must be tested whether a specific token refers to 
    //  whitespace and this whitespace has to be skipped.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The 'token' parameter holds a reference to the current token. The policy 
    //  is free to change this token if needed.
    //
    //  The 'skipped_newline' parameter holds a reference to a boolean value 
    //  which should be set to true by the policy function whenever a newline 
    //  is going to be skipped. 
    //
    //  If the return value is true, the given token is skipped and the 
    //  preprocessing continues to the next token. If the return value is 
    //  false, the given token is returned to the calling application. 
    //
    //  ATTENTION!
    //  Caution has to be used, because by returning true the policy function 
    //  is able to force skipping even significant tokens, not only whitespace. 
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    bool
    may_skip_whitespace(Context const& ctx, Token& token, bool& skipped_newline)
    { 
// this generates a lot of noise
//         BOOST_WAVETEST_OSSTREAM strm;
//         strm << "14: " << std::endl;
//         hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return this->base_type::may_skip_whitespace(ctx, token, skipped_newline);
    }

#if BOOST_WAVE_SUPPORT_WARNING_DIRECTIVE != 0
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'found_warning_directive' will be called by the library
    //  whenever a #warning directive is found.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'message' references the argument token sequence of the
    //  encountered #warning directive.
    //
    //  If the return value is false, the library throws a preprocessor 
    //  exception of the type 'warning_directive', if the return value is true
    //  the execution continues as if no #warning directive has been found.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    bool
    found_warning_directive(Context const& ctx, Container const& message)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "15: " << boost::wave::util::impl::as_string(message)
             << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'found_error_directive' will be called by the library
    //  whenever a #error directive is found.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'message' references the argument token sequence of the
    //  encountered #error directive.
    //
    //  If the return value is false, the library throws a preprocessor 
    //  exception of the type 'error_directive', if the return value is true
    //  the execution continues as if no #error directive has been found.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    bool
    found_error_directive(Context const& ctx, Container const& message)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "16: " << boost::wave::util::impl::as_string(message)
             << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'found_line_directive' will be called by the library
    //  whenever a #line directive is found.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'arguments' references the argument token sequence of the
    //  encountered #line directive.
    //
    //  The parameter 'line' contains the recognized line number from the #line
    //  directive.
    //
    //  The parameter 'filename' references the recognized file name from the 
    //  #line directive (if there was one given).
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Container>
    void
    found_line_directive(Context const& ctx, Container const& arguments,
        unsigned int line, std::string const& filename)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "17: " << boost::wave::util::impl::as_string(arguments) 
             << " (" << line << ", \"" << filename << "\")" << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'throw_exception' will be called by the library whenever a
    //  preprocessing exception occurs.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'e' is the exception object containing detailed error 
    //  information.
    //
    //  The default behavior is to call the function boost::throw_exception.
    //  
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context, typename Exception>
    void
    throw_exception(Context const& ctx, Exception const& e)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "18: " << e.what() << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return this->base_type::throw_exception(ctx, e);
    }

#if BOOST_WAVE_SUPPORT_PRAGMA_ONCE != 0
    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'detected_include_guard' is called whenever either a 
    //  include file is about to be added to the list of #pragma once headers.
    //  That means this header file will not be opened and parsed again even 
    //  if it is specified in a later #include directive.
    //  This function is called as the result of a detected include guard 
    //  scheme. 
    //
    //  The implemented heuristics for include guards detects two forms of 
    //  include guards:
    // 
    //       #ifndef INCLUDE_GUARD_MACRO
    //       #define INCLUDE_GUARD_MACRO
    //       ...
    //       #endif
    // 
    //   or
    // 
    //       if !defined(INCLUDE_GUARD_MACRO)
    //       #define INCLUDE_GUARD_MACRO
    //       ...
    //       #endif
    // 
    //  note, that the parenthesis are optional (i.e. !defined INCLUDE_GUARD_MACRO
    //  will work as well). The code allows for any whitespace, newline and single 
    //  '#' tokens before the #if/#ifndef and after the final #endif.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'filename' contains the file system path of the 
    //  opened file (this is relative to the directory of the currently 
    //  processed file or a absolute path depending on the paths given as the
    //  include search paths).
    //
    //  The parameter contains the name of the detected include guard.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContextT>
    void
    detected_include_guard(ContextT const& ctx, std::string filename,
        std::string const& include_guard) 
    {
        using boost::wave::util::impl::escape_lit;

#if defined(BOOST_WINDOWS)
        filename = replace_slashes(filename);
#endif

        BOOST_WAVETEST_OSSTREAM strm;
        strm << "19: " << escape_lit(filename) << ": " 
             << include_guard << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }

    ///////////////////////////////////////////////////////////////////////////
    //  
    //  The function 'detected_pragma_once' is called whenever either a 
    //  include file is about to be added to the list of #pragma once headers.
    //  That means this header file will not be opened and parsed again even 
    //  if it is specified in a later #include directive.
    //  This function is called as the result of a detected directive
    //  #pragma once. 
    //  
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter pragma_token refers to the token "#pragma" triggering 
    //  this preprocessing hook.
    //
    //  The parameter 'filename' contains the file system path of the 
    //  opened file (this is relative to the directory of the currently 
    //  processed file or a absolute path depending on the paths given as the
    //  include search paths).
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContextT, typename TokenT>
    void
    detected_pragma_once(ContextT const& ctx, TokenT const& pragma_token,
        std::string filename) 
    {
        using boost::wave::util::impl::escape_lit;

#if defined(BOOST_WINDOWS)
        filename = replace_slashes(filename);
#endif

        BOOST_WAVETEST_OSSTREAM strm;
        strm << "20: " << repr(pragma_token.get_position()) << ": " 
             << pragma_token.get_value() << ": " 
             << escape_lit(filename) << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
    }
#endif 

    ///////////////////////////////////////////////////////////////////////////
    //
    //  The function 'found_unknown_directive' is called, whenever an unknown 
    //  preprocessor directive was encountered.
    //
    //  The parameter 'ctx' is a reference to the context object used for 
    //  instantiating the preprocessing iterators by the user.
    //
    //  The parameter 'line' holds the tokens of the entire source line
    //  containing the unknown directive.
    //
    //  The parameter 'pending' may be used to push tokens back into the input 
    //  stream, which are to be used as the replacement text for the whole 
    //  line containing the unknown directive.
    //
    //  The return value defines, whether the given expression has been 
    //  properly interpreted by the hook function or not. If this function 
    //  returns 'false', the library will raise an 'ill_formed_directive' 
    //  preprocess_exception. Otherwise the tokens pushed back into 'pending'
    //  are passed on to the user program.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ContextT, typename ContainerT>
    bool
    found_unknown_directive(ContextT const& ctx, ContainerT const& line, 
        ContainerT& pending)
    {
        BOOST_WAVETEST_OSSTREAM strm;
        strm << "21: " << repr((*line.begin()).get_position()) << ": " 
             << boost::wave::util::impl::as_string(line) << std::endl;
        hooks_trace += BOOST_WAVETEST_GETSTRING(strm);
        return false; 
    }

private:
    std::string& hooks_trace;
    bool skipped_token_hooks;
};

#endif




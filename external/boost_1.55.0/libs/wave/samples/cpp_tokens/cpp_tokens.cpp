/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    
    Sample: Print out the preprocessed tokens returned by the Wave iterator

    This sample shows, how it is possible to use a custom lexer type and a 
    custom token type with the Wave library. 

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "cpp_tokens.hpp"                  // global configuration

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
//  The following files contain the custom lexer type to use
#include "slex_token.hpp"
#include "slex_iterator.hpp"

///////////////////////////////////////////////////////////////////////////////
//  include lexer specifics, import lexer names
#if !defined(BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION)
#include "slex/cpp_slex_lexer.hpp"
#endif // !defined(BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION)

///////////////////////////////////////////////////////////////////////////////
//  import required names
using namespace boost::spirit::classic;

using std::string;
using std::getline;
using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;

///////////////////////////////////////////////////////////////////////////////
//  main program
int
main(int argc, char *argv[])
{
    if (2 != argc) {
        cout << "Usage: cpp_tokens input_file" << endl;
        return 1;
    }

// read the file to analyse into a std::string
    ifstream infile(argv[1]);
    string teststr;
    if (infile.is_open()) {
        infile.unsetf(std::ios::skipws);
#if defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)
        // this is known to be very slow for large files on some systems
        copy (std::istream_iterator<char>(infile),
              std::istream_iterator<char>(), 
              std::inserter(teststr, teststr.end()));
#else
        teststr = std::string(std::istreambuf_iterator<char>(infile.rdbuf()),
                              std::istreambuf_iterator<char>());
#endif 
    }
    else {
        teststr = argv[1];
    }

//  The following typedef does the trick. It defines the context type to use, 
//  which depends on the lexer type (provided by the second template 
//  parameter). Our lexer type 'slex_iterator<>' depends on a custom token type
//  'slex_token<>'. Our custom token type differs from the original one provided 
//  by the Wave library only by defining an additional operator<<(), which is 
//  used to dump the token information carried by a given token (see loop 
//  below).
    typedef boost::wave::cpplexer::slex_token<> token_type;
    typedef boost::wave::cpplexer::slex::slex_iterator<token_type> lexer_type;
    typedef boost::wave::context<std::string::iterator, lexer_type> 
        context_type;

// The C++ preprocessor iterator shouldn't be constructed directly. It is to be
// generated through a boost::wave::context<> object. This object is 
// additionally to be used to initialize and define different parameters of 
// the actual preprocessing.
// The preprocessing of the input stream is done on the fly behind the scenes
// during iteration over the context_type::iterator_type stream.
    context_type ctx (teststr.begin(), teststr.end(), argv[1]);

    ctx.set_language(boost::wave::support_cpp0x);
    ctx.set_language(boost::wave::enable_preserve_comments(ctx.get_language()));
    ctx.set_language(boost::wave::enable_prefer_pp_numbers(ctx.get_language()));
    ctx.set_language(boost::wave::enable_emit_contnewlines(ctx.get_language()));

    context_type::iterator_type first = ctx.begin();
    context_type::iterator_type last = ctx.end();
    context_type::token_type current_token;

    try {
    //  Traverse over the tokens generated from the input and dump the token
    //  contents.
        while (first != last) {
        // retrieve next token
            current_token = *first;

        // output token info
            cout << "matched " << current_token << endl;
            ++first;
        }
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        cerr 
            << e.file_name() << "(" << e.line_no() << "): "
            << e.description() << endl;
        return 2;
    }
    catch (std::exception const& e) {
    // use last recognized token to retrieve the error position
        cerr 
            << current_token.get_position().get_file() 
            << "(" << current_token.get_position().get_line() << "): "
            << "unexpected exception: " << e.what()
            << endl;
        return 3;
    }
    catch (...) {
    // use last recognized token to retrieve the error position
        cerr 
            << current_token.get_position().get_file() 
            << "(" << current_token.get_position().get_line() << "): "
            << "unexpected exception." << endl;
        return 4;
    }
    return 0;
}

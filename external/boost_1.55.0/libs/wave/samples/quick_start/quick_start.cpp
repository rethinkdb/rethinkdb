/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
// Include the lexer stuff
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class

///////////////////////////////////////////////////////////////////////////////
// main entry point
int main(int argc, char *argv[])
{
    if (2 != argc) {
        std::cerr << "Usage: quick_start infile" << std::endl;
        return -1;
    }
    
// current file position is saved for exception handling
boost::wave::util::file_position_type current_position;

    try {
//[quick_start_main
    //  The following preprocesses the input file given by argv[1].
    //  Open and read in the specified input file.
        std::ifstream instream(argv[1]);
        std::string instring;

        if (!instream.is_open()) {
            std::cerr << "Could not open input file: " << argv[1] << std::endl;
            return -2;
        }
        instream.unsetf(std::ios::skipws);
        instring = std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                                std::istreambuf_iterator<char>());

    //  This token type is one of the central types used throughout the library. 
    //  It is a template parameter to some of the public classes and instances 
    //  of this type are returned from the iterators.
        typedef boost::wave::cpplexer::lex_token<> token_type;

    //  The template boost::wave::cpplexer::lex_iterator<> is the lexer type to
    //  to use as the token source for the preprocessing engine. It is 
    //  parametrized with the token type.
        typedef boost::wave::cpplexer::lex_iterator<token_type> lex_iterator_type;

    //  This is the resulting context type. The first template parameter should
    //  match the iterator type used during construction of the context 
    //  instance (see below). It is the type of the underlying input stream.
        typedef boost::wave::context<std::string::iterator, lex_iterator_type>
            context_type;

    //  The preprocessor iterator shouldn't be constructed directly. It is 
    //  generated through a wave::context<> object. This wave:context<> object 
    //  is additionally used to initialize and define different parameters of 
    //  the actual preprocessing (not done here).
    //
    //  The preprocessing of the input stream is done on the fly behind the 
    //  scenes during iteration over the range of context_type::iterator_type 
    //  instances.
        context_type ctx (instring.begin(), instring.end(), argv[1]);

    //  Get the preprocessor iterators and use them to generate the token 
    //  sequence.
        context_type::iterator_type first = ctx.begin();
        context_type::iterator_type last = ctx.end();

    //  The input stream is preprocessed for you while iterating over the range
    //  [first, last). The dereferenced iterator returns tokens holding 
    //  information about the preprocessed input stream, such as token type,
    //  token value, and position.
        while (first != last) {
            current_position = (*first).get_position();
            std::cout << (*first).get_value();
            ++first;
        }
//]
    }
    catch (boost::wave::cpp_exception const& e) {
    // some preprocessing error
        std::cerr 
            << e.file_name() << "(" << e.line_no() << "): "
            << e.description() << std::endl;
        return 2;
    }
    catch (std::exception const& e) {
    // use last recognized token to retrieve the error position
        std::cerr 
            << current_position.get_file() 
            << "(" << current_position.get_line() << "): "
            << "exception caught: " << e.what()
            << std::endl;
        return 3;
    }
    catch (...) {
    // use last recognized token to retrieve the error position
        std::cerr 
            << current_position.get_file() 
            << "(" << current_position.get_line() << "): "
            << "unexpected exception caught." << std::endl;
        return 4;
    }
    return 0;
}

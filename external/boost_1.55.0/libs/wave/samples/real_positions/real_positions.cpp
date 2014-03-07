/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample showing how to correct the positions inside the returned tokens in 
    a way that these appear to be consecutive (ignoring positions from macro 
    definitions).
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
// Include the lexer stuff
#include "real_position_token.hpp"                    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>   // lexer type

#include "correct_token_positions.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Special output operator for a lex_token.
//
//      Note: this doesn't compile if BOOST_SPIRIT_DEBUG is defined.
//
///////////////////////////////////////////////////////////////////////////////
template <typename PositionT>
inline std::ostream &
operator<< (std::ostream &stream, lex_token<PositionT> const &t)
{
    using namespace std;
    using namespace boost::wave;

    token_id id = token_id(t);
    stream << setw(16) 
        << left << boost::wave::get_token_name(id) << " ("
        << "#" << setw(3) << BASEID_FROM_TOKEN(id);

    if (ExtTokenTypeMask & id) {
    // this is an extended token id
        if (AltTokenType == (id & ExtTokenOnlyMask)) {
            stream << ", AltTokenType";
        }
        else if (TriGraphTokenType == (id & ExtTokenOnlyMask)) {
            stream << ", TriGraphTokenType";
        }
        else if (AltExtTokenType == (id & ExtTokenOnlyMask)){
            stream << ", AltExtTokenType";
        }
    }

    stream << "): >";

    typedef typename lex_token<PositionT>::string_type string_type;

    string_type const& value = t.get_value();
    for (std::size_t i = 0; i < value.size(); ++i) {
        switch (value[i]) {
        case '\r':  stream << "\\r"; break;
        case '\n':  stream << "\\n"; break;
        case '\t':  stream << "\\t"; break;
        default:
            stream << value[i]; 
            break;
        }
    }
    stream << "<" << std::endl;
    stream << "    at:  " << t.get_position().get_file() << " (" 
        << setw(3) << right << t.get_position().get_line() << "/" 
        << setw(2) << right << t.get_position().get_column() 
        << ")" << std::endl;
    stream << "    and: " << t.get_corrected_position().get_file() << " (" 
        << setw(3) << right << t.get_corrected_position().get_line() << "/" 
        << setw(2) << right << t.get_corrected_position().get_column() 
        << ")";

    return stream;
}

///////////////////////////////////////////////////////////////////////////////
// main entry point
int main(int argc, char *argv[])
{
    if (2 != argc) {
        std::cerr << "Usage: real_positions infile" << std::endl;
        return -1;
    }

// current file position is saved for exception handling
boost::wave::util::file_position_type current_position;

    try {
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

    //  The template real_positions::lex_token<> is the token type to be 
    //  used by the Wave library.
        typedef lex_token<> token_type;

    //  The template boost::wave::cpplexer::lex_iterator<> is the lexer type to
    //  be used by the Wave library.
        typedef boost::wave::cpplexer::lex_iterator<token_type> 
            lex_iterator_type;

    //  This is the resulting context type to use. The first template parameter
    //  should match the iterator type to be used during construction of the
    //  corresponding context object (see below).
        typedef boost::wave::context<
                std::string::iterator, lex_iterator_type,
                boost::wave::iteration_context_policies::load_file_to_string,
                correct_token_position<token_type> > 
            context_type;

    // This preprocessor hooks are used to correct the file positions inside 
    // the tokens returned from the library
    correct_token_position<token_type> hooks(argv[1]);

    // The preprocessor iterator shouldn't be constructed directly. It is 
    // to be generated through a wave::context<> object. This wave:context<> 
    // object is to be used additionally to initialize and define different 
    // parameters of the actual preprocessing (not done here).
    //
    // The preprocessing of the input stream is done on the fly behind the 
    // scenes during iteration over the context_type::iterator_type stream.
    context_type ctx (instring.begin(), instring.end(), argv[1], hooks);

    // analyze the input file
    context_type::iterator_type first = ctx.begin();
    context_type::iterator_type last = ctx.end();

        while (first != last) {
            current_position = (*first).get_position();
            std::cout << *first << std::endl;
            ++first;
        }
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

/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// disable stupid compiler warnings
#include <boost/config/warning_disable.hpp>

//  system headers
#include <string>
#include <iostream>
#include <limits>

#include <boost/wave/wave_config.hpp>
#include <boost/detail/lightweight_test.hpp>
#if defined(TESTLEXERS_TIMING)
#include "high_resolution_timer.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
//  include the Xpressive lexer related stuff
#include <boost/wave/cpplexer/cpp_lex_token.hpp>                  // token type
#include <libs/wave/samples/token_statistics/xlex/xlex_lexer.hpp> // lexer type

typedef boost::wave::cpplexer::lex_token<> token_type;
typedef boost::wave::cpplexer::xlex::xlex_iterator<token_type> lexer_type;

//  This instantiates the correct 'new_lexer' function, which generates the 
//  C++ lexer used in this test.
template struct boost::wave::cpplexer::xlex::new_lexer_gen<std::string::iterator>;

///////////////////////////////////////////////////////////////////////////////
//  include test data
#include "cpp_tokens.hpp"

///////////////////////////////////////////////////////////////////////////////
int 
main(int argc, char *argv[])
{
    try {
        token_type::position_type pos("<testdata>");
    
#if defined(TESTLEXERS_TIMING)
        boost::high_resolution_timer tim;
        for (int i = 0; i < 10; ++i) {
#endif

        for (lexem const* data = lexems; NULL != data->token; ++data) {
        // feed the token to the lexer
        token_type::string_type instr(data->token);

        lexer_type it = lexer_type(instr.begin(), instr.end(), pos, 
            boost::wave::support_option_long_long);
        lexer_type end = lexer_type();

        // verify the correct outcome of the tokenisation
#if defined(TESTLEXERS_VERBOSE)
        std::cerr << boost::wave::get_token_name(data->id) << std::endl;
#endif

            if (data->id != boost::wave::token_id(*it)) {
                BOOST_TEST(data->id == boost::wave::token_id(*it));
                std::cerr << data->token << ": expected: " 
                    << boost::wave::get_token_name(data->id);
                std::cerr << ", found: " 
                    << boost::wave::get_token_name(boost::wave::token_id(*it)) 
                    << std::endl;
            }
            BOOST_TEST(++it != end);
            BOOST_TEST(boost::wave::T_EOF == boost::wave::token_id(*it));
        }

#if defined(TESTLEXERS_TIMING)
        }
        std::cout << tim.elapsed() << " [s]" << std::endl;
#endif
    }
    catch (boost::wave::cpplexer::lexing_exception &e) {
    // some lexing error
        std::cerr 
            << "test_xlex_lexer: "
            << e.description() << std::endl;
        return (std::numeric_limits<int>::max)() - 1;
    }

    return boost::report_errors();
}


/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(CORRECT_TOKEN_POSITIONS_HK_061106_INCLUDED)
#define CORRECT_TOKEN_POSITIONS_HK_061106_INCLUDED

#include <boost/iterator/transform_iterator.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace detail
{
    // count the newlines in a C style comment
    template <typename String>
    unsigned count_newlines(String const& str)
    {
        unsigned newlines = 0;
        typename String::size_type p = str.find_first_of('\n');
        while (p != String::npos)
        {
            ++newlines;
            p = str.find_first_of('\n', p+1);
        }
        return newlines;
    }

    // return the length of the last line in a C style comment
    template <typename String>
    unsigned last_line_length(String const& str)
    {
        unsigned len = str.size();
        typename String::size_type p = str.find_last_of('\n');
        if (p != String::npos) 
            len -= p;
        return len;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  This is the position correcting functor
template <typename Token>
struct correct_token_position
:   public boost::wave::context_policies::eat_whitespace<Token>
{
    correct_token_position(typename Token::string_type filename)
    :   pos(filename) {}

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
    //      TokenT const&
    //      generated_token(ContextT const& ctx, TokenT& t);
    //
    //  which makes it possible to modify the token in place.
    //
    //  The default behavior is to return the token passed as the parameter 
    //  without modification.
    //  
    ///////////////////////////////////////////////////////////////////////////
    template <typename Context>
    Token const&
    generated_token(Context const& ctx, Token& token)
    { 
        typedef typename Token::string_type string_type;
        typedef typename Token::position_type position_type;

        using namespace boost::wave;

        // adjust the current position
        position_type current_pos(pos);

        token_id id = token_id(token);
        string_type const& v (token.get_value());

        switch (id) {
        case T_NEWLINE:
        case T_CPPCOMMENT:
            pos.set_line(current_pos.get_line()+1);
            pos.set_column(1);
            break;

        case T_CCOMMENT:
            {
                unsigned lines = detail::count_newlines(v);
                if (lines > 0) {
                    pos.set_line(current_pos.get_line() + lines);
                    pos.set_column(detail::last_line_length(v));
                }
                else {
                    pos.set_column(current_pos.get_column() + 
                        detail::last_line_length(v));
                }
            }
            break;

        default:
            pos.set_column(current_pos.get_column() + v.size());
            break;
        }

        // set the new position in the token to be returned
        token.set_corrected_position(current_pos);
        return token;
    }

    typename Token::position_type pos;
};

///////////////////////////////////////////////////////////////////////////////

#endif

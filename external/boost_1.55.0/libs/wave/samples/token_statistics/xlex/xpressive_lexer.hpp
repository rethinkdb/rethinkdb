/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Xpressive based generic lexer
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(XPRESSIVE_LEXER_HPP)
#define XPRESSIVE_LEXER_HPP

#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include <boost/detail/iterator.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace boost {
namespace wave {
namespace cpplexer {
namespace xlex {

///////////////////////////////////////////////////////////////////////////////
template <
    typename Iterator = char const*, 
    typename Token = int,
    typename Callback = bool (*)(
        Iterator const&, Iterator&, Iterator const&, Token const&)
>
class xpressive_lexer
{
private:
    typedef typename boost::detail::iterator_traits<Iterator>::value_type
        char_type;
    typedef std::basic_string<char_type> string_type;
    
    // this represents a single token to match
    struct regex_info
    {
        typedef boost::xpressive::basic_regex<Iterator> regex_type;
        
        string_type str;
        Token token;
        regex_type regex;
        Callback callback;

        regex_info(string_type const& str, Token const& token, 
                Callback const& callback)
        :   str(str), token(token), 
            regex(regex_type::compile(str)), 
            callback(callback)
        {}
        
        // these structures are to be ordered by the token id
        friend bool operator< (regex_info const& lhs, regex_info const& rhs)
        {
            return lhs.token < rhs.token;
        }
    };

    typedef std::vector<regex_info> regex_list_type;

public:
    typedef Callback callback_type;
    
    xpressive_lexer() {}

    // register a the regex with the lexer
    void register_regex(string_type const& regex, Token const& id, 
        Callback const& cb = Callback());

    // match the given input and return the next recognized token
    Token next_token(Iterator &first, Iterator const& last, string_type& token);

private:
    regex_list_type regex_list;
};

///////////////////////////////////////////////////////////////////////////////
template <typename Iterator, typename Token, typename Callback>
inline void
xpressive_lexer<Iterator, Token, Callback>::register_regex(
    string_type const& regex, Token const& id, Callback const& cb)
{
    regex_list.push_back(regex_info(regex, id, cb));
}

///////////////////////////////////////////////////////////////////////////////
template <typename Iterator, typename Token, typename Callback>
inline Token
xpressive_lexer<Iterator, Token, Callback>::next_token(
    Iterator &first, Iterator const& last, string_type& token)
{
    typedef typename regex_list_type::iterator iterator;
    
    xpressive::match_results<Iterator> regex_result;
    for (iterator it = regex_list.begin(), end = regex_list.end(); it != end; ++it)
    {
        namespace xpressive = boost::xpressive;

//         regex_info const& curr_regex = *it;
//         xpressive::match_results<Iterator> regex_result;
        if (xpressive::regex_search(first, last, regex_result, (*it).regex,
            xpressive::regex_constants::match_continuous)) 
        {
            Iterator saved = first;
            Token rval = (*it).token;
            
            std::advance(first, regex_result.length());
            token = string_type(saved, first);

            if (NULL != (*it).callback) {
            // execute corresponding callback
                if ((*it).callback(saved, first, last, (*it).token)) 
                    rval = next_token(first, last, token);
            }
            
            return rval;
        }
    }
    return Token(-1);    // TODO: change this to use token_traits<Token>
}

///////////////////////////////////////////////////////////////////////////////
}}}} // boost::wave::cpplexer::xlex

#endif // !defined(XPRESSIVE_LEXER_HPP)



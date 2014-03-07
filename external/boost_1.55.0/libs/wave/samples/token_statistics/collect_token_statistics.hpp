/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: Collect token statistics from the analysed files
            
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost 
    Software License, Version 1.0. (See accompanying file 
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(COLLECT_TOKEN_STATISTICS_VERSION_HPP)
#define COLLECT_TOKEN_STATISTICS_VERSION_HPP

#include <algorithm>
#include <map>

#include <boost/assert.hpp>
#include <boost/wave/token_ids.hpp>

///////////////////////////////////////////////////////////////////////////////
class collect_token_statistics
{
    enum {
        count = boost::wave::T_LAST_TOKEN - boost::wave::T_FIRST_TOKEN
    };
    
public:
    collect_token_statistics()
    {
        std::fill(&token_count[0], &token_count[count], 0);
    }
    
    // account for the given token type
    template <typename Token>
    void operator() (Token const& token)
    {
        using boost::wave::token_id;
        
        int id = token_id(token) - boost::wave::T_FIRST_TOKEN;
        BOOST_ASSERT(id < count);
        ++token_count[id];
    }

    // print out the token statistics in descending order
    void print() const
    {
        using namespace boost::wave;
        typedef std::multimap<int, token_id> ids_type;
        
        ids_type ids;
        for (unsigned int i = 0; i < count; ++i)
        {
            ids.insert(ids_type::value_type(
                token_count[i], token_id(i + T_FIRST_TOKEN)));
        }
        
        ids_type::reverse_iterator rend = ids.rend();
        for(ids_type::reverse_iterator rit = ids.rbegin(); rit != rend; ++rit) 
        {
            std::cout << boost::wave::get_token_name((*rit).second) << ": " 
                      << (*rit).first << std::endl;
        }
    }
    
private:
    int token_count[count];
};

#endif // !defined(COLLECT_TOKEN_STATISTICS_VERSION_HPP)

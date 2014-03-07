/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "regex_comparison.hpp"

#ifdef BOOST_HAS_XPRESSIVE
#include <cassert>
#include <iostream>
#include <boost/timer.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace dxpr
{

double time_match(const std::string& re, const std::string& text, bool icase)
{
    try{
    boost::xpressive::regex_constants::syntax_option_type flags = boost::xpressive::regex_constants::optimize;
    if(icase)
        flags = flags | boost::xpressive::regex_constants::icase;
    boost::xpressive::sregex e(boost::xpressive::sregex::compile(re, flags));
    boost::xpressive::smatch what;
    boost::timer tim;
    int iter = 1;
    int counter, repeats;
    double result = 0;
    double run;
    assert(boost::xpressive::regex_match( text, what, e ));
    do
    {
        tim.restart();
        for(counter = 0; counter < iter; ++counter)
        {
            boost::xpressive::regex_match( text, what, e );
        }
        result = tim.elapsed();
        iter *= 2;
    } while(result < 0.5);
    iter /= 2;

    // repeat test and report least value for consistency:
    for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
    {
        tim.restart();
        for(counter = 0; counter < iter; ++counter)
        {
            boost::xpressive::regex_match( text, what, e );
        }
        run = tim.elapsed();
        result = (std::min)(run, result);
    }
    return result / iter;
   }
   catch(const std::exception& e)
   {
      std::cout << "Exception: " << e.what() << std::endl;
      return -1;
   }
}

struct noop
{
    void operator()( boost::xpressive::smatch const & ) const
    {
    }
};

double time_find_all(const std::string& re, const std::string& text, bool icase)
{
    try{
    boost::xpressive::regex_constants::syntax_option_type flags = boost::xpressive::regex_constants::optimize;
    if(icase)
        flags = flags | boost::xpressive::regex_constants::icase;
    boost::xpressive::sregex e(boost::xpressive::sregex::compile(re, flags));
    boost::xpressive::smatch what;
    boost::timer tim;
    int iter = 1;
    int counter, repeats;
    double result = 0;
    double run;
    do
    {
        tim.restart();
        for(counter = 0; counter < iter; ++counter)
        {
            boost::xpressive::sregex_iterator begin( text.begin(), text.end(), e ), end;
            std::for_each( begin, end, noop() );
        }
        result = tim.elapsed();
        iter *= 2;
    }while(result < 0.5);
    iter /= 2;

    if(result >10)
        return result / iter;

    // repeat test and report least value for consistency:
    for(repeats = 0; repeats < REPEAT_COUNT; ++repeats)
    {
        tim.restart();
        for(counter = 0; counter < iter; ++counter)
        {
            boost::xpressive::sregex_iterator begin( text.begin(), text.end(), e ), end;
            std::for_each( begin, end, noop() );
        }
        run = tim.elapsed();
        result = (std::min)(run, result);
    }
    return result / iter;
    }
    catch(const std::exception& e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        return -1;
    }
}

}

#else

namespace dxpr{

double time_match(const std::string& re, const std::string& text, bool icase)
{
   return -1;
}
double time_find_all(const std::string& re, const std::string& text, bool icase)
{
   return -1;
}

}
#endif



/*
 *
 * Copyright (c) 2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "./regex_comparison.hpp"
#include <map>
#include <cassert>
#include <boost/timer.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace sxpr
{

using namespace boost::xpressive;

// short matches
char const * sz1 = "^([0-9]+)(\\-| |$)(.*)$";
sregex rx1 = bol >> (s1= +range('0','9')) >> (s2= as_xpr('-')|' '|eol) >> (s3= *_) >> eol;

char const * sz2 = "([[:digit:]]{4}[- ]){3}[[:digit:]]{3,4}";
sregex rx2 = bol >> repeat<3>(s1= repeat<4>(set[digit]) >> (set='-',' ')) >> repeat<3,4>(set[digit]);

char const * sz3 = "^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$";
sregex rx3
    =  bol 
    >> (s1= +set[ range('a','z') | range('A','Z') | range('0','9') | '_' | '-' | '.' ]) 
    >> '@'
    >> (s2=
            (s3= '[' >> repeat<1,3>(range('0','9')) >> '.' >> repeat<1,3>(range('0','9')) 
                 >> '.' >> repeat<1,3>(range('0','9')) >> '.' 
            )
         |  
            (s4= +(s5= +set[ range('a','z') | range('A','Z') | range('0','9') | '-' ] >> '.' ) ) 
       )
    >> (s6= repeat<2,4>(set[ range('a','z') | range('A','Z')]) | repeat<1,3>(range('0','9')))
    >> (s7= !as_xpr(']'))
    >> eol
    ;

char const * sz4 = "^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$";
sregex rx4 = rx3;

char const * sz5 = "^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$";
sregex rx5 = rx3;

char const * sz6 = "^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$";
sregex rx6 = bol >> repeat<1,2>(set[ range('a','z') | range('A','Z') ])
            >> range('0','9') >> repeat<0,1>(set[range('0','9')|range('A','Z')|range('a','z')])
            >> repeat<0,1>(as_xpr(' ')) >> range('0','9') 
            >> repeat<2>(set[range('A','Z')|range('a','z')]) >> eol;

char const * sz7 = "^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$";
sregex rx7 = rx6;

char const * sz8 = "^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$";
sregex rx8 = rx6;

char const * sz9 = "^[[:digit:]]{1,2}/[[:digit:]]{1,2}/[[:digit:]]{4}$";
sregex rx9 = bol >> repeat<1,2>(set[digit]) >> '/' >> repeat<1,2>(set[digit]) >> '/' >> repeat<4>(set[digit]) >> eol;

char const * sz10 = "^[[:digit:]]{1,2}/[[:digit:]]{1,2}/[[:digit:]]{4}$";
sregex rx10 = rx9;

char const * sz11 = "^[-+]?[[:digit:]]*\\.?[[:digit:]]*$";
sregex rx11 = bol >> !(set= '-','+') >> *set[digit] >> !as_xpr('.') >> *set[digit] >> eol ;

char const * sz12 = "^[-+]?[[:digit:]]*\\.?[[:digit:]]*$";
sregex rx12 = rx11;

char const * sz13 = "^[-+]?[[:digit:]]*\\.?[[:digit:]]*$";
sregex rx13 = rx11;

// long matches
char const * sz14 = "Twain";
boost::xpressive::sregex rx14 = as_xpr("Twain");

char const * sz15 = "Huck[[:alpha:]]+";
boost::xpressive::sregex rx15 = "Huck" >> +set[alpha];

char const * sz16 = "[[:alpha:]]+ing";
boost::xpressive::sregex rx16 = +set[alpha] >> "ing";

char const * sz17 = "^[^\n]*?Twain";
boost::xpressive::sregex rx17 = bol >> -*~as_xpr('\n') >> "Twain";

char const * sz18 = "Tom|Sawyer|Huckleberry|Finn";
boost::xpressive::sregex rx18 = ( as_xpr("Tom") | "Sawyer" | "Huckleberry" | "Finn" );

//char const * sz18 = "Tom|Sawyer|.uckleberry|Finn";
//boost::xpressive::sregex rx18 = ( as_xpr("Tom") | "Sawyer" | _ >> "uckleberry" | "Finn" );

char const * sz19 = "(Tom|Sawyer|Huckleberry|Finn).{0,30}river|river.{0,30}(Tom|Sawyer|Huckleberry|Finn)";
boost::xpressive::sregex rx19 = 
       (s1= as_xpr("Tom") | "Sawyer" | "Huckleberry" | "Finn" )
    >> repeat<0,30>(_)
    >> "river"
    |
       "river"
    >> repeat<0,30>(_)
    >> (s2= as_xpr("Tom") | "Sawyer" | "Huckleberry" | "Finn" );

std::map< std::string, sregex > rxmap;

struct map_init
{
    map_init()
    {
        rxmap[ sz1 ] = rx1;
        rxmap[ sz2 ] = rx2;
        rxmap[ sz3 ] = rx3;
        rxmap[ sz4 ] = rx4;
        rxmap[ sz5 ] = rx5;
        rxmap[ sz6 ] = rx6;
        rxmap[ sz7 ] = rx7;
        rxmap[ sz8 ] = rx8;
        rxmap[ sz9 ] = rx9;
        rxmap[ sz10 ] = rx10;
        rxmap[ sz11 ] = rx11;
        rxmap[ sz12 ] = rx12;
        rxmap[ sz13 ] = rx13;
        rxmap[ sz14 ] = rx14;
        rxmap[ sz15 ] = rx15;
        rxmap[ sz16 ] = rx16;
        rxmap[ sz17 ] = rx17;
        rxmap[ sz18 ] = rx18;
        rxmap[ sz19 ] = rx19;
    }
};

static map_init const i = map_init();

double time_match(const std::string& re, const std::string& text)
{
    boost::xpressive::sregex const &e = rxmap[ re ];
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

struct noop
{
    void operator()( boost::xpressive::smatch const & ) const
    {
    }
};

double time_find_all(const std::string& re, const std::string& text)
{
    boost::xpressive::sregex const &e = rxmap[ re ];
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

}



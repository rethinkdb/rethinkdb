//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_LOCALE_TEST_H
#define BOOST_LOCALE_TEST_H
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <string>

int error_counter=0;
int test_counter=0;

#ifndef BOOST_LOCALE_ERROR_LIMIT
#define BOOST_LOCALE_ERROR_LIMIT 20
#endif


#define THROW_IF_TOO_BIG(X)                                                     \
do {                                                                            \
    if((X) > BOOST_LOCALE_ERROR_LIMIT)                                          \
        throw std::runtime_error("Error limits reached, stopping unit test");   \
}while(0)

#define TEST(X)                                                         \
    do {                                                                \
        test_counter++;                                                 \
        if(X) break;                                                    \
        std::cerr << "Error in line:"<<__LINE__ << " "#X  << std::endl; \
        THROW_IF_TOO_BIG(error_counter++);                              \
    }while(0)    
#endif

#define TEST_THROWS(X,E)                                                \
    do {                                                                \
        test_counter++;                                                 \
        try { X; } catch(E const &/*e*/ ) {break;} catch(...){}         \
        std::cerr << "Error in line:"<<__LINE__ << " "#X  << std::endl; \
        THROW_IF_TOO_BIG(error_counter++);                              \
    }while(0)    

#define FINALIZE()                                                      \
    do {                                                                \
        int passed=test_counter - error_counter;                        \
        std::cout << std::endl;                                         \
        std::cout << "Passed "<<passed<<" tests" << std::endl;          \
        if(error_counter >0 ) {                                         \
            std::cout << "Failed "<<error_counter<<" tests"<<std::endl; \
        }                                                               \
        std::cout <<" "<< std::fixed << std::setprecision(1)            \
                << std::setw(5) << 100.0 * passed / test_counter <<     \
                "% of tests completed sucsessefully" << std::endl;      \
        return error_counter == 0 ? EXIT_SUCCESS : EXIT_FAILURE ;       \
    }while(0)


inline unsigned utf8_next(std::string const &s,unsigned &pos)
{
    unsigned c=(unsigned char)s[pos++];
    if( (unsigned char)(c - 0xc0) >= 0x35)
        return c;
    unsigned l;
    if(c < 192)
        l = 0;
    else if(c < 224)
        l = 1;
    else if(c < 240)
        l = 2;
    else
        l = 3;
    
    c&=(1<<(6-l))-1;
    
    switch(l) {
    case 3:
        c = (c << 6) | (((unsigned char)s[pos++]) & 0x3F);
    case 2:
        c = (c << 6) | (((unsigned char)s[pos++]) & 0x3F);
    case 1:
        c = (c << 6) | (((unsigned char)s[pos++]) & 0x3F);
    }
    return c;
}

template<typename Char>
std::basic_string<Char> to(std::string const &utf8)
{
    std::basic_string<Char> out;
    unsigned i=0;
    while(i<utf8.size()) {
        unsigned point;
        unsigned prev=i;
        point = utf8_next(utf8,i);
        if(sizeof(Char)==1 && point > 255) {
            std::ostringstream ss;
            ss << "Can't convert codepoint U" << std::hex << point <<"(" <<std::string(utf8.begin()+prev,utf8.begin()+i)<<") to Latin1";
            throw std::runtime_error(ss.str());
        }
        else if(sizeof(Char)==2 && point >0xFFFF) { // Deal with surragates
            point-=0x10000;
            out+=static_cast<Char>(0xD800 | (point>>10));
            out+=static_cast<Char>(0xDC00 | (point & 0x3FF));
            continue;
        }
        out+=static_cast<Char>(point);
    }
    return out;
}


// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

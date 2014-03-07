//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#include "test_locale.hpp"
#include "test_locale_tools.hpp"
#include <boost/locale/utf.hpp>

#include <string.h>

using namespace boost::locale::utf;

char *make2(unsigned v)
{
    static unsigned char buf[3] = {0};
    buf[0] = 0xC0 | (v >> 6);
    buf[1] = 0x80 | (v & 0x3F );
    return reinterpret_cast<char*>(buf);
}

char *make3(unsigned v)
{
    static unsigned char buf[4] = {0};
    buf[0] = 0xE0 | ((v >> 12) ) ;
    buf[1] = 0x80 | ((v >>  6) & 0x3F );
    buf[2] = 0x80 | ((v >>  0) & 0x3F );
    return reinterpret_cast<char*>(buf);
}

char *make4(unsigned v)
{
    static unsigned char buf[5] = {0};
    buf[0] = 0xF0 | ((v >> 18) ) ;
    buf[1] = 0x80 | ((v >> 12) & 0x3F );
    buf[2] = 0x80 | ((v >>  6) & 0x3F );
    buf[3] = 0x80 | ((v >>  0) & 0x3F );
    return reinterpret_cast<char*>(buf);
}

boost::uint32_t const *u32_seq(boost::uint32_t a)
{
    static boost::uint32_t buf[2];
    buf[0]=a;
    buf[1]=0;
    return buf;
}

boost::uint16_t const *u16_seq(boost::uint16_t a)
{
    static boost::uint16_t buf[2];
    buf[0]=a;
    buf[1]=0;
    return buf;
}

boost::uint16_t const *u16_seq(boost::uint16_t a,boost::uint16_t b)
{
    static boost::uint16_t buf[3];
    buf[0]=a;
    buf[1]=b;
    buf[2]=0;
    return buf;
}

template<typename CharType>
void test_to(CharType const *s,unsigned codepoint)
{
    CharType const *begin = s;
    CharType const *end = begin;

    while(*end)
        end++;

    typedef utf_traits<CharType> tr;
    
    TEST(tr::max_width == 4 / sizeof(CharType));

    TEST(tr::template decode<CharType const *>(begin,end) == codepoint);

    if(codepoint == incomplete || codepoint != illegal)
        TEST(end == begin);

    if(codepoint == incomplete) {
        TEST(*s== 0 || 0 < tr::trail_length(*s));
        TEST(tr::trail_length(*s) + 1 > end - s);
    }

    if(codepoint != incomplete && codepoint != illegal) {
        begin=s;
        TEST(tr::is_lead(*begin));
        TEST(!tr::is_trail(*begin));
        begin++;
        while(begin!=end) {
            TEST(tr::is_trail(*begin));
            TEST(!tr::is_lead(*begin));
            begin++;
        }
        TEST(tr::width(codepoint)==end - s);
        TEST(tr::trail_length(*s) == tr::width(codepoint) - 1);
        begin = s;
        TEST(tr::decode_valid(begin) == codepoint);
        TEST(begin == end);
    }
}

template<typename CharType>
void test_from(CharType const *str,unsigned codepoint)
{
    CharType buf[5] = {1,1,1,1,1};
    CharType *p=buf;
    p = utf_traits<CharType>::template encode<CharType *>(codepoint,p);
    CharType const *end = str;
    while(*end)
        end++;
    TEST(end - str == p-buf );
    TEST(*p);
    *p=0;
    TEST(memcmp(str,buf,sizeof(CharType) * (end-str))==0);
}


int main()
{
    try {

        std::cout << "Test UTF-8" << std::endl;
        std::cout << "- From UTF-8" << std::endl;


        std::cout << "-- Correct" << std::endl;

        test_to("\x7f",0x7f);
        test_to("\xc2\x80",0x80);
        test_to("\xdf\xbf",0x7ff);
        test_to("\xe0\xa0\x80",0x800);
        test_to("\xef\xbf\xbf",0xffff);
        test_to("\xf0\x90\x80\x80",0x10000);
        test_to("\xf4\x8f\xbf\xbf",0x10ffff);

        std::cout << "-- Too big" << std::endl;
        test_to("\xf4\x9f\x80\x80",illegal); //  11 0000
        test_to("\xfb\xbf\xbf\xbf",illegal); // 3ff ffff
        test_to("\xf8\x90\x80\x80\x80",illegal);  // 400 0000
        test_to("\xfd\xbf\xbf\xbf\xbf\xbf",illegal);  // 7fff ffff

        std::cout << "-- Invalid length" << std::endl;

        /// test that this actually works
        test_to(make2(0x80),0x80);
        test_to(make2(0x7ff),0x7ff);

        test_to(make3(0x800),0x800);
        test_to(make3(0xffff),0xffff);

        test_to(make4(0x10000),0x10000);
        test_to(make4(0x10ffff),0x10ffff);

        test_to(make4(0x110000),illegal);
        test_to(make4(0x1fffff),illegal);
        
        test_to(make2(0),illegal);
        test_to(make3(0),illegal);
        test_to(make4(0),illegal);
        test_to(make2(0x7f),illegal);
        test_to(make3(0x7f),illegal);
        test_to(make4(0x7f),illegal);

        test_to(make3(0x80),illegal);
        test_to(make4(0x80),illegal);
        test_to(make3(0x7ff),illegal);
        test_to(make4(0x7ff),illegal);

        test_to(make4(0x8000),illegal);
        test_to(make4(0xffff),illegal);
        
        std::cout << "-- Invalid surrogate" << std::endl;
        
        test_to(make3(0xd800),illegal);
        test_to(make3(0xdbff),illegal);
        test_to(make3(0xdc00),illegal);
        test_to(make3(0xdfff),illegal);

        test_to(make4(0xd800),illegal);
        test_to(make4(0xdbff),illegal);
        test_to(make4(0xdc00),illegal);
        test_to(make4(0xdfff),illegal);

        std::cout <<"-- Incomplete" << std::endl;

        test_to("",incomplete);

        test_to("\x80",illegal);
        test_to("\xc2",incomplete);
        
        test_to("\xdf",incomplete);

        test_to("\xe0",incomplete);
        test_to("\xe0\xa0",incomplete);

        test_to("\xef\xbf",incomplete);
        test_to("\xef",incomplete);

        test_to("\xf0\x90\x80",incomplete);
        test_to("\xf0\x90",incomplete);
        test_to("\xf0",incomplete);

        test_to("\xf4\x8f\xbf",incomplete);
        test_to("\xf4\x8f",incomplete);
        test_to("\xf4",incomplete);

        std::cout << "- To UTF-8" << std::endl;

        std::cout << "-- Test correct" << std::endl;

        test_from("\x7f",0x7f);
        test_from("\xc2\x80",0x80);
        test_from("\xdf\xbf",0x7ff);
        test_from("\xe0\xa0\x80",0x800);
        test_from("\xef\xbf\xbf",0xffff);
        test_from("\xf0\x90\x80\x80",0x10000);
        test_from("\xf4\x8f\xbf\xbf",0x10ffff);

        std::cout << "Test UTF-16" << std::endl;
        std::cout << "- From UTF-16" << std::endl;


        std::cout << "-- Correct" << std::endl;

        test_to(u16_seq(0x10),0x10);
        test_to(u16_seq(0xffff),0xffff);
        test_to(u16_seq(0xD800,0xDC00),0x10000);
        test_to(u16_seq(0xDBFF,0xDFFF),0x10FFFF);


        std::cout << "-- Invalid surrogate" << std::endl;
        
        test_to(u16_seq(0xDFFF),illegal);
        test_to(u16_seq(0xDC00),illegal);

        std::cout <<"-- Incomplete" << std::endl;

        test_to(u16_seq(0),incomplete);
        test_to(u16_seq(0xD800),incomplete);
        test_to(u16_seq(0xDBFF),incomplete);

        std::cout << "- To UTF-16" << std::endl;

        std::cout << "-- Test correct" << std::endl;

        test_to(u16_seq(0x10),0x10);
        test_to(u16_seq(0xffff),0xffff);
        test_to(u16_seq(0xD800,0xDC00),0x10000);
        test_to(u16_seq(0xDBFF,0xDFFF),0x10FFFF);


        std::cout << "Test UTF-32" << std::endl;
        std::cout << "- From UTF-32" << std::endl;


        std::cout << "-- Correct" << std::endl;

        test_to(u32_seq(0x10),0x10);
        test_to(u32_seq(0xffff),0xffff);
        test_to(u32_seq(0x10000),0x10000);
        test_to(u32_seq(0x10ffff),0x10ffff);



        std::cout << "-- Invalid surrogate" << std::endl;
        
        test_to(u32_seq(0xD800),illegal);
        test_to(u32_seq(0xDBFF),illegal);
        test_to(u32_seq(0xDFFF),illegal);
        test_to(u32_seq(0xDC00),illegal);
        test_to(u32_seq(0x110000),illegal);

        std::cout <<"-- Incomplete" << std::endl;

        test_to(u32_seq(0),incomplete);

        std::cout << "- To UTF-32" << std::endl;

        std::cout << "-- Test correct" << std::endl;

        test_to(u32_seq(0x10),0x10);
        test_to(u32_seq(0xffff),0xffff);
        test_to(u32_seq(0x10ffff),0x10ffff);



    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
// boostinspect:noascii 

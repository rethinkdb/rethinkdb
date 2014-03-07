//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/locale/encoding.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/locale/info.hpp>
#include <boost/locale/config.hpp>
#include <fstream>
#include "test_locale.hpp"
#include "test_locale_tools.hpp"


#ifndef BOOST_LOCALE_NO_POSIX_BACKEND
# ifdef __APPLE__
#  include <xlocale.h>
#  endif
# include <locale.h>
#endif

#if !defined(BOOST_LOCALE_WITH_ICU) && !defined(BOOST_LOCALE_WITH_ICONV) && (defined(BOOST_WINDOWS) || defined(__CYGWIN__))
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <windows.h>
#endif


bool test_iso;
bool test_iso_8859_8 = true;
bool test_utf;
bool test_sjis;

std::string he_il_8bit;
std::string en_us_8bit;
std::string ja_jp_shiftjis;


template<typename Char>
std::basic_string<Char> read_file(std::basic_istream<Char> &in)
{
    std::basic_string<Char> res;
    Char c;
    while(in.get(c))
        res+=c;
    return res;
}


template<typename Char>
void test_ok(std::string file,std::locale const &l,std::basic_string<Char> cmp=std::basic_string<Char>())
{
    if(cmp.empty())
        cmp=to<Char>(file);
    std::ofstream test("testi.txt");
    test << file;
    test.close();
    typedef std::basic_fstream<Char> stream_type;

    stream_type f1("testi.txt",stream_type::in);
    f1.imbue(l);
    TEST(read_file<Char>(f1) == cmp); 
    f1.close();

    stream_type f2("testo.txt",stream_type::out);
    f2.imbue(l);
    f2 << cmp;
    f2.close();

    std::ifstream testo("testo.txt");
    TEST(read_file<char>(testo) == file);
}

template<typename Char>
void test_rfail(std::string file,std::locale const &l,int pos)
{
    std::ofstream test("testi.txt");
    test << file;
    test.close();
    typedef std::basic_fstream<Char> stream_type;

    stream_type f1("testi.txt",stream_type::in);
    f1.imbue(l);
    Char c;
    for(int i=0;i<pos;i++) {
        f1.get(c);
        if(f1.fail()) { // failed before as detected errors at forward;
            return;
        }
        TEST(f1);
    }
    // if the pos above suceed, at this point
    // it MUST fail
    TEST(f1.get(c).fail());
}

template<typename Char>
void test_wfail(std::string file,std::locale const &l,int pos)
{
    typedef std::basic_fstream<Char> stream_type;
    stream_type f1("testo.txt",stream_type::out);
    f1.imbue(l);
    std::basic_string<Char> out=to<Char>(file);
    int i;
    for(i=0;i<pos;i++) {
        f1 << out.at(i);
        f1<<std::flush;
        TEST(f1.good());
    }
    f1 << out.at(i);
    TEST(f1.fail() || (f1<<std::flush).fail());
}


template<typename Char>
void test_for_char()
{
    boost::locale::generator g;
    if(test_utf) {
        std::cout << "    UTF-8" << std::endl;
        test_ok<Char>("grüße\nn i",g("en_US.UTF-8"));
        test_rfail<Char>("abc\xFF\xFF",g("en_US.UTF-8"),3);
        std::cout << "    Testing codepoints above 0xFFFF" << std::endl;
        std::cout << "      Single U+2008A" << std::endl;
        test_ok<Char>("\xf0\xa0\x82\x8a",g("en_US.UTF-8")); // U+2008A
        std::cout << "      Single U+2008A withing text" << std::endl;
        test_ok<Char>("abc\"\xf0\xa0\x82\x8a\"",g("en_US.UTF-8")); // U+2008A
        std::string one = "\xf0\xa0\x82\x8a";
        std::string res;
        for(unsigned i=0;i<1000;i++)
            res+=one;
        std::cout << "      U+2008A x 1000" << std::endl;
        test_ok<Char>(res.c_str(),g("en_US.UTF-8")); // U+2008A
    }
    else {
        std::cout << "    UTF-8 Not supported " << std::endl;
    }
    
    if(test_iso) {
        if(test_iso_8859_8) {
            std::cout << "    ISO8859-8" << std::endl;
            test_ok<Char>("hello \xf9\xec\xe5\xed",g(he_il_8bit),to<Char>("hello שלום"));
        }
        std::cout << "    ISO8859-1" << std::endl;
        test_ok<Char>(to<char>("grüße\nn i"),g(en_us_8bit),to<Char>("grüße\nn i"));
        test_wfail<Char>("grüßen שלום",g(en_us_8bit),7);
    }

    if(test_sjis) {
        std::cout << "    Shift-JIS" << std::endl;
        test_ok<Char>("\x93\xfa\x96\x7b",g(ja_jp_shiftjis),
                boost::locale::conv::to_utf<Char>("\xe6\x97\xa5\xe6\x9c\xac","UTF-8"));  // Japan
    }
}
void test_wide_io()
{
    std::cout << "  wchar_t" << std::endl;
    test_for_char<wchar_t>();
    
    #if defined BOOST_HAS_CHAR16_T && !defined(BOOST_NO_CHAR16_T_CODECVT)
    std::cout << "  char16_t" << std::endl;
    test_for_char<char16_t>();
    #endif
    #if defined BOOST_HAS_CHAR32_T && !defined(BOOST_NO_CHAR32_T_CODECVT)
    std::cout << "  char32_t" << std::endl;
    test_for_char<char32_t>();
    #endif
}

template<typename Char>
void test_pos(std::string source,std::basic_string<Char> target,std::string encoding)
{
    using namespace boost::locale::conv;
    boost::locale::generator g;
    std::locale l= encoding == "ISO8859-8" ? g("he_IL."+encoding) : g("en_US."+encoding);
    TEST(to_utf<Char>(source,encoding)==target);
    TEST(to_utf<Char>(source.c_str(),encoding)==target);
    TEST(to_utf<Char>(source.c_str(),source.c_str()+source.size(),encoding)==target);
    
    TEST(to_utf<Char>(source,l)==target);
    TEST(to_utf<Char>(source.c_str(),l)==target);
    TEST(to_utf<Char>(source.c_str(),source.c_str()+source.size(),l)==target);

    TEST(from_utf<Char>(target,encoding)==source);
    TEST(from_utf<Char>(target.c_str(),encoding)==source);
    TEST(from_utf<Char>(target.c_str(),target.c_str()+target.size(),encoding)==source);
    
    TEST(from_utf<Char>(target,l)==source);
    TEST(from_utf<Char>(target.c_str(),l)==source);
    TEST(from_utf<Char>(target.c_str(),target.c_str()+target.size(),l)==source);
}

#define TESTF(X) TEST_THROWS(X,boost::locale::conv::conversion_error)

template<typename Char>
void test_to_neg(std::string source,std::basic_string<Char> target,std::string encoding)
{
    using namespace boost::locale::conv;
    boost::locale::generator g;
    std::locale l=g("en_US."+encoding);

    TEST(to_utf<Char>(source,encoding)==target);
    TEST(to_utf<Char>(source.c_str(),encoding)==target);
    TEST(to_utf<Char>(source.c_str(),source.c_str()+source.size(),encoding)==target);
    TEST(to_utf<Char>(source,l)==target);
    TEST(to_utf<Char>(source.c_str(),l)==target);
    TEST(to_utf<Char>(source.c_str(),source.c_str()+source.size(),l)==target);

    TESTF(to_utf<Char>(source,encoding,stop));
    TESTF(to_utf<Char>(source.c_str(),encoding,stop));
    TESTF(to_utf<Char>(source.c_str(),source.c_str()+source.size(),encoding,stop));
    TESTF(to_utf<Char>(source,l,stop));
    TESTF(to_utf<Char>(source.c_str(),l,stop));
    TESTF(to_utf<Char>(source.c_str(),source.c_str()+source.size(),l,stop));
}

template<typename Char>
void test_from_neg(std::basic_string<Char> source,std::string target,std::string encoding)
{
    using namespace boost::locale::conv;
    boost::locale::generator g;
    std::locale l=g("en_US."+encoding);

    TEST(from_utf<Char>(source,encoding)==target);
    TEST(from_utf<Char>(source.c_str(),encoding)==target);
    TEST(from_utf<Char>(source.c_str(),source.c_str()+source.size(),encoding)==target);
    TEST(from_utf<Char>(source,l)==target);
    TEST(from_utf<Char>(source.c_str(),l)==target);
    TEST(from_utf<Char>(source.c_str(),source.c_str()+source.size(),l)==target);

    TESTF(from_utf<Char>(source,encoding,stop));
    TESTF(from_utf<Char>(source.c_str(),encoding,stop));
    TESTF(from_utf<Char>(source.c_str(),source.c_str()+source.size(),encoding,stop));
    TESTF(from_utf<Char>(source,l,stop));
    TESTF(from_utf<Char>(source.c_str(),l,stop));
    TESTF(from_utf<Char>(source.c_str(),source.c_str()+source.size(),l,stop));
}

template<typename Char>
std::basic_string<Char> utf(char const *s)
{
    return to<Char>(s);
}

template<>
std::basic_string<char> utf(char const *s)
{
    return s;
}

template<typename Char>
void test_with_0()
{
    std::string a("abc\0\0 yz\0",3+2+3+1);
    TEST(boost::locale::conv::from_utf<Char>(boost::locale::conv::to_utf<Char>(a,"UTF-8"),"UTF-8") == a);
    TEST(boost::locale::conv::from_utf<Char>(boost::locale::conv::to_utf<Char>(a,"ISO8859-1"),"ISO8859-1") == a);
}

template<typename Char,int n=sizeof(Char)>
struct utfutf;

template<>
struct utfutf<char,1> {
    static char const *ok() {return "grüßen";}
    static char const *bad() { return "gr\xFF" "üßen"; }
                                // split into 2 to make SunCC happy
};

template<>
struct utfutf<wchar_t,2> {
    static wchar_t const *ok(){ return  L"\x67\x72\xfc\xdf\x65\x6e"; }
    static wchar_t const *bad() { 
        static wchar_t buf[256] = L"\x67\x72\xFF\xfc\xFE\xFD\xdf\x65\x6e"; 
        buf[2]=0xDC01; // second surrogate must not be
        buf[4]=0xD801; // First
        buf[5]=0xD801; // Must be surrogate trail
        return buf;
    }
};
template<>
struct utfutf<wchar_t,4> {
    static wchar_t const *ok(){ return  L"\x67\x72\xfc\xdf\x65\x6e"; }
    static wchar_t const *bad() { 
        static wchar_t buf[256] = L"\x67\x72\xFF\xfc\xdf\x65\x6e"; 
        buf[2]=static_cast<wchar_t>(0x1000000); // > 10FFFF
        return buf;
    }
};


template<typename CharOut,typename CharIn>
void test_combinations()
{
    using boost::locale::conv::utf_to_utf;
    typedef utfutf<CharOut> out;
    typedef utfutf<CharIn> in;
    TEST( (utf_to_utf<CharOut,CharIn>(in::ok())==out::ok()) );
    TESTF( (utf_to_utf<CharOut,CharIn>(in::bad(),boost::locale::conv::stop)) );
    TEST( (utf_to_utf<CharOut,CharIn>(in::bad())==out::ok()) );
}

void test_all_combinations()
{
    std::cout << "Testing utf_to_utf" << std::endl;
    std::cout <<"  char<-char"<<std::endl;
    test_combinations<char,char>();
    std::cout <<"  char<-wchar"<<std::endl;
    test_combinations<char,wchar_t>();
    std::cout <<"  wchar<-char"<<std::endl;
    test_combinations<wchar_t,char>();
    std::cout <<"  wchar<-wchar"<<std::endl;
    test_combinations<wchar_t,wchar_t>();
}

template<typename Char>
void test_to()
{
    test_pos<Char>(to<char>("grüßen"),utf<Char>("grüßen"),"ISO8859-1");
    if(test_iso_8859_8)
        test_pos<Char>("\xf9\xec\xe5\xed",utf<Char>("שלום"),"ISO8859-8");
    test_pos<Char>("grüßen",utf<Char>("grüßen"),"UTF-8");
    test_pos<Char>("abc\"\xf0\xa0\x82\x8a\"",utf<Char>("abc\"\xf0\xa0\x82\x8a\""),"UTF-8");
    
    test_to_neg<Char>("g\xFFrüßen",utf<Char>("grüßen"),"UTF-8");
    test_from_neg<Char>(utf<Char>("hello שלום"),"hello ","ISO8859-1");
 
    test_with_0<Char>();
}


void test_skip(char const *enc,char const *utf,char const *name,char const *opt=0)
{
    if(opt!=0) {
        if(boost::locale::conv::to_utf<char>(enc,name) == opt) {
            test_skip(enc,opt,name);
            return;
        }
    }
    TEST(boost::locale::conv::to_utf<char>(enc,name) == utf);
    TEST(boost::locale::conv::to_utf<wchar_t>(enc,name) == boost::locale::conv::utf_to_utf<wchar_t>(utf));
    #ifdef BOOST_HAS_CHAR16_T
    TEST(boost::locale::conv::to_utf<char16_t>(enc,name) == boost::locale::conv::utf_to_utf<char16_t>(utf));
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    TEST(boost::locale::conv::to_utf<char32_t>(enc,name) == boost::locale::conv::utf_to_utf<char32_t>(utf));
    #endif
}

void test_simple_conversions()
{
    namespace blc=boost::locale::conv;
    std::cout << "- Testing correct invalid bytes skipping" << std::endl;
    try {
        std::cout << "-- ISO-8859-8" << std::endl;
        test_skip("test \xE0\xE1\xFB-","test \xd7\x90\xd7\x91-","ISO-8859-8");
        test_skip("\xFB","","ISO-8859-8");
        test_skip("test \xE0\xE1\xFB","test \xd7\x90\xd7\x91","ISO-8859-8");
        test_skip("\xFB-","-","ISO-8859-8");
    }
    catch(blc::invalid_charset_error const &) {
        std::cout <<"--- not supported" << std::endl;
    }
    try {
        std::cout << "-- cp932" << std::endl;
        test_skip("test\xE0\xA0 \x83\xF8-","test\xe7\x87\xbf -","cp932","test\xe7\x87\xbf ");
        test_skip("\x83\xF8","","cp932");
        test_skip("test\xE0\xA0 \x83\xF8","test\xe7\x87\xbf ","cp932");
        test_skip("\x83\xF8-","-","cp932","");
    }
    catch(blc::invalid_charset_error const &) {
        std::cout <<"--- not supported" << std::endl;
    }
}


int main()
{
    try {
        std::vector<std::string> def;
        #ifdef BOOST_LOCALE_WITH_ICU
        def.push_back("icu");
        #endif
        #ifndef BOOST_LOCALE_NO_STD_BACKEND
        def.push_back("std");
        #endif
        #ifndef BOOST_LOCALE_NO_WINAPI_BACKEND
        def.push_back("winapi");
        #endif
        #ifndef BOOST_LOCALE_NO_POSIX_BACKEND
        def.push_back("posix");
        #endif

        #if !defined(BOOST_LOCALE_WITH_ICU) && !defined(BOOST_LOCALE_WITH_ICONV) && (defined(BOOST_WINDOWS) || defined(__CYGWIN__))
        test_iso_8859_8 = IsValidCodePage(28598)!=0;
        #endif

        test_simple_conversions();
        
        
        for(int type = 0; type < int(def.size()); type ++ ) {
            boost::locale::localization_backend_manager tmp_backend = boost::locale::localization_backend_manager::global();
            tmp_backend.select(def[type]);
            boost::locale::localization_backend_manager::global(tmp_backend);
            
            std::string bname = def[type];
            
            if(bname=="std") { 
                en_us_8bit = get_std_name("en_US.ISO8859-1");
                he_il_8bit = get_std_name("he_IL.ISO8859-8");
                ja_jp_shiftjis = get_std_name("ja_JP.SJIS");
            }
            else {
                en_us_8bit = "en_US.ISO8859-1";
                he_il_8bit = "he_IL.ISO8859-8";
                ja_jp_shiftjis = "ja_JP.SJIS";
            }

            std::cout << "Testing for backend " << def[type] << std::endl;

            test_iso = true;
            if(bname=="std" && (he_il_8bit.empty() || en_us_8bit.empty())) {
                std::cout << "no iso locales availible, passing" << std::endl;
                test_iso = false;
            }
            test_sjis = true;
            if(bname=="std" && ja_jp_shiftjis.empty()) {
                test_sjis = false;
            }
            if(bname=="winapi") {
                test_iso = false;
                test_sjis = false;
            }
            test_utf = true;
            #ifndef BOOST_LOCALE_NO_POSIX_BACKEND
            if(bname=="posix") {
                {
                    locale_t l = newlocale(LC_ALL_MASK,he_il_8bit.c_str(),0);
                    if(!l)
                        test_iso = false;
                    else
                        freelocale(l);
                }
                {
                    locale_t l = newlocale(LC_ALL_MASK,en_us_8bit.c_str(),0);
                    if(!l)
                        test_iso = false;
                    else
                        freelocale(l);
                }
                {
                    locale_t l = newlocale(LC_ALL_MASK,"en_US.UTF-8",0);
                    if(!l)
                        test_utf = false;
                    else
                        freelocale(l);
                }
                #ifdef BOOST_LOCALE_WITH_ICONV
                {
                    locale_t l = newlocale(LC_ALL_MASK,ja_jp_shiftjis.c_str(),0);
                    if(!l)
                        test_sjis = false;
                    else
                        freelocale(l);
                }
                #else
                test_sjis = false;
                #endif
            }
            #endif

            if(def[type]=="std" && (get_std_name("en_US.UTF-8").empty() || get_std_name("he_IL.UTF-8").empty())) 
            {
                test_utf = false;
            }
            
            std::cout << "Testing wide I/O" << std::endl;
            test_wide_io();
            std::cout << "Testing charset to/from UTF conversion functions" << std::endl;
            std::cout << "  char" << std::endl;
            test_to<char>();
            std::cout << "  wchar_t" << std::endl;
            test_to<wchar_t>();
            #ifdef BOOST_HAS_CHAR16_T
            if(bname == "icu" || bname == "std") {
                std::cout << "  char16_t" << std::endl;
                test_to<char16_t>();
            }
            #endif
            #ifdef BOOST_HAS_CHAR32_T
            if(bname == "icu" || bname == "std") {
                std::cout << "  char32_t" << std::endl;
                test_to<char32_t>();
            }
            #endif

            test_all_combinations();
        }
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();
}

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
// boostinspect:noascii 

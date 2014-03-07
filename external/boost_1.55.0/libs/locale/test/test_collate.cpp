//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_LOCALE_WITH_ICU
#include <iostream>
int main()
{
        std::cout << "ICU is not build... Skipping" << std::endl;
}
#else

#include <boost/locale/collator.hpp>
#include <boost/locale/generator.hpp>
#include <iomanip>
#include "test_locale.hpp"


template<typename Char>
void test_comp(std::locale l,std::basic_string<Char> left,std::basic_string<Char> right,int ilevel,int expected)
{
    typedef std::basic_string<Char> string_type;
    boost::locale::collator_base::level_type level = static_cast<boost::locale::collator_base::level_type>(ilevel);
    TEST(boost::locale::comparator<Char>(l,level)(left,right) == (expected < 0));
    if(ilevel==4) {
        std::collate<Char> const &coll=std::use_facet<std::collate<Char> >(l);
        string_type lt=coll.transform(left.c_str(),left.c_str()+left.size());
        string_type rt=coll.transform(right.c_str(),right.c_str()+right.size());
        if(expected < 0)
            TEST(lt<rt);
        else if(expected == 0) {
            TEST(lt==rt);
        }
        else 
            TEST(lt > rt);
        long lh=coll.hash(left.c_str(),left.c_str()+left.size());
        long rh=coll.hash(right.c_str(),right.c_str()+right.size());
        if(expected == 0)
            TEST(lh==rh);
        else
            TEST(lh!=rh);
    }
    boost::locale::collator<Char> const &coll=std::use_facet<boost::locale::collator<Char> >(l);
    string_type lt=coll.transform(level,left.c_str(),left.c_str()+left.size());
    TEST(lt==coll.transform(level,left));
    string_type rt=coll.transform(level,right.c_str(),right.c_str()+right.size());
    TEST(rt==coll.transform(level,right));
    if(expected < 0)
        TEST(lt<rt);
    else if(expected == 0)
        TEST(lt==rt);
    else 
        TEST(lt > rt);
    long lh=coll.hash(level,left.c_str(),left.c_str()+left.size());
    TEST(lh==coll.hash(level,left));
    long rh=coll.hash(level,right.c_str(),right.c_str()+right.size());
    TEST(rh==coll.hash(level,right));
    if(expected == 0)
        TEST(lh==rh);
    else
        TEST(lh!=rh);

}    
        
#define TEST_COMP(c,_l,_r) test_comp<c>(l,_l,_r,level,expected)


void compare(std::string left,std::string right,int level,int expected)
{
    boost::locale::generator gen;
    std::locale l=gen("en_US.UTF-8");
    if(level == 4)
        TEST(l(left,right) == (expected < 0));
    TEST_COMP(char,left,right);
    TEST_COMP(wchar_t,to<wchar_t>(left),to<wchar_t>(right));
    #ifdef BOOST_HAS_CHAR16_T
    TEST_COMP(char16_t,to<char16_t>(left),to<char16_t>(right));
    #endif
    #ifdef BOOST_HAS_CHAR32_T
    TEST_COMP(char32_t,to<char32_t>(left),to<char32_t>(right));
    #endif
    l=gen("en_US.ISO8859-1");
    if(level == 4)
        TEST(l(to<char>(left),to<char>(right)) == (expected < 0));
    TEST_COMP(char,to<char>(left),to<char>(right));
}


void test_collate()
{
    int
        primary     = 0,
        secondary   = 1,
        tertiary    = 2,
        quaternary  = 3,
        identical   = 4;
    int     le = -1,gt = 1,eq = 0;


    compare("a","A",primary,eq);
    compare("a","A",secondary,eq);
    compare("A","a",tertiary,gt);
    compare("a","A",tertiary,le);
    compare("a","A",quaternary,le);
    compare("A","a",quaternary,gt);
    compare("a","A",identical,le);
    compare("A","a",identical,gt);
    compare("a","ä",primary,eq); //  a , ä
    compare("a","ä",secondary,le); //  a , ä
    compare("ä","a",secondary,gt); //  a , ä
    compare("a","ä",quaternary,le); //  a , ä
    compare("ä","a",quaternary,gt); //  a , ä
    compare("a","ä",identical,le); //  a , ä
    compare("ä","a",identical,gt); //  a , ä
}




int main()
{
    try {
        test_collate();
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();

}

#endif // NOICU
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
// boostinspect:noascii 

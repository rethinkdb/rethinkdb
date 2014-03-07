//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifdef BOOST_LOCALE_NO_POSIX_BACKEND
#include <iostream>
int main()
{
        std::cout << "POSIX Backend is not build... Skipping" << std::endl;
}
#else
#include <boost/locale/conversion.hpp>
#include <boost/locale/localization_backend.hpp>
#include <boost/locale/generator.hpp>
#include <boost/locale/info.hpp>
#include <iomanip>
#include "test_locale.hpp"
#include "test_locale_tools.hpp"
#include "test_posix_tools.hpp"
#include <iostream>

#include <wctype.h>


template<typename CharType>
void test_one(std::locale const &l,std::string src,std::string tgtl,std::string tgtu)
{
    TEST(boost::locale::to_upper(to_correct_string<CharType>(src,l),l) == to_correct_string<CharType>(tgtu,l));
    TEST(boost::locale::to_lower(to_correct_string<CharType>(src,l),l) == to_correct_string<CharType>(tgtl,l));
    TEST(boost::locale::fold_case(to_correct_string<CharType>(src,l),l) == to_correct_string<CharType>(tgtl,l));
}

template<typename CharType>
void test_char()
{
    boost::locale::generator gen;

    std::cout << "- Testing at least C" << std::endl;

    std::locale l = gen("en_US.UTF-8");

    test_one<CharType>(l,"Hello World i","hello world i","HELLO WORLD I");

    std::string name = "en_US.UTF-8";
    if(have_locale(name)) {
        std::cout << "- Testing " << name << std::endl;
        std::locale l=gen(name);
        test_one<CharType>(l,"Façade","façade","FAÇADE");
    }
    else {
        std::cout << "- en_US.UTF-8 is not supported, skipping" << std::endl;
    }

    name = "en_US.ISO8859-1";
    if(have_locale(name)) {
        std::cout << "Testing " << name << std::endl;
        std::locale l=gen(name);
        test_one<CharType>(l,"Hello World","hello world","HELLO WORLD");
        #ifdef __APPLE__
        if(sizeof(CharType)!=1)
        #endif
            test_one<CharType>(l,"Façade","façade","FAÇADE");
    }
    else {
        std::cout << "- en_US.ISO8859-1 is not supported, skipping" << std::endl;
    }
    
    name = "tr_TR.UTF-8";
    if(have_locale(name)) {
        std::cout << "Testing " << name << std::endl;
        locale_t cl = newlocale(LC_ALL_MASK,name.c_str(),0);
        try { 
            TEST(cl);
            if(towupper_l(L'i',cl) == 0x130) {
                test_one<CharType>(gen(name),"i","i","İ");
            }
            else {
                std::cout <<"  Turkish locale is not supported well" << std::endl;
            }
        }
        catch(...) {
            if(cl) freelocale(cl);
            throw;
        }
        if(cl) freelocale(cl);
        
    }
    else 
    {
        std::cout << "- tr_TR.UTF-8 is not supported, skipping" << std::endl;
    }
}


int main()
{
    try {
        boost::locale::localization_backend_manager mgr = boost::locale::localization_backend_manager::global();
        mgr.select("posix");
        boost::locale::localization_backend_manager::global(mgr);

        std::cout << "Testing char" << std::endl;
        test_char<char>();
        std::cout << "Testing wchar_t" << std::endl;
        test_char<wchar_t>();
    }
    catch(std::exception const &e) {
        std::cerr << "Failed " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    FINALIZE();

}

#endif // POSIX
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4


// boostinspect:noascii 

//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_LOCLAE_TEST_LOCALE_TOOLS_HPP
#define BOOST_LOCLAE_TEST_LOCALE_TOOLS_HPP

#include <boost/locale/encoding.hpp>

template<typename Char>
std::basic_string<Char> to_correct_string(std::string const &e,std::locale /*l*/)
{
    return boost::locale::conv::to_utf<Char>(e,"UTF-8");
}


template<>
inline std::string to_correct_string(std::string const &e,std::locale l)
{
    return boost::locale::conv::from_utf(e,l);
}

bool has_std_locale(std::string const &name)
{
    try {
        std::locale tmp(name.c_str());
        return true;
    }
    catch(...) {
        return false;
    }
}

std::string get_std_name(std::string const &name,std::string *real_name = 0)
{
    if(has_std_locale(name)) {
        if(real_name)
            *real_name = name;
        return name;
    }

    #ifdef BOOST_WINDOWS
    bool utf8=name.find("UTF-8")!=std::string::npos;

    if(name=="en_US.UTF-8" || name == "en_US.ISO8859-1") {
        if(has_std_locale("English_United States.1252")) {
            if(real_name) 
                *real_name = "English_United States.1252";
            return utf8 ? name : "en_US.windows-1252";
        }
        return "";
    }
    else if(name=="he_IL.UTF-8" || name == "he_IL.ISO8859-8")  {
        if(has_std_locale("Hebrew_Israel.1255")) {
            if(real_name) 
                *real_name = "Hebrew_Israel.1255";
            return utf8 ? name : "he_IL.windows-1255";
            return name;
        }
    }
    else if(name=="ru_RU.UTF-8")  {
        if(has_std_locale("Russian_Russia.1251")) {
            if(real_name) 
                *real_name = "Russian_Russia.1251";
            return name;
        }
    }
    else if(name == "tr_TR.UTF-8") {
        if(has_std_locale("Turkish_Turkey.1254")) {
            if(real_name) 
                *real_name = "Turkish_Turkey.1254";
            return name;
        }
    }
    if(name == "ja_JP.SJIS") {
        if(has_std_locale("Japanese_Japan.932")) {
            if(real_name) 
                *real_name = "Japanese_Japan.932";
            return name;
        }
        return "";
    }
    #endif
    return "";
}



#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

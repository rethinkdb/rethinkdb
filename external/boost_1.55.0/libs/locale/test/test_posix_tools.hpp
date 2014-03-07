//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_LOCLAE_TEST_LOCALE_POSIX_TOOLS_HPP
#define BOOST_LOCLAE_TEST_LOCALE_POSIX_TOOLS_HPP

#include <locale.h>
#include <string>

#ifdef __APPLE__
#include <xlocale.h>
#endif

inline bool have_locale(std::string const &name)
{
    locale_t l=newlocale(LC_ALL_MASK,name.c_str(),0);
    if(l) {
        freelocale(l);
        return true;
    }
    return false;
}

#endif
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4


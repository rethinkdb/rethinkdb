//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#include <iostream>
#include <string>
#include <set>

#include <boost/locale.hpp>

using namespace std;
using namespace boost::locale;

int main(int argc,char **argv)
{
    if(argc!=2) {
        std::cerr << "Usage backend locale" << std::endl;
        return 1;
    }
    boost::locale::localization_backend_manager mgr = boost::locale::localization_backend_manager::global();
    mgr.select(argv[1]);
    generator gen(mgr);
    std::locale::global(gen(argv[2]));
    /// Set global locale to requested

    /// Create a set that includes all strings sorted according to ABC order
    /// std::locale can be used as object for comparison
    std::vector<std::string> all;
    typedef std::set<std::string,std::locale> set_type;
    set_type all_strings;

    /// Read all strings into the set
    while(!cin.eof()) {
        std::string tmp;
        getline(cin,tmp);
        all.push_back(tmp);
    }
    for(int i=0;i<10000;i++) {
        std::vector<std::string> tmp = all;
        std::sort(tmp.begin(),tmp.end(),std::locale());
        if(i==0) {
            for(unsigned j=0;j<tmp.size();j++)
                std::cout << tmp[j] << std::endl;
        }
    }

}
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

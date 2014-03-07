//  Copyright (c) 2005 Carl Barron. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "tag.hpp"
#include <iostream>
#include <algorithm>

namespace
{
    struct print_pair
    {
        template <class P>
        void operator () (const P &x)
        {
            std::cout << '\t' << x.first << ':' << x.second <<'\n';
        }
    };
    
}

void walk_data::operator () (const std::string &x)
{
    std::cout << "String:" << x <<'\n';
}

void walk_data::operator () (const tag &t)
{
    std::cout << "Tag:" << t.id << '\n';
    std::cout << "Attributes\n";
    
    std::for_each
    (
        t.attributes.begin(),
        t.attributes.end(),
        print_pair()
    );
    std::cout << "Children:\n";
    std::for_each
    (
        t.children.begin(),
        t.children.end(),
        boost::apply_visitor(*this)
    );
    std::cout << "End of tag:" << t.id << '\n';
}

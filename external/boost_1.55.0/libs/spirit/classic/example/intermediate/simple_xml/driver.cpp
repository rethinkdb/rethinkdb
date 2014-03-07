//  Copyright (c) 2005 Carl Barron. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "xml_g.hpp"
#include <boost/spirit/include/classic_utility.hpp>
#include <iostream>

namespace std
{
    std::ostream & operator << (std::ostream &os,std::pair<std::string,std::string> const &p)
    {
        return os << p.first << '=' << p.second;
    }
    
    std::ostream & operator << (std::ostream &os,const tag &t)
    {
        return os << t.id;
    }
    
}
        
int main()
{
    const char *test = 
    //  "<A x=\"1\" y=\"2\"> test 1 </A>"
    //  "<B x=\"3\" y= \"4\" z = \"10\"> test 3 </B>"
    //  "<C><A></A><V><W></W></V></C>"
    //  "<D x=\"4\"/>"
        "<E>xxx<F>yyy</F>zzz</E>"
        ;
    std::list<tag>  tags;
    xml_g   g(tags);
    
    if(SP::parse(test,g,SP::comment_p("<---","--->")).full)
    {
        std::for_each(tags.begin(),tags.end(),walk_data());
    }
    else
    {
        std::cout << "parse failed\n";
    }
}

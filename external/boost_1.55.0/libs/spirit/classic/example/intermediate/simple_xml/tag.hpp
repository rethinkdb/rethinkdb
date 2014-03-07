//  Copyright (c) 2005 Carl Barron. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef SIMPLE_XML_TAG_H
#define SIMPLE_XML_TAG_H

#include <boost/variant.hpp>
#include <list>
#include <map>
#include <string>

struct tag
{
    std::string id;
    std::map<std::string,std::string> attributes;
    typedef boost::variant<
        std::string,
        boost::recursive_wrapper<tag>
        >
    variant_type;
    std::list<variant_type> children;
};


struct walk_data
{
    typedef void result_type;
    void operator () (const std::string &x);
    void operator () (const tag &t);
};

#endif

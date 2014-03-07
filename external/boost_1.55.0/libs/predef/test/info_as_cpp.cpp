/*
Copyright Redshift Software Inc. 2011-2012
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#include <string>
#include <iostream>
#include <set>

#define BOOST_PREDEF_INTERNAL_GENERATE_TESTS

namespace
{
    struct predef_info
    {
        std::string name;
        std::string description;
        unsigned value;

        predef_info(
            std::string const & n,
            std::string const & d,
            unsigned v);

        predef_info(
            predef_info const & other)
            : name(other.name)
            , description(other.description)
            , value(other.value)
        {
        }

        bool operator < (predef_info const & other) const
        {
            return name < other.name;
        }
    };

    std::set<predef_info> * predefs = 0;

    predef_info::predef_info(
        std::string const & n,
        std::string const & d,
        unsigned v)
        : name(n)
        , description(d)
        , value(v)
    {
        if (!predefs)
        {
            predefs = new std::set<predef_info>();
        }
        predefs->insert(*this);
    }
}

#define BOOST_PREDEF_DECLARE_TEST(x,s) \
    namespace { \
        predef_info x##_predef_init(#x,s,x); \
    }
#include <boost/predef.h>

int main()
{
    std::set<predef_info>::iterator i;
    std::set<predef_info>::iterator e = predefs->end();
    std::cout << "** Detected **" << std::endl;
    for (i = predefs->begin(); i != e; ++i)
    {
        if (i->value > 0)
            std::cout
                << i->name << " = "
                << i->value
                << " (" << (i->value/10000000)%100 << "," << (i->value/100000)%100 << "," << (i->value)%100000 << ") | "
                << i->description
                << std::endl;
    }
    std::cout << "** Not Detected **" << std::endl;
    for (i = predefs->begin(); i != e; ++i)
    {
        if (i->value == 0)
            std::cout
                << i->name << " = "
                << i->value << " | "
                << i->description
                << std::endl;
    }
    return 0;
}

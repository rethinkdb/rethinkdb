//  Copyright (c) 2005 Carl Barron. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef XML_G_H
#define XML_G_H
#define BOOST_SPIRIT_DEBUG
#ifndef BOOST_SPIRIT_CLOSURE_LIMIT
#define BOOST_SPIRIT_CLOSURE_LIMIT  10
#endif

#ifndef PHOENIX_LIMIT
#define PHOENIX_LIMIT   10
#endif

#if BOOST_SPIRIT_CLOSURE_LIMIT < 6
#undef BOOST_SPIRIT_CLOSURE_LIMIT
#define BOOST_SPIRIT_CLOSURE_LIMIT  6
#endif

#if PHOENIX_LIMIT < BOOST_SPIRIT_CLOSURE_LIMIT
#undef PHOENIX_LIMIT
#define PHOENIX_LIMIT BOOST_SPIRIT_CLOSURE_LIMIT
#endif

#if 0
#ifdef BOOST_SPIRIT_DEBUG_FLAGS
#undef BOOST_SPIRIT_DEBUG_FLAGS
#endif
#define BOOST_SPIRIT_DEBUG_FLAGS (BOOST_SPIRIT_DEBUG_FLAGS_MAX - BOOST_SPIRIT_DEBUG_FLAGS_CLOSURES)
#endif

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_attribute.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include "tag.hpp"
#include "actions.hpp"
#include <boost/variant.hpp>

#include <string>
#include <utility>

namespace SP = BOOST_SPIRIT_CLASSIC_NS;
using phoenix::arg1;
using phoenix::arg2;
using phoenix::construct_;
using phoenix::var;

struct str_cls:SP::closure<str_cls,std::string>
{ member1 value;};

struct attrib_cls:SP::closure
<
    attrib_cls,
    std::pair<std::string,std::string>,
    std::string,
    std::string
>
{
    member1 value;
    member2 first;
    member3 second;
};

struct tagged_cls:SP::closure
<
    tagged_cls,
    tag,
    std::string,
    std::map<std::string,std::string>,
    std::list<typename tag::variant_type>
>
{
    member1 value;
    member2 ID;
    member3 attribs;
    member4 children;
};

struct xml_g:SP::grammar<xml_g>
{
    std::list<tag>  &tags;
    xml_g(std::list<tag> &a):tags(a){}
    template <class Scan>
    struct definition
    {
        definition(const xml_g &s)
        {
            white = +SP::space_p
                ;

            tagged = (start_tag
                >> *inner
                >> end_tag
                | simple_start_tag
                )
                [store_tag(tagged.value,tagged.ID,tagged.attribs,
                    tagged.children)]
                ;

            end_tag = SP::str_p("</")
                >> SP::f_str_p(tagged.ID)
                >> '>'
                ;

            inner = (tagged
                | str)  [push_child(tagged.children,arg1)]
                ;

            str = SP::lexeme_d[+(SP::anychar_p - '<')]
                [str.value=construct_<std::string>(arg1,arg2)]              
                ;

            top = +tagged
                [push_back(var(s.tags),arg1)]
                ;

            starter = SP::ch_p('<')
                >> SP::lexeme_d[+SP::alpha_p]
        [tagged.ID = construct_<std::string>(arg1,arg2)]
                >> *attrib
                [store_in_map(tagged.attribs,arg1)]
                >> !white
                ;
            start_tag = starter
                >> '>'
                ;

            simple_start_tag = starter
                >> "/>"
                ;
                
            attrib = white
                >>SP::lexeme_d[+SP::alpha_p]
                [attrib.first = construct_<std::string>(arg1,arg2)]
                >> !white
                >> '='
                >> !white
                >> '"'
                >> SP::lexeme_d[+(SP::anychar_p - '"')]
                    [attrib.second = construct_<std::string>(arg1,arg2)]
                >> SP::ch_p('"')
                    [attrib.value = construct_
                    <
                        std::pair
                        <
                            std::string,
                            std::string
                        >
                    >(attrib.first,attrib.second)]
                ;
        BOOST_SPIRIT_DEBUG_RULE(tagged);
        BOOST_SPIRIT_DEBUG_RULE(end_tag);
        BOOST_SPIRIT_DEBUG_RULE(inner);
        BOOST_SPIRIT_DEBUG_RULE(str);
        BOOST_SPIRIT_DEBUG_RULE(top);
        BOOST_SPIRIT_DEBUG_RULE(start_tag);
        BOOST_SPIRIT_DEBUG_RULE(attrib);
        BOOST_SPIRIT_DEBUG_RULE(white);
        BOOST_SPIRIT_DEBUG_RULE(starter);
        BOOST_SPIRIT_DEBUG_RULE(simple_start_tag);
        }
        
        // actions
        push_back_f     push_back;
        push_child_f    push_child;
        store_in_map_f  store_in_map;
        store_tag_f     store_tag;
        // rules
        SP::rule<Scan,tagged_cls::context_t>    tagged;
        SP::rule<Scan>                          end_tag;
        SP::rule<Scan>                          inner;
        SP::rule<Scan,str_cls::context_t>       str;
        SP::rule<Scan>                          top;
        SP::rule<Scan>                          starter;
        SP::rule<Scan>                          simple_start_tag;
        SP::rule<Scan>                          start_tag;
        SP::rule<Scan>                          white;
        SP::rule<Scan,attrib_cls::context_t>    attrib;
        SP::rule<Scan> const &start() const
        { return top;}
    };
};

#endif


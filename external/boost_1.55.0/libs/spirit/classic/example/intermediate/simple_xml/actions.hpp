//  Copyright (c) 2005 Carl Barron. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef ACTIONS_H
#define ACTIONS_H
#include <boost/spirit/include/phoenix1.hpp>
#include <boost/variant.hpp>
#include "tag.hpp"

struct push_child_impl
{
    template <class T,class A>
    struct result {typedef void type;};
    
    template <class T,class A>
    void operator () (T &list, const A &value) const
    {
        typename tag::variant_type p(value);
        list.push_back(p);
    }
};

struct store_in_map_impl
{
    template <class T,class A>
    struct result{typedef void type;};
    
    template <class T,class A>
    void operator () (T &map,const A &value)const
    {
        typedef typename T::value_type value_type;
        map.insert(value_type(value));
    }
};

struct push_back_impl
{
    template <class T,class A>
    struct result {typedef void type;};
    
    template <class T,class A>
    void operator () (T &list,const A &value)const
    {
        list.push_back(value);
    }
};

struct store_tag_impl
{
    template <class T,class A,class B,class C>
    struct result {typedef void type;};
    
    template <class T,class A,class B,class C>
    void operator ()(T &t,const A &a,const B &b,const C &c)const
    {
        t.id = a;
        t.attributes = b;
        t.children = c;
    }
};


typedef phoenix::function<push_back_impl>   push_back_f;
typedef phoenix::function<store_in_map_impl>store_in_map_f;
typedef phoenix::function<push_child_impl>  push_child_f;
typedef phoenix::function<store_tag_impl>   store_tag_f;
#endif

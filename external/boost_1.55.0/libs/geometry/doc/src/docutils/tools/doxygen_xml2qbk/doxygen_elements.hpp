// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright (c) 2010-2013 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//
#ifndef DOXYGEN_ELEMENTS_HPP
#define DOXYGEN_ELEMENTS_HPP


#include <string>
#include <vector>


enum markup_type { markup_default, markup_synopsis };
enum markup_order_type { markup_any, markup_before, markup_after };

// TODO: rename, not all are functions
enum function_type 
{ 
    function_unknown, 
    function_define, 
    function_constructor_destructor, 
    function_member, 
    function_free,
};

struct base_element
{
    std::string name;
    std::string brief_description;
    
    bool skip;

    std::string id;

    base_element(std::string const& n = "")
        : name(n)
        , skip(false)
    {}
};


// contains (template)parameter
struct parameter : public base_element
{
    std::string type;
    std::string default_value; // for template parameters
    std::string fulltype; // post-processed
    std::string fulltype_without_links;
};

struct enumeration_value : public base_element
{
    std::string initializer;
};



struct markup
{
    std::string value;
    markup_order_type order;
    markup_type type;

    markup(std::string const& v = "")
        : value(v)
        , order(markup_any)
        , type(markup_default)
    {
        init();
    }

    markup(markup_order_type o, markup_type t, std::string const& v = "")
        : value(v)
        , order(o)
        , type(t)
    {
        init();
    }

    void init()
    {
        boost::trim(value);
        boost::replace_all(value, "\\*", "*");
    }
};

struct paragraph
{
    std::string title;
    std::string text;
};

// Base of a class/struct, function, define
struct element : public base_element
{
    std::string detailed_description;
    std::string location;
    int line; // To sort - Doxygen changes order - we change it back

    // QBK-includes
    // Filled with e.g.: \qbk([include reference/myqbk.qbk]}
    std::vector<markup> qbk_markup;

    // To distinguish overloads: unary, binary etc,
    // Filled with: \qbk{distinguish,<A discerning description>}
    std::string additional_description;

    std::vector<parameter> template_parameters;
    std::vector<parameter> parameters;

    std::vector<paragraph> paragraphs;
    std::string warning;
    std::string note;

    element()
        : line(0)
    {}
};


struct function : public element
{
    function_type type;
    std::string definition, argsstring;
    std::string return_type, return_description;
    std::string precondition;

    std::string return_type_without_links;
    bool is_static, is_const, is_explicit, is_virtual;

    bool unique;

    function()
        : type(function_unknown)
        , unique(true)
        , is_static(false), is_const(false), is_explicit(false), is_virtual(false)
    {}

};


struct enumeration : public element
{
    std::vector<enumeration_value> enumeration_values;
};


struct base_class
{
    std::string name;
    std::string derivation; // "prot" element 
    std::string virtuality; // "virt" element
};

struct class_or_struct : public element
{
    bool is_class; // true if class, false if struct
    std::string name, fullname;
    std::vector<function> functions;

    std::vector<base_element> typedefs;
    std::vector<parameter> variables;

    std::vector<base_class> base_classes;
};


struct documentation
{
    std::string group_id;
    std::string group_title;

    // Only one expected (no grouping)
    class_or_struct cos; 

    // There can be many of them (in groups):
    std::vector<function> functions; 
    std::vector<function> defines;
    std::vector<enumeration> enumerations;
};


#endif // DOXYGEN_ELEMENTS_HPP

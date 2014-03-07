// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

// This example shows what need to be done to customize data_type of ptree.
//
// It creates my_ptree type, which is a basic_ptree having boost::any as its data
// container (instead of std::string that standard ptree has).

#include <boost/property_tree/ptree.hpp>
#include <boost/any.hpp>
#include <list>
#include <string>
#include <iostream>

// Custom translator that works with boost::any instead of std::string
template <class Ext, class Int = boost::any>
struct variant_translator
{
    typedef Ext external_type;
    typedef Int internal_type;

    external_type
    get_value(const internal_type &value) const
    {
        return boost::any_cast<external_type>(value);
    }
    internal_type
    put_value(const external_type &value) const
    {
        return value;
    }
};

int main()
{
    
    using namespace boost::property_tree;
    
    // Property_tree with boost::any as data type
    // Key type:        std::string
    // Data type:       boost::any
    // Key comparison:  default (std::less<std::string>)
    typedef basic_ptree<std::string, boost::any> my_ptree;
    my_ptree pt;

    // Put/get int value
    typedef variant_translator<int> int_tran;
    pt.put("int value", 3, int_tran());
    int int_value = pt.get<int>("int value", int_tran());
    std::cout << "Int value: " << int_value << "\n";

    // Put/get string value
    typedef variant_translator<std::string> string_tran;
    pt.put<std::string>("string value", "foo bar", string_tran());
    std::string string_value = pt.get<std::string>(
        "string value"
      , string_tran()
    );
    std::cout << "String value: " << string_value << "\n";

    // Put/get list<int> value
    typedef std::list<int> intlist;
    typedef variant_translator<intlist> intlist_tran;
    int list_data[] = { 1, 2, 3, 4, 5 };
    pt.put<intlist>(
        "list value"
      , intlist(
            list_data
          , list_data + sizeof(list_data) / sizeof(*list_data)
        )
      , intlist_tran()
    );
    intlist list_value = pt.get<intlist>(
        "list value"
      , intlist_tran()
    );
    std::cout << "List value: ";
    for (intlist::iterator it = list_value.begin(); it != list_value.end(); ++it)
        std::cout << *it << ' ';
    std::cout << '\n';
}

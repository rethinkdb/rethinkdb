/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/config/warning_disable.hpp>

//[customize_karma_embedded_container_includes
#include <boost/spirit/include/karma.hpp>
#include <iostream>
#include <vector>
//]

///////////////////////////////////////////////////////////////////////////////
//[customize_karma_embedded_container_data
namespace client
{
    struct embedded_container
    {
        // expose the iterator of the embedded vector as our iterator
        typedef std::vector<int>::const_iterator iterator;

        // expose the type of the held data elements as our type
        typedef std::vector<int>::value_type type;

        // this is the vector holding the actual elements we need to generate 
        // output from
        std::vector<int> data;
    };
}
//]

//[customize_karma_embedded_container_traits
// All specializations of attribute customization points have to be placed into
// the namespace boost::spirit::traits.
//
// Note that all templates below are specialized using the 'const' type.
// This is necessary as all attributes in Karma are 'const'.
namespace boost { namespace spirit { namespace traits
{
    // The specialization of the template 'is_container<>' will tell the 
    // library to treat the type 'client::embedded_container' as a 
    // container holding the items to generate output from.
    template <>
    struct is_container<client::embedded_container const>
      : mpl::true_
    {};

    // The specialization of the template 'container_iterator<>' will be
    // invoked by the library to evaluate the iterator type to be used
    // for iterating the data elements in the container. We simply return
    // the type of the iterator exposed by the embedded 'std::vector<int>'.
    template <>
    struct container_iterator<client::embedded_container const>
    {
        typedef client::embedded_container::iterator type;
    };

    // The specialization of the templates 'begin_container<>' and 
    // 'end_container<>' below will be used by the library to get the iterators 
    // pointing to the begin and the end of the data to generate output from. 
    // These specializations simply return the 'begin' and 'end' iterators as 
    // exposed by the embedded 'std::vector<int>'.
    //
    // The passed argument refers to the attribute instance passed to the list 
    // generator.
    template <>
    struct begin_container<client::embedded_container const>
    {
        static client::embedded_container::iterator 
        call(client::embedded_container const& d)
        {
            return d.data.begin();
        }
    };

    template <>
    struct end_container<client::embedded_container const>
    {
        static client::embedded_container::iterator 
        call(client::embedded_container const& d)
        {
            return d.data.end();
        }
    };
}}}
//]

///////////////////////////////////////////////////////////////////////////////
namespace karma = boost::spirit::karma;

int main()
{
    //[customize_karma_embedded_container
    client::embedded_container d1;    // create some test data
    d1.data.push_back(1);
    d1.data.push_back(2);
    d1.data.push_back(3);

    // use the instance of an 'client::embedded_container' instead of a 
    // STL vector
    std::cout << karma::format(karma::int_ % ", ", d1) << std::endl;   // prints: '1, 2, 3'
    //]
    return 0;
}


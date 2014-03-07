/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/config/warning_disable.hpp>

//[customize_karma_counter_includes
#include <boost/spirit/include/karma.hpp>
#include <iostream>
#include <vector>
//]

///////////////////////////////////////////////////////////////////////////////
//[customize_karma_counter_data
namespace client
{
    struct counter
    {
        // expose the current value of the counter as our iterator
        typedef int iterator;

        // expose 'int' as the type of each generated element
        typedef int type;

        counter(int max_count) 
          : counter_(0), max_count_(max_count)
        {}

        int counter_;
        int max_count_;
    };
}
//]

//[customize_karma_counter_traits
// All specializations of attribute customization points have to be placed into
// the namespace boost::spirit::traits.
//
// Note that all templates below are specialized using the 'const' type.
// This is necessary as all attributes in Karma are 'const'.
namespace boost { namespace spirit { namespace traits
{
    // The specialization of the template 'is_container<>' will tell the 
    // library to treat the type 'client::counter' as a container providing 
    // the items to generate output from.
    template <>
    struct is_container<client::counter const>
      : mpl::true_
    {};

    // The specialization of the template 'container_iterator<>' will be
    // invoked by the library to evaluate the iterator type to be used
    // for iterating the data elements in the container. 
    template <>
    struct container_iterator<client::counter const>
    {
        typedef client::counter::iterator type;
    };

    // The specialization of the templates 'begin_container<>' and 
    // 'end_container<>' below will be used by the library to get the iterators 
    // pointing to the begin and the end of the data to generate output from. 
    // These specializations respectively return the initial and maximum 
    // counter values.
    //
    // The passed argument refers to the attribute instance passed to the list 
    // generator.
    template <>
    struct begin_container<client::counter const>
    {
        static client::counter::iterator 
        call(client::counter const& c)
        {
            return c.counter_;
        }
    };

    template <>
    struct end_container<client::counter const>
    {
        static client::counter::iterator 
        call(client::counter const& c)
        {
            return c.max_count_;
        }
    };
}}}
//]

//[customize_karma_counter_iterator_traits
// All specializations of attribute customization points have to be placed into
// the namespace boost::spirit::traits.
namespace boost { namespace spirit { namespace traits
{
    // The specialization of the template 'deref_iterator<>' will be used to 
    // dereference the iterator associated with our counter data structure.
    // Since we expose the current value as the iterator we just return the 
    // current iterator as the return value.
    template <>
    struct deref_iterator<client::counter::iterator>
    {
        typedef client::counter::type type;

        static type call(client::counter::iterator const& it)
        {
            return it;
        }
    };
}}}
//]

///////////////////////////////////////////////////////////////////////////////
namespace karma = boost::spirit::karma;

int main()
{
    //[customize_karma_counter
    // use the instance of a 'client::counter' instead of a STL vector
    client::counter count(4);
    std::cout << karma::format(karma::int_ % ", ", count) << std::endl;   // prints: '0, 1, 2, 3'
    //]
    return 0;
}


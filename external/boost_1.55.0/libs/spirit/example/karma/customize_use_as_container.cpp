/*=============================================================================
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/config/warning_disable.hpp>

//[customize_karma_use_as_container_includes
#include <boost/spirit/include/karma.hpp>
#include <iostream>
#include <string>
#include <vector>
//]

///////////////////////////////////////////////////////////////////////////////
//[customize_karma_use_as_container_data
namespace client
{
    struct use_as_container
    {
        // Expose a pair holding a pointer to the use_as_container and to the 
        // current element as our iterator.
        // We intentionally leave out having it a 'operator==()' to demonstrate
        // the use of the 'compare_iterators' customization point.
        struct iterator
        {
            iterator(use_as_container const* container, int const* current)
              : container_(container), current_(current)
            {}

            use_as_container const* container_;
            int const* current_;
        };

        // expose 'int' as the type of each generated element
        typedef int type;

        use_as_container(int value1, int value2, int value3) 
          : value1_(value1), value2_(value2), value3_(value3)
        {}

        int value1_;
        std::string dummy1_;    // insert some unrelated data
        int value2_;
        std::string dummy2_;    // insert some more unrelated data
        int value3_;
    };
}
//]

//[customize_karma_use_as_container_traits
// All specializations of attribute customization points have to be placed into
// the namespace boost::spirit::traits.
//
// Note that all templates below are specialized using the 'const' type.
// This is necessary as all attributes in Karma are 'const'.
namespace boost { namespace spirit { namespace traits
{
    // The specialization of the template 'is_container<>' will tell the 
    // library to treat the type 'client::use_as_container' as a 
    // container holding the items to generate output from.
    template <>
    struct is_container<client::use_as_container const>
      : mpl::true_
    {};

    // The specialization of the template 'container_iterator<>' will be
    // invoked by the library to evaluate the iterator type to be used
    // for iterating the data elements in the container. We simply return
    // the type of the iterator exposed by the embedded 'std::vector<int>'.
    template <>
    struct container_iterator<client::use_as_container const>
    {
        typedef client::use_as_container::iterator type;
    };

    // The specialization of the templates 'begin_container<>' and 
    // 'end_container<>' below will be used by the library to get the iterators 
    // pointing to the begin and the end of the data to generate output from. 
    //
    // The passed argument refers to the attribute instance passed to the list 
    // generator.
    template <>
    struct begin_container<client::use_as_container const>
    {
        static client::use_as_container::iterator 
        call(client::use_as_container const& c)
        {
            return client::use_as_container::iterator(&c, &c.value1_);
        }
    };

    template <>
    struct end_container<client::use_as_container const>
    {
        static client::use_as_container::iterator 
        call(client::use_as_container const& c)
        {
            return client::use_as_container::iterator(&c, (int const*)0);
        }
    };
}}}
//]

//[customize_karma_use_as_container_iterator_traits
// All specializations of attribute customization points have to be placed into
// the namespace boost::spirit::traits.
namespace boost { namespace spirit { namespace traits
{
    // The specialization of the template 'deref_iterator<>' will be used to 
    // dereference the iterator associated with our counter data structure.
    template <>
    struct deref_iterator<client::use_as_container::iterator>
    {
        typedef client::use_as_container::type type;

        static type call(client::use_as_container::iterator const& it)
        {
            return *it.current_;
        }
    };

    template <>
    struct next_iterator<client::use_as_container::iterator>
    {
        static void call(client::use_as_container::iterator& it)
        {
            if (it.current_ == &it.container_->value1_)
                it.current_ = &it.container_->value2_;
            else if (it.current_ == &it.container_->value2_)
                it.current_ = &it.container_->value3_;
            else
                it.current_ = 0;
        }
    };

    template <>
    struct compare_iterators<client::use_as_container::iterator>
    {
        static bool call(client::use_as_container::iterator const& it1
          , client::use_as_container::iterator const& it2)
        {
            return it1.current_ == it2.current_ && 
                   it1.container_ == it2.container_;
        }
    };
}}}
//]

///////////////////////////////////////////////////////////////////////////////
namespace karma = boost::spirit::karma;

int main()
{
    //[customize_karma_use_as_container
    client::use_as_container d2 (1, 2, 3);
    // use the instance of a 'client::use_as_container' instead of a STL vector
    std::cout << karma::format(karma::int_ % ", ", d2) << std::endl;   // prints: '1, 2, 3'
    //]
    return 0;
}


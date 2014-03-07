
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
// See macro_type_arugment.hpp in this directory for details.

#include <string>
#include <typeinfo>
#include <iostream>

#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>

#include "macro_type_args.hpp"


#define TYPE_NAME(parenthesized_type) \
    typeid(BOOST_EXAMPLE_MACRO_TYPE_ARGUMENT(parenthesized_type)).name()

namespace example
{
  namespace mpl = boost::mpl;

  template<class Curr, class End>
  struct mpl_seq_to_string_impl
  {
    static std::string get(std::string const & prev)
    {
      typedef typename mpl::next<Curr>::type next_pos;
      typedef typename mpl::deref<Curr>::type type;

      return mpl_seq_to_string_impl<next_pos,End>::get(
          prev + (prev.empty()? '\0' : ',') + typeid(type).name() );
    }
  };
  template<class End>
  struct mpl_seq_to_string_impl<End, End>
  {
    static std::string get(std::string const & prev)
    {
      return prev;
    }
  };

  template<class Seq>
  std::string mpl_seq_to_string()
  {
    typedef typename mpl::begin<Seq>::type begin;
    typedef typename mpl::end<Seq>::type end;

    return mpl_seq_to_string_impl<begin, end>::get("");
  }

}

#define TYPE_NAMES(parenthesized_types) \
    ::example::mpl_seq_to_string< \
        BOOST_EXAMPLE_MACRO_TYPE_LIST_ARGUMENT(parenthesized_types) >()

int main()
{
  std::cout << TYPE_NAME((int)) << std::endl;

  std::cout << TYPE_NAMES((int,char)) << std::endl;
  std::cout << TYPE_NAMES((int,char,long)) << std::endl;

}


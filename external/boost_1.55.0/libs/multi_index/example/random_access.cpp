/* Boost.MultiIndex example of use of random access indices.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/tokenizer.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;

/* text_container holds words as inserted and also keep them indexed
 * by dictionary order.
 */

typedef multi_index_container<
  std::string,
  indexed_by<
    random_access<>,
    ordered_non_unique<identity<std::string> >
  >
> text_container;

/* ordered index */

typedef nth_index<text_container,1>::type ordered_text;

/* Helper function for obtaining the position of an element in the
 * container.
 */

template<typename IndexIterator>
text_container::size_type text_position(
  const text_container& tc,IndexIterator it)
{
  /* project to the base index and calculate offset from begin() */

  return project<0>(tc,it)-tc.begin();
}

typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;

int main()
{
  std::string text=
    "'Oh, you wicked little thing!' cried Alice, catching up the kitten, "
    "and giving it a little kiss to make it understand that it was in "
    "disgrace. 'Really, Dinah ought to have taught you better manners! You "
    "ought, Dinah, you know you ought!' she added, looking reproachfully at "
    "the old cat, and speaking in as cross a voice as she could manage "
    "-- and then she scrambled back into the armchair, taking the kitten and "
    "the worsted with her, and began winding up the ball again. But she "
    "didn't get on very fast, as she was talking all the time, sometimes to "
    "the kitten, and sometimes to herself. Kitty sat very demurely on her "
    "knee, pretending to watch the progress of the winding, and now and then "
    "putting out one paw and gently touching the ball, as if it would be glad "
    "to help, if it might.";

  /* feed the text into the container */

  text_container tc;
  tc.reserve(text.size()); /* makes insertion faster */
  text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));
  std::copy(tok.begin(),tok.end(),std::back_inserter(tc));

  std::cout<<"enter a position (0-"<<tc.size()-1<<"):";
  text_container::size_type pos=tc.size();
  std::cin>>pos;
  if(pos>=tc.size()){
    std::cout<<"out of bounds"<<std::endl;
  }
  else{
    std::cout<<"the word \""<<tc[pos]<<"\" appears at position(s): ";

    std::pair<ordered_text::iterator,ordered_text::iterator> p=
      get<1>(tc).equal_range(tc[pos]);
    while(p.first!=p.second){
      std::cout<<text_position(tc,p.first++)<<" ";
    }

    std::cout<<std::endl;
  }

  return 0;
}

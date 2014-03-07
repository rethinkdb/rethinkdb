/* Boost.MultiIndex example of a bidirectional map.
 *
 * Copyright 2003-2009 Joaquin M Lopez Munoz.
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
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <iostream>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;

/* tags for accessing both sides of a bidirectional map */

struct from{};
struct to{};

/* The class template bidirectional_map wraps the specification
 * of a bidirectional map based on multi_index_container.
 */

template<typename FromType,typename ToType>
struct bidirectional_map
{
  struct value_type
  {
    value_type(const FromType& first_,const ToType& second_):
      first(first_),second(second_)
    {}

    FromType first;
    ToType   second;
  };

#if defined(BOOST_NO_POINTER_TO_MEMBER_TEMPLATE_PARAMETERS) ||\
    defined(BOOST_MSVC)&&(BOOST_MSVC<1300) ||\
    defined(BOOST_INTEL_CXX_VERSION)&&defined(_MSC_VER)&&\
           (BOOST_INTEL_CXX_VERSION<=700)

/* see Compiler specifics: Use of member_offset for info on member<> and
 * member_offset<>
 */

  BOOST_STATIC_CONSTANT(unsigned,from_offset=offsetof(value_type,first));
  BOOST_STATIC_CONSTANT(unsigned,to_offset  =offsetof(value_type,second));

  typedef multi_index_container<
    value_type,
    indexed_by<
      ordered_unique<
        tag<from>,member_offset<value_type,FromType,from_offset> >,
      ordered_unique<
        tag<to>,  member_offset<value_type,ToType,to_offset> >
    >
  > type;

#else

  /* A bidirectional map can be simulated as a multi_index_container
   * of pairs of (FromType,ToType) with two unique indices, one
   * for each member of the pair.
   */

  typedef multi_index_container<
    value_type,
    indexed_by<
      ordered_unique<
        tag<from>,member<value_type,FromType,&value_type::first> >,
      ordered_unique<
        tag<to>,  member<value_type,ToType,&value_type::second> >
    >
  > type;

#endif
};

/* a dictionary is a bidirectional map from strings to strings */

typedef bidirectional_map<std::string,std::string>::type dictionary;

int main()
{
  dictionary d;

  /* Fill up our microdictionary. first members Spanish, second members
   * English.
   */

  d.insert(dictionary::value_type("hola","hello"));
  d.insert(dictionary::value_type("adios","goodbye"));
  d.insert(dictionary::value_type("rosa","rose"));
  d.insert(dictionary::value_type("mesa","table"));


  std::cout<<"enter a word"<<std::endl;
  std::string word;
  std::getline(std::cin,word);

#if defined(BOOST_NO_MEMBER_TEMPLATES) /* use global get<> and family instead */

  dictionary::iterator it=get<from>(d).find(word);
  if(it!=d.end()){
    std::cout<<word<<" is said "<<it->second<<" in English"<<std::endl;
  }
  else{
    nth_index<dictionary,1>::type::iterator it2=get<1>(d).find(word);
    if(it2!=get<1>(d).end()){
      std::cout<<word<<" is said "<<it2->first<<" in Spanish"<<std::endl;
    }
    else std::cout<<"No such word in the dictionary"<<std::endl;
  }

#else

  /* search the queried word on the from index (Spanish) */

  dictionary::iterator it=d.get<from>().find(word);
  if(it!=d.end()){ /* found */

    /* the second part of the element is the equivalent in English */

    std::cout<<word<<" is said "<<it->second<<" in English"<<std::endl;
  }
  else{
    /* word not found in Spanish, try our luck in English */

    dictionary::index<to>::type::iterator it2=d.get<to>().find(word);
    if(it2!=d.get<to>().end()){
      std::cout<<word<<" is said "<<it2->first<<" in Spanish"<<std::endl;
    }
    else std::cout<<"No such word in the dictionary"<<std::endl;
  }

#endif

  return 0;
}

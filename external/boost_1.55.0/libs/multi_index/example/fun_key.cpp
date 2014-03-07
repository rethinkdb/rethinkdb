/* Boost.MultiIndex example of functions used as key extractors.
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
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <iostream>
#include <string>

using namespace boost::multi_index;

/* A name record consists of the given name (e.g. "Charlie")
 * and the family name (e.g. "Brown"). The full name, calculated
 * by name_record::name() is laid out in the "phonebook order"
 * family name + given_name.
 */

struct name_record
{
  name_record(std::string given_name_,std::string family_name_):
    given_name(given_name_),family_name(family_name_)
  {}

  std::string name()const
  {
    std::string str=family_name;
    str+=" ";
    str+=given_name;
    return str;
  }

private:
  std::string given_name;
  std::string family_name;
};

std::string::size_type name_record_length(const name_record& r)
{
  return r.name().size();
}

/* multi_index_container with indices based on name_record::name()
 * and name_record_length().
 * See Compiler specifics: Use of const_mem_fun_explicit and
 * mem_fun_explicit for info on BOOST_MULTI_INDEX_CONST_MEM_FUN.
 */

typedef multi_index_container<
  name_record,
  indexed_by<
    ordered_unique<
      BOOST_MULTI_INDEX_CONST_MEM_FUN(name_record,std::string,name)
    >,
    ordered_non_unique<
      global_fun<const name_record&,std::string::size_type,name_record_length>
    >
  >
> name_record_set;

int main()
{
  name_record_set ns;

  ns.insert(name_record("Joe","Smith"));
  ns.insert(name_record("Robert","Nightingale"));
  ns.insert(name_record("Robert","Brown"));
  ns.insert(name_record("Marc","Tuxedo"));

  /* list the names in ns in phonebook order */

  std::cout<<"Phonenook order\n"
           <<"---------------"<<std::endl;
  for(name_record_set::iterator it=ns.begin();it!=ns.end();++it){
    std::cout<<it->name()<<std::endl;
  }

  /* list the names in ns according to their length*/

  std::cout<<"\nLength order\n"
           <<  "------------"<<std::endl;
  for(nth_index<name_record_set,1>::type::iterator it1=get<1>(ns).begin();
      it1!=get<1>(ns).end();++it1){
    std::cout<<it1->name()<<std::endl;
  }

  return 0;
}

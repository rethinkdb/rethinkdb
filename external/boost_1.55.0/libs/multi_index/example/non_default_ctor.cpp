/* Boost.MultiIndex example of use of multi_index_container::ctor_args_list.
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
#include <algorithm>
#include <iostream>
#include <iterator>

using boost::multi_index_container;
using namespace boost::multi_index;

/* modulo_less order numbers according to their division residual.
 * For instance, if modulo==10 then 22 is less than 15 as 22%10==2 and
 * 15%10==5.
 */

template<typename IntegralType>
struct modulo_less
{
  modulo_less(IntegralType m):modulo(m){}

  bool operator()(IntegralType x,IntegralType y)const
  {
    return (x%modulo)<(y%modulo);
  }

private:
  IntegralType modulo;
};

/* multi_index_container of unsigned ints holding a "natural" index plus
 * an ordering based on modulo_less.
 */

typedef multi_index_container<
  unsigned int,
  indexed_by<
    ordered_unique<identity<unsigned int> >,
    ordered_non_unique<identity<unsigned int>, modulo_less<unsigned int> >
  >
> modulo_indexed_set;

int main()
{
  /* define a modulo_indexed_set with modulo==10 */

  modulo_indexed_set::ctor_args_list args_list=
    boost::make_tuple(
      /* ctor_args for index #0 is default constructible */
      nth_index<modulo_indexed_set,0>::type::ctor_args(),

      /* first parm is key_from_value, second is our sought for key_compare */
      boost::make_tuple(identity<unsigned int>(),modulo_less<unsigned int>(10))
    );

  modulo_indexed_set m(args_list);
  /* this could have be written online without the args_list variable,
   * left as it is for explanatory purposes. */

  /* insert some numbers */

  unsigned int       numbers[]={0,1,20,40,33,68,11,101,60,34,88,230,21,4,7,17};
  const std::size_t  numbers_length(sizeof(numbers)/sizeof(numbers[0]));

  m.insert(&numbers[0],&numbers[numbers_length]);

  /* lists all numbers in order, along with their "equivalence class", that is,
   * the equivalent numbers under modulo_less
   */

  for(modulo_indexed_set::iterator it=m.begin();it!=m.end();++it){
    std::cout<<*it<<" -> ( ";

    nth_index<modulo_indexed_set,1>::type::iterator it0,it1;
    boost::tie(it0,it1)=get<1>(m).equal_range(*it);
    std::copy(it0,it1,std::ostream_iterator<unsigned int>(std::cout," "));

    std::cout<<")"<<std::endl;
  }

  return 0;
}

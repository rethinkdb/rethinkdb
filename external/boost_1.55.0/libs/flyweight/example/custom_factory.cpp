/* Boost.Flyweight example of custom factory.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

/* We include the default components of Boost.Flyweight except the factory,
 * which will be provided by ourselves.
 */
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/factory_tag.hpp>
#include <boost/flyweight/static_holder.hpp>
#include <boost/flyweight/simple_locking.hpp>
#include <boost/flyweight/refcounted.hpp>
#include <boost/tokenizer.hpp>
#include <functional>
#include <iostream>
#include <set>

using namespace boost::flyweights;

/* custom factory based on std::set with some logging capabilities */

/* Entry is the type of the stored objects. Value is the type
 * on which flyweight operates, that is, the T in flyweoght<T>. It
 * is guaranteed that Entry implicitly converts to const Value&.
 * The factory class could accept other template arguments (for
 * instance, a comparison predicate for the values), we leave it like
 * that for simplicity.
 */

template<typename Entry,typename Key>
class verbose_factory_class
{ 
  /* Entry store. Since Entry is implicitly convertible to const Key&,
   * we can directly use std::less<Key> as the comparer for std::set.
   */

  typedef std::set<Entry,std::less<Key> > store_type;

  store_type store;

public:
  typedef typename store_type::iterator handle_type;

  handle_type insert(const Entry& x)
  {
    /* locate equivalent entry or insert otherwise */

    std::pair<handle_type, bool> p=store.insert(x);
    if(p.second){ /* new entry */
      std::cout<<"new: "<<(const Key&)x<<std::endl;
    }
    else{         /* existing entry */
      std::cout<<"hit: "<<(const Key&)x<<std::endl;
    }
    return p.first;
  }

  void erase(handle_type h)
  {
    std::cout<<"del: "<<(const Key&)*h<<std::endl;
    store.erase(h);
  }

  const Entry& entry(handle_type h)
  {
    return *h; /* handle_type is an iterator */
  }
};

/* Specifier for verbose_factory_class. The simplest way to tag
 * this struct as a factory specifier, so that flyweight<> accepts it
 * as such, is by deriving from boost::flyweights::factory_marker.
 * See the documentation for info on alternative tagging methods.
 */

struct verbose_factory: factory_marker
{
  template<typename Entry,typename Key>
  struct apply
  {
    typedef verbose_factory_class<Entry,Key> type;
  } ;
};

/* ready to use it */

typedef flyweight<std::string,verbose_factory> fw_string;

int main()
{
  typedef boost::tokenizer<boost::char_separator<char> > text_tokenizer;


  std::string text=
    "I celebrate myself, and sing myself, "
    "And what I assume you shall assume, "
    "For every atom belonging to me as good belongs to you. "

    "I loafe and invite my soul, "
    "I lean and loafe at my ease observing a spear of summer grass. "

    "My tongue, every atom of my blood, form'd from this soil, this air, "
    "Born here of parents born here from parents the same, and their "
    "    parents the same, "
    "I, now thirty-seven years old in perfect health begin, "
    "Hoping to cease not till death.";

  std::vector<fw_string> v;

  text_tokenizer tok(text,boost::char_separator<char>(" \t\n.,;:!?'\"-"));
  for(text_tokenizer::iterator it=tok.begin();it!=tok.end();){
    v.push_back(fw_string(*it++));
  }
  
  return 0;
}

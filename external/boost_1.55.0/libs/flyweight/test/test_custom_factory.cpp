/* Boost.Flyweight test of a custom factory.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_custom_factory.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/refcounted.hpp>
#include <boost/flyweight/simple_locking.hpp>
#include <boost/flyweight/static_holder.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <list>
#include "test_basic_template.hpp"

using namespace boost::flyweights;

/* Info on list-update containers:
 * http://gcc.gnu.org/onlinedocs/libstdc++/ext/pb_ds/lu_based_containers.html
 */

template<typename Entry,typename Key>
class lu_factory_class:public factory_marker
{
  struct entry_type
  {
    entry_type(const Entry& x_):x(x_),count(0){}

    Entry       x;
    std::size_t count;
  };

  typedef std::list<entry_type> container_type;

public:
  typedef typename container_type::iterator handle_type;
  
  handle_type insert(const Entry& x)
  {
    handle_type h;
    for(h=cont.begin();h!=cont.end();++h){
      if(static_cast<const Key&>(h->x)==static_cast<const Key&>(x)){
        if(++(h->count)==10){
          h->count=0;
          cont.splice(cont.begin(),cont,h); /* move to front */
        }
        return h;
      }
    }
    cont.push_back(entry_type(x));
    h=cont.end();
    --h;
    return h;
  }

  void erase(handle_type h)
  {
    cont.erase(h);
  }

  const Entry& entry(handle_type h){return h->x;}

private:  
  container_type cont;

public:
  typedef lu_factory_class type;
  BOOST_MPL_AUX_LAMBDA_SUPPORT(2,lu_factory_class,(Entry,Key))
};

struct lu_factory:factory_marker
{
  template<typename Entry,typename Key>
  struct apply
  {
    typedef lu_factory_class<Entry,Key> type;
  };
};

struct custom_factory_flyweight_specifier1
{
  template<typename T>
  struct apply
  {
    typedef flyweight<T,lu_factory> type;
  };
};

struct custom_factory_flyweight_specifier2
{
  template<typename T>
  struct apply
  {
    typedef flyweight<
      T,
      lu_factory_class<boost::mpl::_1,boost::mpl::_2>
    > type;
  };
};

void test_custom_factory()
{
  test_basic_template<custom_factory_flyweight_specifier1>();
  test_basic_template<custom_factory_flyweight_specifier2>();
}

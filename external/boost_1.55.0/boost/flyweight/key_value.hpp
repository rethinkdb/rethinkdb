/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_KEY_VALUE_HPP
#define BOOST_FLYWEIGHT_KEY_VALUE_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/flyweight/detail/value_tag.hpp>
#include <boost/flyweight/key_value_fwd.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp> 
#include <boost/type_traits/is_same.hpp>
#include <new>

/* key-value policy: flywewight lookup is based on Key, which also serves
 * to construct Value only when needed (new factory entry). key_value is
 * used to avoid the construction of temporary values when such construction
 * is expensive.
 * Optionally, KeyFromValue extracts the key from a value, which
 * is needed in expressions like this:
 *
 *  typedef flyweight<key_value<Key,Value> > fw_t;
 *  fw_t  fw;
 *  Value v;
 *  fw=v; // no key explicitly given
 *
 * If no KeyFromValue is provided, this latter expression fails to compile.
 */

namespace boost{

namespace flyweights{

namespace detail{

template<typename Key,typename Value,typename KeyFromValue>
struct optimized_key_value:value_marker
{
  typedef Key   key_type;
  typedef Value value_type;

  class rep_type
  {
  public:
    /* template ctors */

#define BOOST_FLYWEIGHT_PERFECT_FWD_NAME explicit rep_type
#define BOOST_FLYWEIGHT_PERFECT_FWD_BODY(n)          \
  :value_ptr(0)                                      \
{                                                    \
  new(spc_ptr())key_type(BOOST_PP_ENUM_PARAMS(n,t)); \
}
#include <boost/flyweight/detail/perfect_fwd.hpp>

    rep_type(const value_type& x):value_ptr(&x){}

    rep_type(const rep_type& x):value_ptr(x.value_ptr)
    {
      if(!x.value_ptr)new(key_ptr())key_type(*x.key_ptr());
    }

    ~rep_type()
    {
      if(!value_ptr)       key_ptr()->~key_type();
      else if(value_cted())value_ptr->~value_type();
    }

    operator const key_type&()const
    {
      if(value_ptr)return key_from_value(*value_ptr);
      else         return *key_ptr();
    }

    operator const value_type&()const
    {
      /* This is always called after construct_value() or copy_value(),
       * so we access spc directly rather than through value_ptr to
       * save us an indirection.
       */

      return *static_cast<value_type*>(spc_ptr());
    }

  private:
    friend struct optimized_key_value;

    void* spc_ptr()const{return static_cast<void*>(&spc);}
    bool  value_cted()const{return value_ptr==spc_ptr();}

    key_type* key_ptr()const
    {
      return static_cast<key_type*>(static_cast<void*>(&spc));
    }

    static const key_type& key_from_value(const value_type& x)
    {
      KeyFromValue k;
      return k(x);
    }

    void construct_value()const
    {
      if(!value_cted()){
        /* value_ptr must be ==0, oherwise copy_value would have been called */

        key_type k(*key_ptr());
        key_ptr()->~key_type();
        value_ptr= /* guarantees key won't be re-dted at ~rep_type if the */
          static_cast<value_type*>(spc_ptr())+1; /* next statement throws */
        value_ptr=new(spc_ptr())value_type(k);
      }
    }

    void copy_value()const
    {
      if(!value_cted())value_ptr=new(spc_ptr())value_type(*value_ptr);
    }

    mutable typename boost::aligned_storage<
      (sizeof(key_type)>sizeof(value_type))?
        sizeof(key_type):sizeof(value_type),
      (boost::alignment_of<key_type>::value >
       boost::alignment_of<value_type>::value)?
        boost::alignment_of<key_type>::value:
        boost::alignment_of<value_type>::value
    >::type                                    spc;
    mutable const value_type*                  value_ptr;
  };

  static void construct_value(const rep_type& r)
  {
    r.construct_value();
  }

  static void copy_value(const rep_type& r)
  {
    r.copy_value();
  }
};

template<typename Key,typename Value>
struct regular_key_value:value_marker
{
  typedef Key   key_type;
  typedef Value value_type;

  class rep_type
  {
  public:
    /* template ctors */

#define BOOST_FLYWEIGHT_PERFECT_FWD_NAME explicit rep_type
#define BOOST_FLYWEIGHT_PERFECT_FWD_BODY(n) \
  :key(BOOST_PP_ENUM_PARAMS(n,t)),value_ptr(0){}
#include <boost/flyweight/detail/perfect_fwd.hpp>

    rep_type(const value_type& x):key(no_key_from_value_failure()){}

    rep_type(const rep_type& x):key(x.key),value_ptr(0){}

    ~rep_type()
    {
      if(value_ptr)value_ptr->~value_type();
    }

    operator const key_type&()const{return key;}

    operator const value_type&()const
    {
      /* This is always called after construct_value(),so we access spc
       * directly rather than through value_ptr to save us an indirection.
       */

      return *static_cast<value_type*>(spc_ptr());
    }

  private:
    friend struct regular_key_value;

    void* spc_ptr()const{return static_cast<void*>(&spc);}

    struct no_key_from_value_failure
    {
      BOOST_MPL_ASSERT_MSG(
        false,
        NO_KEY_FROM_VALUE_CONVERSION_PROVIDED,
        (key_type,value_type));

      operator const key_type&()const;
    };

    void construct_value()const
    {
      if(!value_ptr)value_ptr=new(spc_ptr())value_type(key);
    }

    key_type                                 key;
    mutable typename boost::aligned_storage<
      sizeof(value_type),
      boost::alignment_of<value_type>::value
    >::type                                  spc;
    mutable const value_type*                value_ptr;
  };

  static void construct_value(const rep_type& r)
  {
    r.construct_value();
  }

  static void copy_value(const rep_type&){}
};

} /* namespace flyweights::detail */

template<typename Key,typename Value,typename KeyFromValue>
struct key_value:
  mpl::if_<
    is_same<KeyFromValue,no_key_from_value>,
    detail::regular_key_value<Key,Value>,
    detail::optimized_key_value<Key,Value,KeyFromValue>
  >::type
{};

} /* namespace flyweights */

} /* namespace boost */

#endif

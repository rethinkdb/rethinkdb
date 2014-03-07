//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <memory>
#else
#include <boost/tr1/memory.hpp>
#endif

#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/type_traits/is_same.hpp>
#include "verify_return.hpp"

struct base
{
   char member();
};
struct derived : public base{};
struct derived_deleter
{
   void operator()(derived*b)
   {
      delete b;
   }
};

struct abstract_base
{
   virtual ~abstract_base();
};

struct concrete : public abstract_base
{};

void noop(){}

struct shared_self : public std::tr1::enable_shared_from_this<shared_self>
{
   typedef std::tr1::enable_shared_from_this<shared_self> base_type;
   shared_self() : base_type(){}
   shared_self(const shared_self& s) : base_type(s){}
   ~shared_self(){}
   // implicit assignment:
   //shared_self& operator=(const shared_self& s)
   //{
   //   return *this;
   //}
};

int main()
{
   // bad_weak_ptr:
   std::tr1::bad_weak_ptr b;
   BOOST_STATIC_ASSERT((::boost::is_base_and_derived<std::exception, std::tr1::bad_weak_ptr>::value));

   // shared_ptr:
   typedef std::tr1::shared_ptr<derived> pderived;
   typedef std::tr1::shared_ptr<base> pbase;
   typedef std::tr1::shared_ptr<abstract_base> pabase;
   BOOST_STATIC_ASSERT((::boost::is_same<pderived::element_type, derived>::value));
   pderived pd1;
   pabase pb1(new concrete());
   pbase pb2(new derived(), derived_deleter());
   pderived pd2(pd1);
   pbase pb3(pd1);
   std::tr1::weak_ptr<derived>* pweak = 0;
   pbase pb4(*pweak);
   std::auto_ptr<derived>* pap = 0;
   pbase pb5(*pap);
   pb2 = pb3;
   pb2 = pd1;
   pb2 = *pap;
   pb2.swap(pb3);
   pb2.reset();
   pb1.reset(new concrete());
   pb2.reset(new derived(), derived_deleter());
   verify_return_type(pb2.get(), static_cast<base*>(0));
   verify_return_type(*pb2, base());
   verify_return_type(pb2->member(), char(0));
   verify_return_type(pb2.use_count(), long(0));
   verify_return_type(pb2.unique(), bool(0));
   if(pb2) { noop(); }
   if(!pb3) { noop(); }
   if(pb2 && pb3) { noop(); }
   // heterogeneous compare:
   verify_return_type(pd1 == pb2, bool());
   verify_return_type(pd1 != pb2, bool());
   verify_return_type(pd1 < pb2, bool());
   std::cout << pb1 << pb2 << std::endl;
   std::tr1::swap(pb2, pb3);
   swap(pb2, pb3);  // ADL
   verify_return_type(std::tr1::static_pointer_cast<derived>(pb2), pderived());
   verify_return_type(std::tr1::dynamic_pointer_cast<concrete>(pb1), std::tr1::shared_ptr<concrete>());
   verify_return_type(std::tr1::const_pointer_cast<base>(std::tr1::shared_ptr<const base>()), std::tr1::shared_ptr<base>());
   verify_return_type(std::tr1::get_deleter<derived_deleter>(pb2), static_cast<derived_deleter*>(0));

   // weak_ptr:
   typedef std::tr1::weak_ptr<base> wpb_t;
   BOOST_STATIC_ASSERT((::boost::is_same<wpb_t::element_type, base>::value));
   wpb_t wpb1;
   wpb_t wpb2(pd1);
   wpb_t wpb3(wpb1);
   std::tr1::weak_ptr<derived> wpd;
   wpb_t wpb4(wpd);
   wpb4 = wpb1;
   wpb4 = wpd;
   wpb4 = pd1;
   wpb4.swap(wpb1);
   wpb4.reset();
   verify_return_type(wpb4.use_count(), long(0));
   verify_return_type(wpb4.expired(), bool(0));
   verify_return_type(wpb4.lock(), pb2);

   // enable_shared_from_this:
   typedef std::tr1::shared_ptr<shared_self> pshared_self;
   typedef std::tr1::shared_ptr<const shared_self> pcshared_self;
   pshared_self sf1(new shared_self());
   pshared_self sf2(new shared_self(*sf1));
   *sf2 = *sf1;
   pcshared_self csf(sf1);
   verify_return_type(sf1->shared_from_this(), sf1);
   verify_return_type(csf->shared_from_this(), csf);
}


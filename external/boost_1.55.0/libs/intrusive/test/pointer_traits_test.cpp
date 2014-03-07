//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/intrusive/pointer_traits.hpp>

template<class T>
class CompleteSmartPtr
{
   template <class U>
   friend class CompleteSmartPtr;

   public:

   #if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
   template <class U> using rebind = CompleteSmartPtr<U>;
   #else
   template <class U> struct rebind
   {  typedef CompleteSmartPtr<U> other;  };
   #endif

   typedef char          difference_type;
   typedef T             element_type;

   CompleteSmartPtr()
    : ptr_(0)
   {}

   explicit CompleteSmartPtr(T &p)
    : ptr_(&p)
   {}

   CompleteSmartPtr(const CompleteSmartPtr &c)
   {  this->ptr_ = c.ptr_; }

   CompleteSmartPtr & operator=(const CompleteSmartPtr &c)
   {  this->ptr_ = c.ptr_; }

   static CompleteSmartPtr pointer_to(T &r)
   {  return CompleteSmartPtr(r);  }

   T * operator->() const
   {  return ptr_;  }

   T & operator *() const
   {  return *ptr_;  }

   template<class U>
   static CompleteSmartPtr static_cast_from(const CompleteSmartPtr<U> &uptr)
   {  return CompleteSmartPtr(*static_cast<element_type*>(uptr.ptr_));  }

   template<class U>
   static CompleteSmartPtr const_cast_from(const CompleteSmartPtr<U> &uptr)
   {  return CompleteSmartPtr(*const_cast<element_type*>(uptr.ptr_));  }

   template<class U>
   static CompleteSmartPtr dynamic_cast_from(const CompleteSmartPtr<U> &uptr)
   {  return CompleteSmartPtr(*dynamic_cast<element_type*>(uptr.ptr_));  }

   friend bool operator ==(const CompleteSmartPtr &l, const CompleteSmartPtr &r)
   {  return l.ptr_ == r.ptr_; }

   friend bool operator !=(const CompleteSmartPtr &l, const CompleteSmartPtr &r)
   {  return l.ptr_ != r.ptr_; }

   private:
   T *ptr_;
};


template<class T>
class SimpleSmartPtr
{
   public:

   SimpleSmartPtr()
    : ptr_(0)
   {}

   explicit SimpleSmartPtr(T *p)
    : ptr_(p)
   {}

   SimpleSmartPtr(const SimpleSmartPtr &c)
   {  this->ptr_ = c.ptr_; }

   SimpleSmartPtr & operator=(const SimpleSmartPtr &c)
   {  this->ptr_ = c.ptr_; }

   friend bool operator ==(const SimpleSmartPtr &l, const SimpleSmartPtr &r)
   {  return l.ptr_ == r.ptr_; }

   friend bool operator !=(const SimpleSmartPtr &l, const SimpleSmartPtr &r)
   {  return l.ptr_ != r.ptr_; }

   T* operator->() const
   {  return ptr_;  }

   T & operator *() const
   {  return *ptr_;  }

   private:
   T *ptr_;
};

class B{ public: virtual ~B(){} };
class D : public B {};
class DD : public virtual B {};

int main()
{
   int dummy;

   //Raw pointer
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       <int*>::element_type, int>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       <int*>::pointer, int*>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       <int*>::difference_type, std::ptrdiff_t>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       <int*>::rebind_pointer<double>::type
                       , double*>::value ));
   if(boost::intrusive::pointer_traits<int*>::pointer_to(dummy) != &dummy){
      return 1;
   }
   if(boost::intrusive::pointer_traits<D*>::static_cast_from((B*)0)){
      return 1;
   }
   if(boost::intrusive::pointer_traits<D*>::const_cast_from((const D*)0)){
      return 1;
   }
   if(boost::intrusive::pointer_traits<DD*>::dynamic_cast_from((B*)0)){
      return 1;
   }

   //Complete smart pointer
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < CompleteSmartPtr<int> >::element_type, int>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < CompleteSmartPtr<int> >::pointer, CompleteSmartPtr<int> >::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < CompleteSmartPtr<int> >::difference_type, char>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < CompleteSmartPtr<int> >::rebind_pointer<double>::type
                       , CompleteSmartPtr<double> >::value ));
   if(boost::intrusive::pointer_traits< CompleteSmartPtr<int> >
      ::pointer_to(dummy) != CompleteSmartPtr<int>(dummy)){
      return 1;
   }
   if(boost::intrusive::pointer_traits< CompleteSmartPtr<D> >::
         static_cast_from(CompleteSmartPtr<B>()) != CompleteSmartPtr<D>()){
      return 1;
   }
   if(boost::intrusive::pointer_traits< CompleteSmartPtr<D> >::
         const_cast_from(CompleteSmartPtr<const D>()) != CompleteSmartPtr<D>()){
      return 1;
   }
   if(boost::intrusive::pointer_traits< CompleteSmartPtr<DD> >::
         dynamic_cast_from(CompleteSmartPtr<B>()) != CompleteSmartPtr<DD>()){
      return 1;
   }

   //Simple smart pointer
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < SimpleSmartPtr<int> >::element_type, int>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < SimpleSmartPtr<int> >::pointer, SimpleSmartPtr<int> >::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < SimpleSmartPtr<int> >::difference_type, std::ptrdiff_t>::value ));
   BOOST_STATIC_ASSERT(( boost::is_same<boost::intrusive::pointer_traits
                       < SimpleSmartPtr<int> >::rebind_pointer<double>::type
                       , SimpleSmartPtr<double> >::value ));
   if(boost::intrusive::pointer_traits< SimpleSmartPtr<int> >
      ::pointer_to(dummy) != SimpleSmartPtr<int>(&dummy)){
      return 1;
   }
   if(boost::intrusive::pointer_traits< SimpleSmartPtr<D> >::
         static_cast_from(SimpleSmartPtr<B>()) != SimpleSmartPtr<D>()){
      return 1;
   }
   if(boost::intrusive::pointer_traits< SimpleSmartPtr<D> >::
         const_cast_from(SimpleSmartPtr<const D>()) != SimpleSmartPtr<D>()){
      return 1;
   }
   if(boost::intrusive::pointer_traits< SimpleSmartPtr<DD> >::
         dynamic_cast_from(SimpleSmartPtr<B>()) != SimpleSmartPtr<DD>()){
      return 1;
   }
   return 0;
}

#include <boost/intrusive/detail/config_end.hpp>

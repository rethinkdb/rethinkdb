//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright David Abrahams, Vicente Botet, Ion Gaztanaga 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
#include <boost/move/utility.hpp>

#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/is_class.hpp>
#include <cassert>
#include <new>
#include <boost/move/detail/move_helpers.hpp>


enum ConstructionType { Copied, Moved, Other };

class conversion_source
{
   public:
   conversion_source(){}
   operator int() const { return 0; }
};

class conversion_target
{
   ConstructionType c_type_;
   public:
   conversion_target(conversion_source)
      { c_type_ = Other; }
   conversion_target()
      { c_type_ = Other; }
   conversion_target(const conversion_target &)
      { c_type_ = Copied; }
   ConstructionType construction_type() const
      { return c_type_; }
};


class conversion_target_copymovable
{
   ConstructionType c_type_;
   BOOST_COPYABLE_AND_MOVABLE(conversion_target_copymovable)
   public:
   conversion_target_copymovable()
      { c_type_ = Other; }
   conversion_target_copymovable(conversion_source)
      { c_type_ = Other; }
   conversion_target_copymovable(const conversion_target_copymovable &)
      { c_type_ = Copied; }
   conversion_target_copymovable(BOOST_RV_REF(conversion_target_copymovable) )
      { c_type_ = Moved; }
   conversion_target_copymovable &operator=(BOOST_RV_REF(conversion_target_copymovable) )
      { c_type_ = Moved; return *this; }
   conversion_target_copymovable &operator=(BOOST_COPY_ASSIGN_REF(conversion_target_copymovable) )
      { c_type_ = Copied; return *this; }
   ConstructionType construction_type() const
      {  return c_type_; }
};

class conversion_target_movable
{
   ConstructionType c_type_;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(conversion_target_movable)
   public:
   conversion_target_movable()
      { c_type_ = Other; }
   conversion_target_movable(conversion_source)
      { c_type_ = Other; }
   conversion_target_movable(BOOST_RV_REF(conversion_target_movable) )
      { c_type_ = Moved; }
   conversion_target_movable &operator=(BOOST_RV_REF(conversion_target_movable) )
      { c_type_ = Moved; return *this; }
   ConstructionType construction_type() const
      {  return c_type_; }
};


template<class T>
class container
{
   T *storage_;
   public:
   struct const_iterator{};
   struct iterator : const_iterator{};
   container()
      : storage_(0)
   {}

   ~container()
   {  delete storage_; }

   container(const container &c)
      : storage_(c.storage_ ? new T(*c.storage_) : 0)
   {}

   container &operator=(const container &c)
   {
      if(storage_){
         delete storage_;
         storage_ = 0;
      }
      storage_ = c.storage_ ? new T(*c.storage_) : 0;
      return *this;
   }

   BOOST_MOVE_CONVERSION_AWARE_CATCH(push_back, T, void, priv_push_back)

   BOOST_MOVE_CONVERSION_AWARE_CATCH_1ARG(insert, T, iterator, priv_insert, const_iterator, const_iterator)

    template <class Iterator>
   iterator insert(Iterator, Iterator){ return iterator(); }

   ConstructionType construction_type() const
     {  return construction_type_impl(typename ::boost::is_class<T>::type()); }

   ConstructionType construction_type_impl(::boost::true_type) const
     {  return storage_->construction_type(); }

   ConstructionType construction_type_impl(::boost::false_type) const
     {  return Copied; }

   iterator begin() const { return iterator(); }
   
   private:
   template<class U>
    void priv_construct(BOOST_MOVE_CATCH_FWD(U) x)
   {
      if(storage_){
         delete storage_;
         storage_ = 0;
      }
      storage_ = new T(::boost::forward<U>(x));
   }

   template<class U>
   void priv_push_back(BOOST_MOVE_CATCH_FWD(U) x)
   {  priv_construct(::boost::forward<U>(x));   }

   template<class U>
   iterator priv_insert(const_iterator, BOOST_MOVE_CATCH_FWD(U) x)
   {  priv_construct(::boost::forward<U>(x));   return iterator();   }
};

class recursive_container
{
   BOOST_COPYABLE_AND_MOVABLE(recursive_container)
   public:
   recursive_container()
   {}

   recursive_container(const recursive_container &c)
      : container_(c.container_)
   {}

   recursive_container(BOOST_RV_REF(recursive_container) c)
      : container_(::boost::move(c.container_))
   {}

   recursive_container & operator =(BOOST_COPY_ASSIGN_REF(recursive_container) c)
   {
      container_= c.container_;
      return *this;
   }

   recursive_container & operator =(BOOST_RV_REF(recursive_container) c)
   {
      container_= ::boost::move(c.container_);
      return *this;
   }

   container<recursive_container> container_;
   friend bool operator< (const recursive_container &a, const recursive_container &b)
   {  return &a < &b;   }
};


int main()
{
   conversion_target_movable a;
   conversion_target_movable b(::boost::move(a));
   {
      container<conversion_target> c;
      {
         conversion_target x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         const conversion_target x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_target());
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), conversion_target());
         assert(c.construction_type() == Copied);
      }
      {
         conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         const conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_source());
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), conversion_source());
         assert(c.construction_type() == Copied);
      }
   }

   {
      container<conversion_target_copymovable> c;
      {
         conversion_target_copymovable x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         const conversion_target_copymovable x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_target_copymovable());
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), conversion_target_copymovable());
         assert(c.construction_type() == Moved);
      }
      {
         conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Moved);
      }
      {
         const conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Moved);
      }
      {
         c.push_back(conversion_source());
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), conversion_source());
         assert(c.construction_type() == Moved);
      }
   }
   {
      container<conversion_target_movable> c;
      //This should not compile
      //{
      //   conversion_target_movable x;
      //   c.push_back(x);
      //   assert(c.construction_type() == Copied);
      //}
      //{
      //   const conversion_target_movable x;
      //   c.push_back(x);
      //   assert(c.construction_type() == Copied);
      //}
      {
         c.push_back(conversion_target_movable());
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), conversion_target_movable());
         assert(c.construction_type() == Moved);
      }
      {
         conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Moved);
      }
      {
         const conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Moved);
      }
      {
         c.push_back(conversion_source());
         assert(c.construction_type() == Moved);
         c.insert(c.begin(), conversion_source());
         assert(c.construction_type() == Moved);
      }
   }
   {
      container<int> c;
      {
         int x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), c.construction_type());
         assert(c.construction_type() == Copied);
      }
      {
         const int x = 0;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(int(0));
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), int(0));
         assert(c.construction_type() == Copied);
      }
      {
         conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }

      {
         const conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_source());
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), conversion_source());
         assert(c.construction_type() == Copied);
      }
      //c.insert(c.begin(), c.begin());
   }

   {
      container<int> c;
      {
         int x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), c.construction_type());
         assert(c.construction_type() == Copied);
      }
      {
         const int x = 0;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(int(0));
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), int(0));
         assert(c.construction_type() == Copied);
      }
      {
         conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }

      {
         const conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_source());
         assert(c.construction_type() == Copied);
         c.insert(c.begin(), conversion_source());
         assert(c.construction_type() == Copied);
      }
      c.insert(c.begin(), c.begin());
   }

   {
      recursive_container c;
      recursive_container internal;
      c.container_.insert(c.container_.begin(), recursive_container());
      c.container_.insert(c.container_.begin(), internal);
      c.container_.insert(c.container_.begin(), c.container_.begin());
   }

   return 0;
}

#include <boost/move/detail/config_end.hpp>

/*
#include <boost/move/utility.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/is_class.hpp>
#include <cassert>


enum ConstructionType { Default, Copied, Moved, Other };

class conversion_source
{
   public:
   conversion_source(){}
   operator int() const { return 0; }
};

class conversion_target
{
   ConstructionType c_type_;
   public:
   conversion_target(conversion_source)
      { c_type_ = Other; }
   conversion_target()
      { c_type_ = Default; }
   conversion_target(const conversion_target &)
      { c_type_ = Copied; }
   ConstructionType construction_type() const
      { return c_type_; }
};


class conversion_target_copymovable
{
   ConstructionType c_type_;
   BOOST_COPYABLE_AND_MOVABLE(conversion_target_copymovable)
   public:
   conversion_target_copymovable()
      { c_type_ = Default; }
   conversion_target_copymovable(conversion_source)
      { c_type_ = Other; }
   conversion_target_copymovable(const conversion_target_copymovable &)
      { c_type_ = Copied; }
   conversion_target_copymovable(BOOST_RV_REF(conversion_target_copymovable) )
      { c_type_ = Moved; }
   conversion_target_copymovable &operator=(BOOST_RV_REF(conversion_target_copymovable) )
      { c_type_ = Moved; return *this; }
   conversion_target_copymovable &operator=(BOOST_COPY_ASSIGN_REF(conversion_target_copymovable) )
      { c_type_ = Copied; return *this; }
   ConstructionType construction_type() const
      {  return c_type_; }
};

class conversion_target_movable
{
   ConstructionType c_type_;
   BOOST_MOVABLE_BUT_NOT_COPYABLE(conversion_target_movable)
   public:
   conversion_target_movable()
      { c_type_ = Default; }
   conversion_target_movable(conversion_source)
      { c_type_ = Other; }
   conversion_target_movable(BOOST_RV_REF(conversion_target_movable) )
      { c_type_ = Moved; }
   conversion_target_movable &operator=(BOOST_RV_REF(conversion_target_movable) )
      { c_type_ = Moved; return *this; }
   ConstructionType construction_type() const
      {  return c_type_; }
};

struct not_a_type;

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#define BOOST_MOVE_CATCH_CONST(U)  \
   typename ::boost::mpl::if_< ::boost::is_class<T>, BOOST_CATCH_CONST_RLVALUE(U), const U &>::type
#define BOOST_MOVE_CATCH_RVALUE(U)\
   typename ::boost::mpl::if_< ::boost::is_class<T>, BOOST_RV_REF(T), not_a_type>::type
#else
#define BOOST_MOVE_CATCH_CONST(U)  BOOST_CATCH_CONST_RLVALUE(U)
#define BOOST_MOVE_CATCH_RVALUE(U) BOOST_RV_REF(U)
#endif

// BEGIN JLH additional definitions...

template< class T > struct remove_const_remove_reference { typedef T type; };
template< class T > struct remove_const_remove_reference< const T > : remove_const_remove_reference<T> { };
template< class T > struct remove_const_remove_reference< volatile T > : remove_const_remove_reference<T> { };
template< class T > struct remove_const_remove_reference< const volatile T > : remove_const_remove_reference<T> { };
template< class T > struct remove_const_remove_reference< T& > : remove_const_remove_reference<T> { };
template< class T > struct remove_const_remove_reference< boost::rv<T> > : remove_const_remove_reference<T> { };

template< class T > struct add_reference_add_const { typedef T const & type; };
template< class T > struct add_reference_add_const< T& > { typedef T& type; };

template< class T, class U >
struct is_same_sans_const_sans_reference
    : boost::is_same<
          typename remove_const_remove_reference<T>::type,
          typename remove_const_remove_reference<U>::type
      >
{ };

template< class T > struct add_lvalue_reference { typedef T& type; };
template<>          struct add_lvalue_reference< void > { typedef void type; };
template<>          struct add_lvalue_reference< const void > { typedef const void type; };
template<>          struct add_lvalue_reference< volatile void > { typedef volatile void type; };
template<>          struct add_lvalue_reference< const volatile void > { typedef const volatile void type; };
template< class T > struct add_lvalue_reference< T& > { typedef T& type; };
template< class T > struct add_lvalue_reference< boost::rv<T> > { typedef T& type; };
template< class T > struct add_lvalue_reference< const boost::rv<T> > { typedef const T& type; };
template< class T > struct add_lvalue_reference< volatile boost::rv<T> > { typedef volatile T& type; };
template< class T > struct add_lvalue_reference< const volatile boost::rv<T> > { typedef const volatile T& type; };
template< class T > struct add_lvalue_reference< boost::rv<T>& > { typedef T& type; };
template< class T > struct add_lvalue_reference< const boost::rv<T>& > { typedef const T& type; };
template< class T > struct add_lvalue_reference< volatile boost::rv<T>& > { typedef volatile T& type; };
template< class T > struct add_lvalue_reference< const volatile boost::rv<T>& > { typedef const volatile T& type; };

template< class T > struct remove_rvalue_reference { typedef T type; };
template< class T > struct remove_rvalue_reference< boost::rv<T> > { typedef T type; };
template< class T > struct remove_rvalue_reference< const boost::rv<T> > { typedef T type; };
template< class T > struct remove_rvalue_reference< volatile boost::rv<T> > { typedef T type; };
template< class T > struct remove_rvalue_reference< const volatile boost::rv<T> > { typedef T type; };
template< class T > struct remove_rvalue_reference< boost::rv<T>& > { typedef T type; };
template< class T > struct remove_rvalue_reference< const boost::rv<T>& > { typedef T type; };
template< class T > struct remove_rvalue_reference< volatile boost::rv<T>& > { typedef T type; };
template< class T > struct remove_rvalue_reference< const volatile boost::rv<T>& > { typedef T type; };

template< class T >
struct add_rvalue_reference
    : boost::mpl::if_<
          boost::has_move_emulation_enabled<T>,
          boost::rv<T>&,
          T
      >
{ };

template< class T >
struct remove_crv
    : remove_rvalue_reference<T>
{ };
template< class T >
struct remove_crv< const T >
    : remove_rvalue_reference<T>
{ };

template< class T >
inline typename add_lvalue_reference<T>::type
as_lvalue(T& x)
{ return x; }

// END JLH additional definitions...

template<class T>
class container
{
   typename ::boost::aligned_storage<sizeof(T), ::boost::alignment_of<T>::value>::type storage_;
   public:

   ConstructionType construction_type() const
      {  return construction_type_impl(typename ::boost::is_class<T>::type()); }
   ConstructionType construction_type_impl(::boost::true_type) const
      {  return reinterpret_cast<const T&>(storage_).construction_type(); }
   ConstructionType construction_type_impl(::boost::false_type) const
      {  return Copied; }

#if 0

   // Ion's original implementation

   void push_back(BOOST_MOVE_CATCH_CONST(T) x)
   {  return priv_push_back(static_cast<const T&>(x)); }

   void push_back(BOOST_MOVE_CATCH_RVALUE(T) x)
   {  return priv_push_back(::boost::move(x));  }

   //Tricks for C++03
   #if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
   void push_back(T &x)
   { priv_push_back(const_cast<const T &>(x)); }

   template<class U>
   typename ::boost::enable_if_c
                     <  ::boost::is_class<T>::value &&
                        ::boost::is_same<T, U>::value &&
                        !::boost::has_move_emulation_enabled<U>::value
                     >::type
   push_back(const U &u)
   { return priv_push_back(u); }

   template<class U>
   typename ::boost::enable_if_c
                     <  ::boost::is_class<U>::value   &&
                       !::boost::is_same<T, U>::value &&
                       !::boost::move_detail::is_rv<U>::value
                     >::type
   push_back(const U &u)
   {
      T t(u);
      priv_push_back(::boost::move(t));
   }

   #endif

   private:
   template<class U>
   void priv_push_back(BOOST_FWD_REF(U) x)
      { new (&storage_) T(::boost::forward<U>(x)); }

#else // #if 0|1

    // JLH's current implementation (roughly; only showing C++03 here)

    template< class U >
    typename boost::disable_if<
        is_same_sans_const_sans_reference<U,T>
    >::type
    push_back(const U& x) { priv_push_back(as_lvalue(x)); }
    template< class U >
    void
    push_back(U& x) { priv_push_back(x); }

    typedef typename add_reference_add_const<
        typename add_rvalue_reference<T>::type
    >::type rv_param_type;

    void
    push_back(rv_param_type x) { priv_push_back(x); }
private:
    template< class U >
    void
    priv_push_back(U& x) { new (&storage_) T(x); }

#endif // #if 0|1
};


int main()
{
   conversion_target_movable a;
   conversion_target_movable b(::boost::move(a));
   {
      container<conversion_target> c;
      {
         conversion_target x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         const conversion_target x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_target());
         assert(c.construction_type() == Copied);
      }
      {
         conversion_source x;
         c.push_back(x);
         //assert(c.construction_type() == Copied);
         assert(c.construction_type() == Other);
      }
      {
         const conversion_source x;
         c.push_back(x);
         //assert(c.construction_type() == Copied);
         assert(c.construction_type() == Other);
      }
      {
         c.push_back(conversion_source());
         //assert(c.construction_type() == Copied);
         assert(c.construction_type() == Other);
      }
   }

   {
      container<conversion_target_copymovable> c;
      {
         conversion_target_copymovable x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         const conversion_target_copymovable x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_target_copymovable());
         assert(c.construction_type() == Moved);
      }
      {
         conversion_source x;
         c.push_back(x);
         //assert(c.construction_type() == Moved);
         assert(c.construction_type() == Other);
      }
      {
         const conversion_source x;
         c.push_back(x);
         //assert(c.construction_type() == Moved);
         assert(c.construction_type() == Other);
      }
      {
         c.push_back(conversion_source());
         //assert(c.construction_type() == Moved);
         assert(c.construction_type() == Other);
      }
   }
   {
      container<conversion_target_movable> c;
      //This should not compile
      //{
      //   conversion_target_movable x;
      //   c.push_back(x);
      //   assert(c.construction_type() == Copied);
      //}
      //{
      //   const conversion_target_movable x;
      //   c.push_back(x);
      //   assert(c.construction_type() == Copied);
      //}
      {
         c.push_back(conversion_target_movable());
         assert(c.construction_type() == Moved);
      }
      {
         conversion_source x;
         c.push_back(x);
         //assert(c.construction_type() == Moved);
         assert(c.construction_type() == Other);
      }
      {
         const conversion_source x;
         c.push_back(x);
         //assert(c.construction_type() == Moved);
         assert(c.construction_type() == Other);
      }
      {
         c.push_back(conversion_source());
         //assert(c.construction_type() == Moved);
         assert(c.construction_type() == Other);
      }
   }
   {
      container<int> c;
      {
         int x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         const int x = 0;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(int(0));
         assert(c.construction_type() == Copied);
      }
      {
         conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }

      {
         const conversion_source x;
         c.push_back(x);
         assert(c.construction_type() == Copied);
      }
      {
         c.push_back(conversion_source());
         assert(c.construction_type() == Copied);
      }
   }

   return 0;
}
*/

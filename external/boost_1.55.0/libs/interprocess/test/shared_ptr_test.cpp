//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Peter Dimov 2002-2005, 2007.
// (C) Copyright Ion Gaztanaga 2006-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/smart_ptr/weak_ptr.hpp>
#include <boost/interprocess/smart_ptr/enable_shared_from_this.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/smart_ptr/deleter.hpp>
#include <boost/interprocess/smart_ptr/scoped_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <string>
#include "get_process_id_name.hpp"


#include <boost/interprocess/sync/upgradable_lock.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>

using namespace boost::interprocess;

class base_class
{
   public:
   virtual ~base_class()
   {}
};

class derived_class
   :  public base_class
{
   public:
   virtual ~derived_class()
   {}
};

int simple_test()
{
   typedef allocator<base_class, managed_shared_memory::segment_manager>
      base_class_allocator;
   typedef deleter<base_class, managed_shared_memory::segment_manager>
      base_deleter_t;
   typedef shared_ptr<base_class, base_class_allocator, base_deleter_t>    base_shared_ptr;

   std::string process_name;
   test::get_process_id_name(process_name);

   shared_memory_object::remove(process_name.c_str());
   {
      managed_shared_memory shmem(create_only, process_name.c_str(), 10000);

      {
         base_shared_ptr s_ptr(base_shared_ptr::pointer(0),
                           base_class_allocator(shmem.get_segment_manager()),
                           base_deleter_t(shmem.get_segment_manager()));

         base_shared_ptr s_ptr2(shmem.construct<base_class>("base_class")(),
                              base_class_allocator(shmem.get_segment_manager()),
                              base_deleter_t(shmem.get_segment_manager()));

         base_shared_ptr s_ptr3(offset_ptr<derived_class>(shmem.construct<derived_class>("derived_class")()),
                              base_class_allocator(shmem.get_segment_manager()),
                              base_deleter_t(shmem.get_segment_manager()));

         if(s_ptr3.get_deleter()   == 0){
            return 1;
         }
         //if(s_ptr3.get_allocator() == 0){
            //return 1;
         //}

         base_shared_ptr s_ptr_empty;

         if(s_ptr_empty.get_deleter()   != 0){
            return 1;
         }
         //if(s_ptr_empty.get_allocator() != 0){
            //return 1;
         //}
      }
   }
   shared_memory_object::remove(process_name.c_str());
   return 0;
}

int string_shared_ptr_vector_insertion_test()
{
   //Allocator of chars
   typedef allocator<char, managed_shared_memory::segment_manager >
      char_allocator_t;

   //A shared memory string class
   typedef basic_string<char, std::char_traits<char>,
                        char_allocator_t> string_t;

   //A shared memory string allocator
   typedef allocator<string_t, managed_shared_memory::segment_manager>
      string_allocator_t;

   //A deleter for shared_ptr<> that erases a shared memory string
   typedef deleter<string_t, managed_shared_memory::segment_manager>
      string_deleter_t;

   //A shared pointer that points to a shared memory string and its instantiation
   typedef shared_ptr<string_t, string_allocator_t, string_deleter_t> string_shared_ptr_t;

   //An allocator for shared pointers to a string in shared memory
   typedef allocator<string_shared_ptr_t, managed_shared_memory::segment_manager>
      string_shared_ptr_allocator_t;

   //A weak pointer that points to a shared memory string and its instantiation
   typedef weak_ptr<string_t, string_allocator_t, string_deleter_t> string_weak_ptr_t;

   //An allocator for weak pointers to a string in shared memory
   typedef allocator<string_weak_ptr_t, managed_shared_memory::segment_manager >
      string_weak_ptr_allocator_t;

   //A vector of shared pointers to strings (all in shared memory) and its instantiation
   typedef vector<string_shared_ptr_t, string_shared_ptr_allocator_t>
      string_shared_ptr_vector_t;

   //A vector of weak pointers to strings (all in shared memory) and its instantiation
   typedef vector<string_weak_ptr_t, string_weak_ptr_allocator_t>
      string_weak_ptr_vector_t;

   std::string process_name;
   test::get_process_id_name(process_name);

   //A shared memory managed memory classes
   shared_memory_object::remove(process_name.c_str());
   {
      managed_shared_memory shmem(create_only, process_name.c_str(), 20000);

      {
         const int NumElements = 100;
         //Construct the allocator of strings
         string_allocator_t string_allocator(shmem.get_segment_manager());
         //Construct the allocator of a shared_ptr to string
         string_shared_ptr_allocator_t string_shared_ptr_allocator(shmem.get_segment_manager());
         //Construct the allocator of a shared_ptr to string
         string_weak_ptr_allocator_t string_weak_ptr_allocator(shmem.get_segment_manager());
         //This is a string deleter using destroy_ptr() function of the managed_shared_memory
         string_deleter_t deleter(shmem.get_segment_manager());
         //Create a string in shared memory, to avoid leaks with exceptions use
         //scoped ptr until we store this pointer in the shared ptr
         scoped_ptr<string_t, string_deleter_t> scoped_string
         (shmem.construct<string_t>(anonymous_instance)(string_allocator),
            deleter);
         //Now construct a shared pointer to a string
         string_shared_ptr_t string_shared_ptr (scoped_string.get(),
                                                string_shared_ptr_allocator,
                                                deleter);
         //Check use count is just one
         if(!string_shared_ptr.unique()){
            return 1;
         }
         //We don't need the scoped_ptr anonymous since the raw pointer is in the shared ptr
         scoped_string.release();
         //Now fill a shared memory vector of shared_ptrs to a string
         string_shared_ptr_vector_t my_sharedptr_vector(string_shared_ptr_allocator);
         my_sharedptr_vector.insert(my_sharedptr_vector.begin(), NumElements, string_shared_ptr);
         //Insert in the middle to test movability
         my_sharedptr_vector.insert(my_sharedptr_vector.begin() + my_sharedptr_vector.size()/2, NumElements, string_shared_ptr);
         //Now check the shared count is the objects contained in the
         //vector plus string_shared_ptr
         if(string_shared_ptr.use_count() != static_cast<long>(my_sharedptr_vector.size()+1)){
            return 1;
         }
         //Now create a weak ptr from the shared_ptr
         string_weak_ptr_t string_weak_ptr (string_shared_ptr);
         //Use count should remain the same
         if(string_weak_ptr.use_count() != static_cast<long>(my_sharedptr_vector.size()+1)){
            return 1;
         }
         //Now reset the local shared_ptr and check use count
         string_shared_ptr.reset();
         if(string_weak_ptr.use_count() != static_cast<long>(my_sharedptr_vector.size())){
            return 1;
         }
         //Now reset the local shared_ptr's use count should be zero
         if(string_shared_ptr.use_count() != 0){
            return 1;
         }
         //Now recreate the shared ptr from the weak ptr
         //and recheck use count
         string_shared_ptr = string_shared_ptr_t(string_weak_ptr);
         if(string_shared_ptr.use_count() != static_cast<long>(my_sharedptr_vector.size()+1)){
            return 1;
         }
         //Now fill a vector of weak_ptr-s
         string_weak_ptr_vector_t my_weakptr_vector(string_weak_ptr_allocator);
         my_weakptr_vector.insert(my_weakptr_vector.begin(), NumElements, string_weak_ptr);
         //The shared count should remain the same
         if(string_shared_ptr.use_count() != static_cast<long>(my_sharedptr_vector.size()+1)){
            return 1;
         }
         //So weak pointers should be fine
         string_weak_ptr_vector_t::iterator beg = my_weakptr_vector.begin(),
                                          end = my_weakptr_vector.end();
         for(;beg != end; ++beg){
            if(beg->expired()){
               return 1;
            }
            //The shared pointer constructed from weak ptr should
            //be the same as the original, since all weak pointer
            //point the the same object
            if(string_shared_ptr_t(*beg) != string_shared_ptr){
               return 1;
            }
         }
         //Now destroy all the shared ptr-s of the shared ptr vector
         my_sharedptr_vector.clear();
         //The only alive shared ptr should be the local one
         if(string_shared_ptr.use_count() != 1){
            return 1;
         }
         //Now we invalidate the last alive shared_ptr
         string_shared_ptr.reset();
         //Now all weak pointers should have expired
         beg = my_weakptr_vector.begin();
         end = my_weakptr_vector.end();
         for(;beg != end; ++beg){
            if(!beg->expired()){
               return 1;
            }
            bool success = false;
            //Now this should throw
            try{
               string_shared_ptr_t dummy(*beg);
               //We should never reach here
               return 1;
            }
            catch(const boost::interprocess::bad_weak_ptr &){
               success = true;
            }
            if(!success){
               return 1;
            }
         }
         //Clear weak ptr vector
         my_weakptr_vector.clear();
         //Now lock returned shared ptr should return null
         if(string_weak_ptr.lock().get()){
            return 1;
         }
         //Reset weak_ptr
         string_weak_ptr.reset();
      }
   }
   shared_memory_object::remove(process_name.c_str());
   return 0;
}
//
//  This part is taken from shared_ptr_basic_test.cpp
//
//  Copyright (c) 2001, 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2006 Ion Gaztanaga
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

static int cnt = 0;

struct X
{
   X(){ ++cnt;   }
   // virtual destructor deliberately omitted
   ~X(){   --cnt;    }

   virtual int id() const
   {  return 1;   }

   private:
   X(X const &);
   X & operator= (X const &);
};

struct Y: public X
{
   Y(){  ++cnt;   }
   ~Y(){ --cnt;   }

   virtual int id() const
   {  return 2;   }

   private:
   Y(Y const &);
   Y & operator= (Y const &);
};

int * get_object()
{  ++cnt;   return &cnt;   }

void release_object(int * p)
{  BOOST_TEST(p == &cnt);  --cnt;   }

template<class T, class A, class D>
void test_is_X(shared_ptr<T, A, D> const & p)
{
   BOOST_TEST(p->id() == 1);
   BOOST_TEST((*p).id() == 1);
}

template<class T, class A, class D>
void test_is_X(weak_ptr<T, A, D> const & p)
{
   BOOST_TEST(p.get() != 0);
   BOOST_TEST(p.get()->id() == 1);
}

template<class T, class A, class D>
void test_is_Y(shared_ptr<T, A, D> const & p)
{
   BOOST_TEST(p->id() == 2);
   BOOST_TEST((*p).id() == 2);
}

template<class T, class A, class D>
void test_is_Y(weak_ptr<T, A, D> const & p)
{
   shared_ptr<T, A, D> q = p.lock();
   BOOST_TEST(q.get() != 0);
   BOOST_TEST(q->id() == 2);
}

template<class T, class T2>
void test_eq(T const & a, T2 const & b)
{
   BOOST_TEST(a == b);
   BOOST_TEST(!(a != b));
   BOOST_TEST(!(a < b));
   BOOST_TEST(!(b < a));
}

template<class T, class T2>
void test_ne(T const & a, T2 const & b)
{
   BOOST_TEST(!(a == b));
   BOOST_TEST(a != b);
   BOOST_TEST(a < b || b < a);
   BOOST_TEST(!(a < b && b < a));
}

template<class T, class U, class A, class D, class D2>
void test_shared(weak_ptr<T, A, D> const & a, weak_ptr<U, A, D2> const & b)
{
   BOOST_TEST(!(a < b));
   BOOST_TEST(!(b < a));
}

template<class T, class U, class A, class D, class D2>
void test_nonshared(weak_ptr<T, A, D> const & a, weak_ptr<U, A, D2> const & b)
{
   BOOST_TEST(a < b || b < a);
   BOOST_TEST(!(a < b && b < a));
}

template<class T, class U>
void test_eq2(T const & a, U const & b)
{
   BOOST_TEST(a == b);
   BOOST_TEST(!(a != b));
}

template<class T, class U>
void test_ne2(T const & a, U const & b)
{
   BOOST_TEST(!(a == b));
   BOOST_TEST(a != b);
}

template<class T, class A, class D>
void test_is_zero(shared_ptr<T, A, D> const & p)
{
   BOOST_TEST(!p);
   BOOST_TEST(p.get() == 0);
}

template<class T, class A, class D>
void test_is_nonzero(shared_ptr<T, A, D> const & p)
{
   // p? true: false is used to test p in a boolean context.
   // BOOST_TEST(p) is not guaranteed to test the conversion,
   // as the macro might test !!p instead.
   BOOST_TEST(p? true: false);
   BOOST_TEST(p.get() != 0);
}

int basic_shared_ptr_test()
{
   typedef allocator<void, managed_shared_memory::segment_manager>
      v_allocator_t;

   typedef deleter<X, managed_shared_memory::segment_manager>
      x_deleter_t;

   typedef deleter<Y, managed_shared_memory::segment_manager>
      y_deleter_t;

   typedef shared_ptr<X, v_allocator_t, x_deleter_t> x_shared_ptr;

   typedef shared_ptr<Y, v_allocator_t, y_deleter_t> y_shared_ptr;

   typedef weak_ptr<X, v_allocator_t, x_deleter_t> x_weak_ptr;

   typedef weak_ptr<Y, v_allocator_t, y_deleter_t> y_weak_ptr;

   std::string process_name;
   test::get_process_id_name(process_name);

   shared_memory_object::remove(process_name.c_str());
   {
      managed_shared_memory shmem(create_only, process_name.c_str(), 10000);
      {
         v_allocator_t  v_allocator (shmem.get_segment_manager());
         x_deleter_t    x_deleter   (shmem.get_segment_manager());
         y_deleter_t    y_deleter   (shmem.get_segment_manager());

         y_shared_ptr p (shmem.construct<Y>(anonymous_instance)(), v_allocator, y_deleter);
         x_shared_ptr p2(shmem.construct<X>(anonymous_instance)(), v_allocator, x_deleter);

         test_is_nonzero(p);
         test_is_nonzero(p2);
         test_is_Y(p);
         test_is_X(p2);
         test_ne(p, p2);

         {
            shared_ptr<X, v_allocator_t, y_deleter_t> q(p);
            test_eq(p, q);
         }

         y_shared_ptr p3 (dynamic_pointer_cast<Y>(p));
         shared_ptr<Y, v_allocator_t, x_deleter_t> p4 (dynamic_pointer_cast<Y>(p2));
         test_is_nonzero(p3);
         test_is_zero(p4);
         BOOST_TEST(p.use_count() == 2);
         BOOST_TEST(p2.use_count() == 1);
         BOOST_TEST(p3.use_count() == 2);
         test_is_Y(p3);
         test_eq2(p, p3);
         test_ne2(p2, p4);

         shared_ptr<void, v_allocator_t, y_deleter_t> p5(p);

         test_is_nonzero(p5);
         test_eq2(p, p5);
         BOOST_TEST(p5.use_count() == 3);

         x_weak_ptr wp1(p2);

         BOOST_TEST(!wp1.expired());
         BOOST_TEST(wp1.use_count() != 0);

         p.reset();
         p2.reset();
         p3.reset();
         p4.reset();

         test_is_zero(p);
         test_is_zero(p2);
         test_is_zero(p3);
         test_is_zero(p4);

         BOOST_TEST(p5.use_count() == 1);
         BOOST_TEST(wp1.expired());
         BOOST_TEST(wp1.use_count() == 0);

         try{
            x_shared_ptr sp1(wp1);
            BOOST_ERROR("shared_ptr<X, A, D> sp1(wp1) failed to throw");
         }
         catch(boost::interprocess::bad_weak_ptr const &)
         {}

         test_is_zero(wp1.lock());

         weak_ptr<X, v_allocator_t, y_deleter_t> wp2 = static_pointer_cast<X>(p5);

         BOOST_TEST(wp2.use_count() == 1);
         test_is_Y(wp2);
         test_nonshared(wp1, wp2);

         // Scoped to not affect the subsequent use_count() tests.
         {
            shared_ptr<X, v_allocator_t, y_deleter_t> sp2(wp2);
            test_is_nonzero(wp2.lock());
         }

         y_weak_ptr wp3 = dynamic_pointer_cast<Y>(wp2.lock());

         BOOST_TEST(wp3.use_count() == 1);
         test_shared(wp2, wp3);

         weak_ptr<X, v_allocator_t, y_deleter_t> wp4(wp3);

         BOOST_TEST(wp4.use_count() == 1);
         test_shared(wp2, wp4);

         wp1 = p2;
         test_is_zero(wp1.lock());

         wp1 = p4;

         x_weak_ptr wp5;

         bool b1 = wp1 < wp5;
         bool b2 = wp5 < wp1;

         y_shared_ptr p6 = static_pointer_cast<Y>(p5);
         p5.reset();
         p6.reset();

         BOOST_TEST(wp1.use_count() == 0);
         BOOST_TEST(wp2.use_count() == 0);
         BOOST_TEST(wp3.use_count() == 0);

         // Test operator< stability for std::set< weak_ptr<> >
         // Thanks to Joe Gottman for pointing this out
         BOOST_TEST(b1 == (wp1 < wp5));
         BOOST_TEST(b2 == (wp5 < wp1));
      }

      BOOST_TEST(cnt == 0);
   }
   shared_memory_object::remove(process_name.c_str());
   return boost::report_errors();
}

struct alias_tester
{
    int v_;

    explicit alias_tester( int v ): v_( v )
    {
    }

    ~alias_tester()
    {
        v_ = 0;
    }
};

void test_alias()
{
   typedef allocator<void, managed_shared_memory::segment_manager>
      v_allocator_t;

   typedef deleter<int, managed_shared_memory::segment_manager>
      int_deleter_t;

   typedef shared_ptr<int, v_allocator_t, int_deleter_t> int_shared_ptr;
   typedef shared_ptr<const int, v_allocator_t, int_deleter_t> const_int_shared_ptr;

   std::string process_name;
   test::get_process_id_name(process_name);

   shared_memory_object::remove(process_name.c_str());
   {
      managed_shared_memory shmem(create_only, process_name.c_str(), 10000);

      {
         int m = 0;
         int_shared_ptr p;
         int_shared_ptr p2( p, &m );

         BOOST_TEST( ipcdetail::to_raw_pointer(p2.get()) == &m );
         BOOST_TEST( p2? true: false );
         BOOST_TEST( !!p2 );
         BOOST_TEST( p2.use_count() == p.use_count() );
         BOOST_TEST( !( p < p2 ) && !( p2 < p ) );

         p2.reset( p, static_cast<int*>(0) );

         BOOST_TEST( p2.get() == 0 );

         BOOST_TEST( p2? false: true );
         BOOST_TEST( !p2 );
         BOOST_TEST( p2.use_count() == p.use_count() );
         BOOST_TEST( !( p < p2 ) && !( p2 < p ) );
      }

      {
         int m = 0;
         int_shared_ptr p(make_managed_shared_ptr
            (shmem.construct<int>(anonymous_instance)(), shmem));
         const_int_shared_ptr p2( p, &m );

         BOOST_TEST( ipcdetail::to_raw_pointer(p2.get()) == &m );
         BOOST_TEST( p2? true: false );
         BOOST_TEST( !!p2 );
         BOOST_TEST( p2.use_count() == p.use_count() );
         BOOST_TEST( !( p < p2 ) && !( p2 < p ) );
         int_shared_ptr p_nothrow(make_managed_shared_ptr
            (shmem.construct<int>(anonymous_instance)(), shmem, std::nothrow));
      }
   }
   shared_memory_object::remove(process_name.c_str());
}

int main()
{
   if(0 != simple_test())
      return 1;

   if(0 != string_shared_ptr_vector_insertion_test())
      return 1;

   if(0 != basic_shared_ptr_test())
      return 1;

   test_alias();
}

#include <boost/interprocess/detail/config_end.hpp>


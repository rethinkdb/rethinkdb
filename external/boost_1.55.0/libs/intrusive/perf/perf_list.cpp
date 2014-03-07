/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

//Includes for tests
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/config.hpp>
#include <list>
#include <functional>
#include <iostream>
#include <boost/intrusive/list.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace boost::posix_time;

//[perf_list_value_type
//Iteration and element count defines
const int NumIter = 100;
const int NumElements   = 50000;

using namespace boost::intrusive;

template<bool BigSize>  struct filler        {  int dummy[10];   };
template <>             struct filler<false> {};

template<bool BigSize> //The object for non-intrusive containers
struct test_class :  private filler<BigSize>
{
   int i_;
   test_class()               {}
   test_class(int i) :  i_(i) {}
   friend bool operator <(const test_class &l, const test_class &r)  {  return l.i_ < r.i_;  }
   friend bool operator >(const test_class &l, const test_class &r)  {  return l.i_ > r.i_;  }
};

template <bool BigSize, link_mode_type LinkMode>
struct itest_class   //The object for intrusive containers
   :  public list_base_hook<link_mode<LinkMode> >,  public test_class<BigSize>
{
   itest_class()                                {}
   itest_class(int i) : test_class<BigSize>(i)  {}
};

template<class FuncObj> //Adapts functors taking values to functors taking pointers
struct func_ptr_adaptor  :  public FuncObj
{
   typedef typename FuncObj::first_argument_type*  first_argument_type;
   typedef typename FuncObj::second_argument_type* second_argument_type;
   typedef typename FuncObj::result_type           result_type;
   result_type operator()(first_argument_type a,  second_argument_type b) const
      {  return FuncObj::operator()(*a, *b); }
};
//]

template <bool BigSize, link_mode_type LinkMode>
struct get_ilist  //Helps to define an intrusive list from a policy
{
   typedef list<itest_class<BigSize, LinkMode>, constant_time_size<false> > type;
};

template <bool BigSize> //Helps to define an std list
struct get_list      {  typedef std::list<test_class<BigSize> > type;   };

template <bool BigSize> //Helps to define an std pointer list
struct get_ptrlist   {  typedef std::list<test_class<BigSize>*> type;   };

////////////////////////////////////////////////////////////////////////
//
//                            PUSH_BACK
//
////////////////////////////////////////////////////////////////////////

template <bool BigSize, link_mode_type LinkMode>
void test_intrusive_list_push_back()
{
   typedef typename get_ilist<BigSize, LinkMode>::type ilist;
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      //First create the elements and insert them in the intrusive list
      //[perf_list_push_back_intrusive
      std::vector<typename ilist::value_type> objects(NumElements);
      ilist l;
      for(int i = 0; i < NumElements; ++i)
         l.push_back(objects[i]);
      //Elements are unlinked in ilist's destructor
      //Elements are destroyed in vector's destructor
      //]
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "link_mode: " << LinkMode << ", usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_std_list_push_back()
{
   typedef typename get_list<BigSize>::type stdlist;
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      //Now create the std list and insert
      //[perf_list_push_back_stdlist
      stdlist l;
      for(int i = 0; i < NumElements; ++i)
         l.push_back(typename stdlist::value_type(i));
      //Elements unlinked and destroyed in stdlist's destructor
      //]
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_compact_std_ptrlist_push_back()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;
   //Now measure insertion time, including element creation
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      //Create elements and insert their pointer in the list
      //[perf_list_push_back_stdptrlist
      std::vector<typename stdlist::value_type> objects(NumElements);
      stdptrlist l;
      for(int i = 0; i < NumElements; ++i)
         l.push_back(&objects[i]);
      //Pointers to elements unlinked and destroyed in stdptrlist's destructor
      //Elements destroyed in vector's destructor
      //]
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "compact std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_disperse_std_ptrlist_push_back()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;
   //Now measure insertion time, including element creation
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      //Create elements and insert their pointer in the list
      //[perf_list_push_back_disperse_stdptrlist
      stdlist objects;  stdptrlist l;
      for(int i = 0; i < NumElements; ++i){
         objects.push_back(typename stdlist::value_type(i));
         l.push_back(&objects.back());
      }
      //Pointers to elements unlinked and destroyed in stdptrlist's destructor
      //Elements unlinked and destroyed in stdlist's destructor
      //]
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "disperse std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

////////////////////////////////////////////////////////////////////////
//
//                            REVERSE
//
////////////////////////////////////////////////////////////////////////

//[perf_list_reverse
template <bool BigSize, link_mode_type LinkMode>
void test_intrusive_list_reverse()
{
   typedef typename get_ilist<BigSize, LinkMode>::type ilist;
   //First create the elements
   std::vector<typename ilist::value_type> objects(NumElements);

   //Now create the intrusive list and insert data
   ilist l(objects.begin(), objects.end());

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      l.reverse();
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "link_mode: " << LinkMode << ", usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_std_list_reverse()
{
   typedef typename get_list<BigSize>::type stdlist;

   //Create the list and insert values
   stdlist l;
   for(int i = 0; i < NumElements; ++i)
      l.push_back(typename stdlist::value_type());

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      l.reverse();
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_compact_std_ptrlist_reverse()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;

   //First create the elements
   std::vector<typename stdlist::value_type> objects(NumElements);

   //Now create the std list and insert
   stdptrlist l;
   for(int i = 0; i < NumElements; ++i)
      l.push_back(&objects[i]);

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      l.reverse();
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "compact std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_disperse_std_ptrlist_reverse()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;

   //First create the elements
   std::list<typename stdlist::value_type> objects;
   //Now create the std list and insert
   stdptrlist l;
   for(int i = 0; i < NumElements; ++i){
      objects.push_back(typename stdlist::value_type());
      l.push_back(&objects.back());
   }

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      l.reverse();
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "disperse std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}
//]

////////////////////////////////////////////////////////////////////////
//
//                            SORT
//
////////////////////////////////////////////////////////////////////////

//[perf_list_sort
template <bool BigSize, link_mode_type LinkMode>
void test_intrusive_list_sort()
{
   typedef typename get_ilist<BigSize, LinkMode>::type ilist;

   //First create the elements
   std::vector<typename ilist::value_type> objects(NumElements);
   for(int i = 0; i < NumElements; ++i)
      objects[i].i_ = i;

   //Now create the intrusive list and insert data
   ilist l(objects.begin(), objects.end());

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      if(!(i % 2)){
         l.sort(std::greater<typename ilist::value_type>());
      }
      else{
         l.sort(std::less<typename ilist::value_type>());
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "link_mode: " << LinkMode << ", usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_std_list_sort()
{
   typedef typename get_list<BigSize>::type stdlist;

   //Create the list and insert values
   stdlist l;
   for(int i = 0; i < NumElements; ++i)
      l.push_back(typename stdlist::value_type(i));

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      if(!(i % 2)){
         l.sort(std::greater<typename stdlist::value_type>());
      }
      else{
         l.sort(std::less<typename stdlist::value_type>());
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_compact_std_ptrlist_sort()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;

   //First create the elements
   std::vector<typename stdlist::value_type> objects(NumElements);
   for(int i = 0; i < NumElements; ++i)
      objects[i].i_ = i;
   //Now create the std list and insert
   stdptrlist l;
   for(int i = 0; i < NumElements; ++i)
      l.push_back(&objects[i]);

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      if(!(i % 2)){
         l.sort(func_ptr_adaptor<std::greater<typename stdlist::value_type> >());
      }
      else{
         l.sort(func_ptr_adaptor<std::less<typename stdlist::value_type> >());
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "compact std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_disperse_std_ptrlist_sort()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;

   //First create the elements and the list
   std::list<typename stdlist::value_type> objects;
   stdptrlist l;
   for(int i = 0; i < NumElements; ++i){
      objects.push_back(typename stdlist::value_type(i));
      l.push_back(&objects.back());
   }

   //Now measure sorting time
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      if(!(i % 2)){
         l.sort(func_ptr_adaptor<std::greater<typename stdlist::value_type> >());
      }
      else{
         l.sort(func_ptr_adaptor<std::less<typename stdlist::value_type> >());
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "disperse std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}
//]

////////////////////////////////////////////////////////////////////////
//
//                            WRITE ACCESS
//
////////////////////////////////////////////////////////////////////////
//[perf_list_write_access
template <bool BigSize, link_mode_type LinkMode>
void test_intrusive_list_write_access()
{
   typedef typename get_ilist<BigSize, LinkMode>::type ilist;

   //First create the elements
   std::vector<typename ilist::value_type> objects(NumElements);
   for(int i = 0; i < NumElements; ++i){
      objects[i].i_ = i;
   }

   //Now create the intrusive list and insert data
   ilist l(objects.begin(), objects.end());

   //Now measure access time to the value type
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      typename ilist::iterator it(l.begin()), end(l.end());
      for(; it != end; ++it){
         ++(it->i_);
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "link_mode: " << LinkMode << ", usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_std_list_write_access()
{
   typedef typename get_list<BigSize>::type stdlist;

   //Create the list and insert values
   stdlist l;
   for(int i = 0; i < NumElements; ++i)
      l.push_back(typename stdlist::value_type(i));

   //Now measure access time to the value type
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      typename stdlist::iterator it(l.begin()), end(l.end());
      for(; it != end; ++it){
         ++(it->i_);
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_compact_std_ptrlist_write_access()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;

   //First create the elements
   std::vector<typename stdlist::value_type> objects(NumElements);
   for(int i = 0; i < NumElements; ++i){
      objects[i].i_ = i;
   }

   //Now create the std list and insert
   stdptrlist l;
   for(int i = 0; i < NumElements; ++i)
      l.push_back(&objects[i]);

   //Now measure access time to the value type
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      typename stdptrlist::iterator it(l.begin()), end(l.end());
      for(; it != end; ++it){
         ++((*it)->i_);
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "compact std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}

template <bool BigSize>
void test_disperse_std_ptrlist_write_access()
{
   typedef typename get_list<BigSize>::type stdlist;
   typedef typename get_ptrlist<BigSize>::type stdptrlist;

   //First create the elements
   std::list<typename stdlist::value_type> objects;
   //Now create the std list and insert
   stdptrlist l;
   for(int i = 0; i < NumElements; ++i){
      objects.push_back(typename stdlist::value_type(i));
      l.push_back(&objects.back());
   }

   //Now measure access time to the value type
   ptime tini = microsec_clock::universal_time();
   for(int i = 0; i < NumIter; ++i){
      typename stdptrlist::iterator it(l.begin()), end(l.end());
      for(; it != end; ++it){
         ++((*it)->i_);
      }
   }
   ptime tend = microsec_clock::universal_time();
   std::cout << "disperse std::list usecs/iteration: "
             << (tend-tini).total_microseconds()/NumIter << std::endl;
}
//]

////////////////////////////////////////////////////////////////////////
//
//                            ALL TESTS
//
////////////////////////////////////////////////////////////////////////
template<bool BigSize>
void do_all_tests()
{
   std::cout << "\n\nTesting push back() with BigSize:" << BigSize << std::endl;
   test_intrusive_list_push_back<BigSize, normal_link>();
   test_intrusive_list_push_back<BigSize, safe_link>();
   test_intrusive_list_push_back<BigSize, auto_unlink>();
   test_std_list_push_back<BigSize> ();
   test_compact_std_ptrlist_push_back<BigSize>();
   test_disperse_std_ptrlist_push_back<BigSize>();
   //reverse
   std::cout << "\n\nTesting reverse() with BigSize:" << BigSize << std::endl;
   test_intrusive_list_reverse<BigSize, normal_link>();
   test_intrusive_list_reverse<BigSize, safe_link>();
   test_intrusive_list_reverse<BigSize, auto_unlink>();
   test_std_list_reverse<BigSize>();
   test_compact_std_ptrlist_reverse<BigSize>();
   test_disperse_std_ptrlist_reverse<BigSize>();
   //sort
   std::cout << "\n\nTesting sort() with BigSize:" << BigSize << std::endl;
   test_intrusive_list_sort<BigSize, normal_link>();
   test_intrusive_list_sort<BigSize, safe_link>();
   test_intrusive_list_sort<BigSize, auto_unlink>();
   test_std_list_sort<BigSize>();
   test_compact_std_ptrlist_sort<BigSize>();
   test_disperse_std_ptrlist_sort<BigSize>();
   //write_access
   std::cout << "\n\nTesting write_access() with BigSize:" << BigSize << std::endl;
   test_intrusive_list_write_access<BigSize, normal_link>();
   test_intrusive_list_write_access<BigSize, safe_link>();
   test_intrusive_list_write_access<BigSize, auto_unlink>();
   test_std_list_write_access<BigSize>();
   test_compact_std_ptrlist_write_access<BigSize>();
   test_disperse_std_ptrlist_write_access<BigSize>();
}

int main()
{
   //First pass the tests with a small size class
   do_all_tests<false>();
   //Now pass the tests with a big size class
   do_all_tests<true>();
   return 0;
}

#include <boost/intrusive/detail/config_end.hpp>

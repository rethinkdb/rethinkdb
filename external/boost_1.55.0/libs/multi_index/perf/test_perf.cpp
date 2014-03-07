/* Boost.MultiIndex performance test.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */

#include <algorithm>
#include <assert.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/next_prior.hpp>
#include <climits>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>

using namespace std;
using namespace boost::multi_index;

/* Measurement harness by Andrew Koenig, extracted from companion code to
 *   Stroustrup, B.: "Wrapping C++ Member Function Calls", The C++ Report,
 *     June 2000, Vol 12/No 6.
 * Original code retrievable at: http://www.research.att.com/~bs/wrap_code.cpp
 */

// How many clock units does it take to interrogate the clock?
static double clock_overhead()
{
    clock_t k = clock(), start, limit;

    // Wait for the clock to tick
    do start = clock();
    while (start == k);

    // interrogate the clock until it has advanced at least a second
    // (for reasonable accuracy)
    limit = start + CLOCKS_PER_SEC;

    unsigned long r = 0;
    while ((k = clock()) < limit)
        ++r;

    return double(k - start) / r;
}

// We'd like the odds to be factor:1 that the result is
// within percent% of the median
const int factor = 10;
const int percent = 20;

// Measure a function (object) factor*2 times,
// appending the measurements to the second argument
template<class F>
void measure_aux(F f, vector<double>& mv)
{
    static double ovhd = clock_overhead();

    // Ensure we don't reallocate in mid-measurement
    mv.reserve(mv.size() + factor*2);

    // Wait for the clock to tick
    clock_t k = clock();
    clock_t start;

    do start = clock();
    while (start == k);

    // Do 2*factor measurements
    for (int i = 2*factor; i; --i) {
        unsigned long count = 0, limit = 1, tcount = 0;

        // Original code used CLOCKS_PER_SEC/100
        const clock_t clocklimit = start + CLOCKS_PER_SEC/10;
        clock_t t;

        do {
            while (count < limit) {
                f();
                ++count;
            }
            limit *= 2;
            ++tcount;
        } while ((t = clock()) < clocklimit);

        // Wait for the clock to tick again;
        clock_t t2;
        do ++tcount;
        while ((t2 = clock()) == t);

        // Append the measurement to the vector
        mv.push_back(((t2 - start) - (tcount * ovhd)) / count);

        // Establish a new starting point
        start = t2;
    }
}

// Returns the number of clock units per iteration
// With odds of factor:1, the measurement is within percent% of
// the value returned, which is also the median of all measurements.
template<class F>
double measure(F f)
{
    vector<double> mv;

    int n = 0;                        // iteration counter
    do {
        ++n;

        // Try 2*factor measurements
        measure_aux(f, mv);
        assert(mv.size() == 2*n*factor);

        // Compute the median.  We know the size is even, so we cheat.
        sort(mv.begin(), mv.end());
        double median = (mv[n*factor] + mv[n*factor-1])/2;

        // If the extrema are within threshold of the median, we're done
        if (mv[n] > (median * (100-percent))/100 &&
            mv[mv.size() - n - 1] < (median * (100+percent))/100)
            return median;

    } while (mv.size() < factor * 200);

    // Give up!
    clog << "Help!\n\n";
    exit(1);
}

/* dereferencing compare predicate */

template <typename Iterator,typename Compare>
struct it_compare
{
  bool operator()(const Iterator& x,const Iterator& y)const{return comp(*x,*y);}

private:
  Compare comp;
};

/* list_wrapper and multiset_wrapper adapt std::lists and std::multisets
 * to make them conform to a set-like insert interface which test
 * routines do assume.
 */

template <typename List>
struct list_wrapper:List
{
  typedef typename List::value_type value_type;
  typedef typename List::iterator   iterator;

  pair<iterator,bool> insert(const value_type& v)
  {
    List::push_back(v);
    return pair<iterator,bool>(boost::prior(List::end()),true);
  }
};

template <typename Multiset>
struct multiset_wrapper:Multiset
{
  typedef typename Multiset::value_type value_type;
  typedef typename Multiset::iterator   iterator;

  pair<iterator,bool> insert(const value_type& v)
  {
    return pair<iterator,bool>(Multiset::insert(v),true);
  }
};

/* space comsumption of manual simulations is determined by checking
 * the node sizes of the containers involved. This cannot be done in a
 * portable manner, so node_size has to be written on a per stdlibrary
 * basis. Add your own versions if necessary.
 */

#if defined(BOOST_DINKUMWARE_STDLIB)

template<typename Container>
size_t node_size(const Container&)
{
  return sizeof(*Container().begin()._Mynode());
}

#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)

template<typename Container>
size_t node_size(const Container&)
{
  typedef typename Container::iterator::_Link_type node_ptr;
  node_ptr p=0;
  return sizeof(*p);
}

template<typename Value,typename Allocator>
size_t node_size(const list<Value,Allocator>&)
{
  return sizeof(typename list<Value,Allocator>::iterator::_Node);
}

template<typename List>
size_t node_size(const list_wrapper<List>&)
{
  return sizeof(typename List::iterator::_Node);
}

#else

/* default version returns 0 by convention */

template<typename Container>
size_t node_size(const Container&)
{
  return 0;
}

#endif

/* mono_container runs the tested routine on multi_index and manual
 * simulations comprised of one standard container.
 * bi_container and tri_container run the equivalent routine for manual
 * compositions of two and three standard containers, respectively.
 */

template <typename Container>
struct mono_container
{
  mono_container(int n_):n(n_){}

  void operator()()
  {
    typedef typename Container::iterator iterator;

    Container c;

    for(int i=0;i<n;++i)c.insert(i);
    for(iterator it=c.begin();it!=c.end();)c.erase(it++);
  }

  static size_t multi_index_node_size()
  {
    return sizeof(*Container().begin().get_node());
  }

  static size_t node_size()
  {
    return ::node_size(Container());
  }

private:
  int n;
};

template <typename Container1,typename Container2>
struct bi_container
{
  bi_container(int n_):n(n_){}

  void operator()()
  {
    typedef typename Container1::iterator iterator1;
    typedef typename Container2::iterator iterator2;

    Container1 c1;
    Container2 c2;

    for(int i=0;i<n;++i){
      iterator1 it1=c1.insert(i).first;
      c2.insert(it1);
    }
    for(iterator2 it2=c2.begin();it2!=c2.end();)
    {
      c1.erase(*it2);
      c2.erase(it2++);
    }
  }

  static size_t node_size()
  {
    return ::node_size(Container1())+::node_size(Container2());
  }

private:
  int n;
};

template <typename Container1,typename Container2,typename Container3>
struct tri_container
{
  tri_container(int n_):n(n_){}

  void operator()()
  {
    typedef typename Container1::iterator iterator1;
    typedef typename Container2::iterator iterator2;
    typedef typename Container3::iterator iterator3;

    Container1 c1;
    Container2 c2;
    Container3 c3;

    for(int i=0;i<n;++i){
      iterator1 it1=c1.insert(i).first;
      iterator2 it2=c2.insert(it1).first;
      c3.insert(it2);
    }
    for(iterator3 it3=c3.begin();it3!=c3.end();)
    {
      c1.erase(**it3);
      c2.erase(*it3);
      c3.erase(it3++);
    }
  }

  static size_t node_size()
  {
    return ::node_size(Container1())+
           ::node_size(Container2())+::node_size(Container3());
  }

private:
  int n;
};

/* measure and compare two routines for several numbers of elements
 * and also estimates relative memory consumption.
 */

template <typename IndexedTest,typename ManualTest>
void run_tests(
  const char* title
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(IndexedTest)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualTest))
{
  cout<<fixed<<setprecision(2);
  cout<<title<<endl;
  int n=1000;
  for(int i=0;i<3;++i){
    double indexed_t=measure(IndexedTest(n));
    double manual_t=measure(ManualTest(n));
    cout<<"  10^"<<i+3<<" elmts: "
        <<setw(6)<<100.0*indexed_t/manual_t<<"% "
        <<"("
          <<setw(6)<<1000.0*indexed_t/CLOCKS_PER_SEC<<" ms / "
          <<setw(6)<<1000.0*manual_t/CLOCKS_PER_SEC<<" ms)"
        <<endl;
    n*=10;
  }

  size_t indexed_t_node_size=IndexedTest::multi_index_node_size();
  size_t manual_t_node_size=ManualTest::node_size();

  if(manual_t_node_size){
    cout<<"  space gain: "
        <<setw(6)<<100.0*indexed_t_node_size/manual_t_node_size<<"%"<<endl;
  }
}

/* compare_structures accept a multi_index_container instantiation and
 * several standard containers, builds a manual simulation out of the
 * latter and run the tests.
 */

template <typename IndexedType,typename ManualType>
void compare_structures(
  const char* title
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(IndexedType)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualType))
{
  run_tests<
    mono_container<IndexedType>,
    mono_container<ManualType>
  >(title);
}

template <typename IndexedType,typename ManualType1,typename ManualType2>
void compare_structures2(
  const char* title
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(IndexedType)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualType1)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualType2))
{
  run_tests<
    mono_container<IndexedType>,
    bi_container<ManualType1,ManualType2>
  >(title);
}

template <
  typename IndexedType,
  typename ManualType1,typename ManualType2,typename ManualType3
>
void compare_structures3(
  const char* title
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(IndexedType)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualType1)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualType2)
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(ManualType3))
{
  run_tests<
    mono_container<IndexedType>,
    tri_container<ManualType1,ManualType2,ManualType3>
  >(title);
}

int main()
{
  {
    /* 1 ordered index */

    typedef multi_index_container<int> indexed_t;
    typedef set<int>                   manual_t;
    indexed_t dummy; /* MSVC++ 6.0 chokes if indexed_t is not instantiated */ 

    compare_structures<indexed_t,manual_t>(
      "1 ordered index");
  }
  {
    /* 1 sequenced index */

    typedef list_wrapper<
      multi_index_container<
        int,
        indexed_by<sequenced<> > 
      >
    >                                  indexed_t;
    typedef list_wrapper<list<int> >   manual_t;
    indexed_t dummy; /* MSVC++ 6.0 chokes if indexed_t is not instantiated */ 

    compare_structures<indexed_t,manual_t>(
      "1 sequenced index");
  }
  {
    /* 2 ordered indices */

    typedef multi_index_container<
      int,
      indexed_by<
        ordered_unique<identity<int> >,
        ordered_non_unique<identity<int> >
      >
    >                                  indexed_t;
    typedef set<int>                   manual_t1;
    typedef multiset<
      manual_t1::iterator,
      it_compare<
        manual_t1::iterator,
        manual_t1::key_compare
      >
    >                                  manual_t2;
    indexed_t dummy; /* MSVC++ 6.0 chokes if indexed_t is not instantiated */ 

    compare_structures2<indexed_t,manual_t1,manual_t2>(
      "2 ordered indices");
  }
  {
    /* 1 ordered index + 1 sequenced index */

    typedef multi_index_container<
      int,
      indexed_by<
        boost::multi_index::ordered_unique<identity<int> >,
        sequenced<>
      >
    >                                  indexed_t;
    typedef list_wrapper<
      list<int>
    >                                  manual_t1;
    typedef multiset<
      manual_t1::iterator,
      it_compare<
        manual_t1::iterator,
        std::less<int>
      >
    >                                  manual_t2;
    indexed_t dummy; /* MSVC++ 6.0 chokes if indexed_t is not instantiated */ 

    compare_structures2<indexed_t,manual_t1,manual_t2>(
      "1 ordered index + 1 sequenced index");
  }
  {
    /* 3 ordered indices */

    typedef multi_index_container<
      int,
      indexed_by<
        ordered_unique<identity<int> >,
        ordered_non_unique<identity<int> >,
        ordered_non_unique<identity<int> >
      >
    >                                  indexed_t;
    typedef set<int>                   manual_t1;
    typedef multiset_wrapper<
      multiset<
        manual_t1::iterator,
        it_compare<
          manual_t1::iterator,
          manual_t1::key_compare
        >
      >
    >                                  manual_t2;
    typedef multiset<
      manual_t2::iterator,
      it_compare<
        manual_t2::iterator,
        manual_t2::key_compare
      >
    >                                  manual_t3;
    indexed_t dummy; /* MSVC++ 6.0 chokes if indexed_t is not instantiated */ 

    compare_structures3<indexed_t,manual_t1,manual_t2,manual_t3>(
      "3 ordered indices");
  }
  {
    /* 2 ordered indices + 1 sequenced index */

    typedef multi_index_container<
      int,
      indexed_by<
        ordered_unique<identity<int> >,
        ordered_non_unique<identity<int> >,
        sequenced<>
      >
    >                                  indexed_t;
    typedef list_wrapper<
      list<int>
    >                                  manual_t1;
    typedef multiset_wrapper<
      multiset<
        manual_t1::iterator,
        it_compare<
          manual_t1::iterator,
          std::less<int>
        >
      >
    >                                  manual_t2;
    typedef multiset<
      manual_t2::iterator,
      it_compare<
        manual_t2::iterator,
        manual_t2::key_compare
      >
    >                                  manual_t3;
    indexed_t dummy; /* MSVC++ 6.0 chokes if indexed_t is not instantiated */ 

    compare_structures3<indexed_t,manual_t1,manual_t2,manual_t3>(
      "2 ordered indices + 1 sequenced index");
  }

  return 0;
}

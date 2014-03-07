/* Boost.Flyweight example of performance comparison.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/hashed_factory.hpp>
#include <boost/flyweight/set_factory.hpp>
#include <boost/flyweight/static_holder.hpp>
#include <boost/flyweight/simple_locking.hpp>
#include <boost/flyweight/refcounted.hpp>
#include <boost/flyweight/no_tracking.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/tokenizer.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
using ::clock;
using ::clock_t;
using ::exit;
}
#endif

using namespace boost::flyweights;

/* Instrumented allocator family keeping track of the memory in
 * current use.
 */

std::size_t count_allocator_mem=0;

template<typename T>
class count_allocator
{
public:
  typedef std::size_t     size_type;
  typedef std::ptrdiff_t  difference_type;
  typedef T*              pointer;
  typedef const T*        const_pointer;
  typedef T&              reference;
  typedef const T&        const_reference;
  typedef T               value_type;
  template<class U>struct rebind{typedef count_allocator<U> other;};

  count_allocator(){}
  count_allocator(const count_allocator<T>&){}
  template<class U>count_allocator(const count_allocator<U>&,int=0){}

  pointer       address(reference x)const{return &x;}
  const_pointer address(const_reference x)const{return &x;}

  pointer allocate(size_type n,const void* =0)
  {
    pointer p=(T*)(new char[n*sizeof(T)]);
    count_allocator_mem+=n*sizeof(T);
    return p;
  }

  void deallocate(void* p,size_type n)
  {
    count_allocator_mem-=n*sizeof(T);
    delete [](char *)p;
  }

  size_type max_size() const{return (size_type )(-1);}
  void      construct(pointer p,const T& val){new(p)T(val);}
  void      destroy(pointer p){p->~T();}

  friend bool operator==(const count_allocator&,const count_allocator&)
  {
    return true;
  }

  friend bool operator!=(const count_allocator&,const count_allocator&)
  {
    return false;
  }
};

template<>
class count_allocator<void>
{
public:
  typedef void*           pointer;
  typedef const void*     const_pointer;
  typedef void            value_type;
  template<class U>struct rebind{typedef count_allocator<U> other;};
};

/* Define some count_allocator-based types and Boost.Flyweight components */

typedef std::basic_string<
  char,std::char_traits<char>,count_allocator<char>
> count_string;

typedef hashed_factory<
  boost::hash<boost::mpl::_2>,
  std::equal_to<boost::mpl::_2>,
  count_allocator<boost::mpl::_1>
> count_hashed_factory;

typedef set_factory<
  std::less<boost::mpl::_2>,
  count_allocator<boost::mpl::_1>
> count_set_factory;

/* Some additional utilities used by the test routine */

class timer
{
public:
  timer(){restart();}

  void restart(){t=std::clock();}

  void time(const char* str)
  {
    std::cout<<str<<": "<<(double)(std::clock()-t)/CLOCKS_PER_SEC<<" s\n";
  }

private:
  std::clock_t t;
};

template<typename T>
struct is_flyweight:
  boost::mpl::false_{};

template<
  typename T,
  typename Arg1,typename Arg2,typename Arg3,typename Arg4,typename Arg5
>
struct is_flyweight<flyweight<T,Arg1,Arg2,Arg3,Arg4,Arg5> >:
  boost::mpl::true_{};

struct length_adder
{
  std::size_t operator()(std::size_t n,const count_string& x)const
  {
    return n+x.size();
  }
};

/* Measure time and memory performance for a String, which is assumed
 * to be either a plain string type or a string flyweight.
 */

template<typename String>
struct test
{
  static std::size_t run(const std::string& file)
  {
    typedef std::vector<String,count_allocator<String> > count_vector;

    /* Define a tokenizer on std::istreambuf. */
  
    typedef std::istreambuf_iterator<char> char_iterator;
    typedef boost::tokenizer<
      boost::char_separator<char>,
      char_iterator
    >                                      tokenizer;

    std::ifstream ifs(file.c_str());
    if(!ifs){
      std::cout<<"can't open "<<file<<std::endl;
      std::exit(EXIT_FAILURE);
    }

    /* Initialization; tokenize using space and common punctuaction as
     * separators, and keeping the separators.
     */

    timer t;

    tokenizer tok=tokenizer(
      char_iterator(ifs),char_iterator(),
      boost::char_separator<char>(
        "",
        "\t\n\r !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"));
    count_vector txt;
    for(tokenizer::iterator it=tok.begin();it!=tok.end();++it){
      txt.push_back(String(it->c_str()));
    }

    t.time("initialization time");

    /* Assignment */

    t.restart();

    count_vector txt2;
    for(int i=0;i<10;++i){
      txt2.insert(txt2.end(),txt.begin(),txt.end());
    }

    t.time("assignment time");

    /* Equality comparison */

    t.restart();

    std::size_t c=0;
    for(int i=0;i<100;++i){
      c+=std::count(txt.begin(),txt.end(),txt[c%txt.size()]);
    }

    t.time("equality comparison time");

    /* Value access */

    t.restart();

    std::size_t s=0;
    for(int i=0;i<20;++i){
      s+=std::accumulate(txt2.begin(),txt2.end(),s,length_adder());
    }

    t.time("value access time");

    std::cout<<"bytes used: "<<count_allocator_mem;
    if(is_flyweight<String>::value){
      std::size_t flyweight_mem=(txt.capacity()+txt2.capacity())*sizeof(String);
      std::cout<<"= flyweights("<<flyweight_mem
               <<")+values("<<count_allocator_mem-flyweight_mem<<")";
    }
    std::cout<<"\n";

    return c+s;
  }
};

/* table of test cases for the user to select from */

struct test_case
{
  const char* name;
  std::size_t (*test)(const std::string&);
};

test_case test_table[]=
{
  {
    "simple string",
    test<count_string>::run
  },
  {
    "flyweight, hashed factory",
    test<flyweight<count_string,count_hashed_factory> >::run
  },
  {
    "flyweight, hashed factory, no tracking",
    test<flyweight<count_string,count_hashed_factory,no_tracking> >::run
  },
  {
    "flyweight, set-based factory",
    test<flyweight<count_string,count_set_factory> >::run
  },
  {
    "flyweight, set-based factory, no tracking",
    test<flyweight<count_string,count_set_factory,no_tracking> >::run
  }
};

const int num_test_cases=sizeof(test_table)/sizeof(test_case);

int main()
{
  try{
    for(int i=0;i<num_test_cases;++i){
      std::cout<<i+1<<". "<<test_table[i].name<<"\n";
    }
    int option=-1;
    for(;;){
      std::cout<<"select option, enter to exit: ";
      std::string str;
      std::getline(std::cin,str);
      if(str.empty())std::exit(EXIT_SUCCESS);
      std::istringstream istr(str);
      istr>>option;
      if(option>=1&&option<=num_test_cases){
        --option; /* pass from 1-based menu to 0-based test_table */
        break;
      }
    }

    std::cout<<"enter file name: ";
    std::string file;
    std::getline(std::cin,file);
    std::size_t result=0;
    result=test_table[option].test(file);
  }
  catch(const std::exception& e){
    std::cout<<"error: "<<e.what()<<"\n";
  }

  return 0;
}

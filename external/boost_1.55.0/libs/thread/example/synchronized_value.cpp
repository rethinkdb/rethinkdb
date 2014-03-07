// (C) Copyright 2010 Just Software Solutions Ltd http://www.justsoftwaresolutions.co.uk
// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4

#include <iostream>
#include <string>
#include <boost/thread/synchronized_value.hpp>

  void addTrailingSlashIfMissing(boost::synchronized_value<std::string> & path)
  {
    boost::strict_lock_ptr<std::string> u=path.synchronize();

    if(u->empty() || (*u->rbegin()!='/'))
    {
      *u+='/';
    }
  }

void f(const boost::synchronized_value<int> &v) {
  std::cout<<"v="<<*v<<std::endl;
}

void g(const boost::const_strict_lock_ptr<int> &v) {
  std::cout<<"v="<<*v<<std::endl;
}

bool checkIfMissingTrailingSlash(boost::synchronized_value<std::string> & path)
{
  boost::strict_lock_ptr<std::string> u=path.synchronize();

  return (u->empty() || (*u->rbegin()!='/'));
}

int main()
{
  {
    boost::synchronized_value<int> v1;
    *v1=42;
    std::cout<<"v1="<<*v1<<std::endl;
    f(v1);
    int i=*v1;
    std::cout<<"i="<<i<<std::endl;

    {
      boost::strict_lock_ptr<int> u=v1.synchronize();

      *u+=43;
      std::cout<<"v1="<<*u<<std::endl;
      g(u);
    }
    boost::synchronized_value<int> v2(2);
    std::cout<<"v2="<<*v2<<std::endl;
    v2 = 3;
    std::cout<<"v2="<<*v2<<std::endl;

    boost::synchronized_value<int> v3(v2);
    std::cout<<"v3="<<*v3<<std::endl;
    v3 = v1;
    std::cout<<"v3="<<*v3<<std::endl;

    std::cout<<"v2="<<*v3<<std::endl;
    std::cout<<"v3="<<*v3<<std::endl;
    swap(v3,v2);
    v1.swap(v2);
    std::cout<<"v3="<<*v3<<std::endl;
  }
  {
    boost::synchronized_value<std::string> s;
    addTrailingSlashIfMissing(s);
    std::cout<<"s="<<std::string(*s)<<std::endl;
  }
  {
    boost::synchronized_value<std::string> s;
    s->append("foo/");
    s.synchronize()->append("foo");
    addTrailingSlashIfMissing(s);
    std::cout<<"s="<<std::string(*s)<<std::endl;
  }
  {
    boost::synchronized_value<std::string> s;
    s = std::string("foo/");
    std::cout<<"ss="<< s << std::endl;
  }
  {
    boost::synchronized_value<std::string> s;
    s = "foo/";
    std::cout<<"ss="<< s << std::endl;
  }
  {
    boost::synchronized_value<std::string> s1("a");
    boost::synchronized_value<std::string> s2;
    s2=s1;
    std::cout<<"s1="<< s1 << std::endl;
    std::cout<<"s2="<< s2 << std::endl;
  }
  {
    boost::synchronized_value<std::string> s1("a");
    boost::synchronized_value<std::string> s2("b");
    std::cout<<"s1="<< s1 << std::endl;
    std::cout<<"s2="<< s2 << std::endl;
    swap(s1,s2);
    std::cout<<"s1="<< s1 << std::endl;
    std::cout<<"s2="<< s2 << std::endl;
  }
#if ! defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)
  {
    boost::synchronized_value<std::string> sts("a");
    std::string s(sts);
    std::cout<<"ssts="<< s << std::endl;
  }
#endif
  {
    boost::synchronized_value<int> s1(1);
    boost::synchronized_value<int> s2(1);
    BOOST_ASSERT(s1==s2);
    BOOST_ASSERT(s1<=s2);
    BOOST_ASSERT(s1>=s2);
    BOOST_ASSERT(s1==1);
    BOOST_ASSERT(s1<=1);
    BOOST_ASSERT(s1>=1);
  }
  {
    boost::synchronized_value<int> s1(1);
    boost::synchronized_value<int> s2(2);
    BOOST_ASSERT(s1!=s2);
    BOOST_ASSERT(s1!=2);
    BOOST_ASSERT(2!=s1);
  }
  {
    boost::synchronized_value<int> s1(1);
    boost::synchronized_value<int> s2(2);
    BOOST_ASSERT(s1<s2);
    BOOST_ASSERT(s1<=s2);
    BOOST_ASSERT(s2>s1);
    BOOST_ASSERT(s2>=s1);
    BOOST_ASSERT(s1<2);
    BOOST_ASSERT(s1<=2);
    BOOST_ASSERT(s2>1);
    BOOST_ASSERT(s2>=1);
  }
  return 0;
}


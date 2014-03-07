// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4

#include <iostream>
#include <string>
#include <boost/thread/synchronized_value.hpp>

//class SafePerson {
//public:
//  std::string GetName() const {
//    const_unique_access<std::string> name(nameGuard);
//    return *name;
//  }
//  void SetName(const std::string& newName)  {
//    unique_access<std::string> name(nameGuard);
//    *name = newName;
//  }
//private:
//  unique_access_guard<std::string> nameGuard;
//};

class SafePerson {
public:
  std::string GetName() const {
    return *name;
  }
  void SetName(const std::string& newName)  {
    *name = newName;
  }

private:
  boost::synchronized_value<std::string> name;
};

class Person {
public:
  std::string GetName() const {
    return name;
  }
  void SetName(const std::string& newName) {
    name = newName;
  }
private:
  std::string name;
};
typedef boost::synchronized_value<Person> Person_ts;


//class SafeMemberPerson {
//public:
//  SafeMemberPerson(unsigned int age) :
//    memberGuard(age)
//  {  }
//  std::string GetName() const  {
//    const_unique_access<Member> member(memberGuard);
//    return member->name;
//  }
//  void SetName(const std::string& newName)  {
//    unique_access<Member> member(memberGuard);
//    member->name = newName;
//  }
//private:
//  struct Member
//  {
//    Member(unsigned int age) :
//      age(age)
//    {    }
//    std::string name;
//    unsigned int age;
//  };
//  unique_access_guard<Member> memberGuard;
//};

class SafeMemberPerson {
public:
  SafeMemberPerson(unsigned int age) :
    member(Member(age))
  {  }
  std::string GetName() const  {
    return member->name;
  }
  void SetName(const std::string& newName)  {
    member->name = newName;
  }
private:
  struct Member  {
    Member(unsigned int age) :
      age(age)
    {    }
    std::string name;
    unsigned int age;
  };
  boost::synchronized_value<Member> member;
};


class Person2 {
public:
  Person2(unsigned int age) : age_(age)
  {}
  std::string GetName() const  {
    return name_;
  }
  void SetName(const std::string& newName)  {
    name_ = newName;
  }
  unsigned int GetAge() const  {
    return age_;
  }

private:
  std::string name_;
  unsigned int age_;
};
typedef boost::synchronized_value<Person2> Person2_ts;

//===================

//class HelperPerson {
//public:
//  HelperPerson(unsigned int age) :
//    memberGuard(age)
//  {  }
//  std::string GetName() const  {
//    const_unique_access<Member> member(memberGuard);
//    Invariant(member);
//    return member->name;
//  }
//  void SetName(const std::string& newName)  {
//    unique_access<Member> member(memberGuard);
//    Invariant(member);
//    member->name = newName;
//  }
//private:
//  void Invariant(const_unique_access<Member>& member) const  {
//    if (member->age < 0) throw std::runtime_error("Age cannot be negative");
//  }
//  struct Member  {
//    Member(unsigned int age) :
//      age(age)
//    {    }
//    std::string name;
//    unsigned int age;
//  };
//  unique_access_guard<Member> memberGuard;
//};

class HelperPerson {
public:
  HelperPerson(unsigned int age) :
    member(age)
  {  }
  std::string GetName() const  {
#if ! defined BOOST_NO_CXX11_AUTO_DECLARATIONS
        auto memberSync = member.synchronize();
#else
    boost::const_strict_lock_ptr<Member>  memberSync = member.synchronize();
#endif
    Invariant(memberSync);
    return memberSync->name;
  }
  void SetName(const std::string& newName)  {
#if ! defined BOOST_NO_CXX11_AUTO_DECLARATIONS
    auto memberSync = member.synchronize();
#else
    boost::strict_lock_ptr<Member> memberSync = member.synchronize();
#endif
    Invariant(memberSync);
    memberSync->name = newName;
  }
private:
  struct Member  {
    Member(unsigned int age) :
      age(age)
    {    }
    std::string name;
    unsigned int age;
  };
  void Invariant(boost::const_strict_lock_ptr<Member> & mbr) const
  {
    if (mbr->age < 1) throw std::runtime_error("Age cannot be negative");
  }
  boost::synchronized_value<Member> member;
};

class Person3 {
public:
  Person3(unsigned int age) :
    age_(age)
  {  }
  std::string GetName() const  {
    Invariant();
    return name_;
  }
  void SetName(const std::string& newName)  {
    Invariant();
    name_ = newName;
  }
private:
  std::string name_;
  unsigned int age_;
  void Invariant() const  {
    if (age_ < 1) throw std::runtime_error("Age cannot be negative");
  }
};

typedef boost::synchronized_value<Person3> Person3_ts;

int main()
{
  {
    SafePerson p;
    p.SetName("Vicente");
  }
  {
    Person_ts p;
    p->SetName("Vicente");
  }
  {
    SafeMemberPerson p(1);
    p.SetName("Vicente");
  }
  {
    Person2_ts p(1);
    p->SetName("Vicente");
  }
  {
    HelperPerson p(1);
    p.SetName("Vicente");
  }
  {
    Person3_ts p(1);
    p->SetName("Vicente");
  }
  {
    Person3_ts p1(1);
    Person3_ts p2(2);
    Person3_ts p3(3);
#if ! defined BOOST_NO_CXX11_AUTO_DECLARATIONS
    auto  lk1 = p1.unique_synchronize(boost::defer_lock);
    auto  lk2 = p2.unique_synchronize(boost::defer_lock);
    auto  lk3 = p3.unique_synchronize(boost::defer_lock);
#else
    boost::unique_lock_ptr<Person3>  lk1 = p1.unique_synchronize(boost::defer_lock);
    boost::unique_lock_ptr<Person3>  lk2 = p2.unique_synchronize(boost::defer_lock);
    boost::unique_lock_ptr<Person3>  lk3 = p3.unique_synchronize(boost::defer_lock);
#endif
    boost::lock(lk1,lk2,lk3);

    lk1->SetName("Carmen");
    lk2->SetName("Javier");
    lk3->SetName("Matias");
  }
#if ! defined BOOST_NO_CXX11_AUTO_DECLARATIONS \
&& ! defined(BOOST_THREAD_NO_SYNCHRONIZE)
  {
    Person3_ts p1(1);
    Person3_ts p2(2);
    Person3_ts p3(3);

    auto t  = boost::synchronize(p1,p2,p3);
    std::get<0>(t)->SetName("Carmen");
    std::get<1>(t)->SetName("Javier");
    std::get<2>(t)->SetName("Matias");
  }
  {
    const Person3_ts p1(1);
    Person3_ts p2(2);
    const Person3_ts p3(3);

    auto t  = boost::synchronize(p1,p2,p3);
    //std::get<0>(t)->SetName("Carmen");
    std::get<1>(t)->SetName("Javier");
    //std::get<2>(t)->SetName("Matias");
  }
#endif
  return 0;
}

/* Boost.Flyweight basic example.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include <boost/bind.hpp>
#include <boost/flyweight.hpp>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

using namespace boost::flyweights;

/* Information associated to a given user of some massive system.
 * first_name and last_name are turned into flyweights to leverage the
 * implicit redundancy of names within the user community.
 */

struct user_entry
{
  flyweight<std::string> first_name;
  flyweight<std::string> last_name;
  int                    age;

  user_entry();
  user_entry(const char* first_name,const char* last_name,int age);
  user_entry(const user_entry& x);
};

/* flyweight<std::string> default ctor simply calls the default ctor of
 * std::string.
 */

user_entry::user_entry()
{}

/* flyweight<std::string> is constructible from a const char* much as
 * a std::string is.
 */

user_entry::user_entry(const char* f,const char* l,int a):
  first_name(f),
  last_name(l),
  age(a)
{}

/* flyweight's are copyable and assignable --unlike std::string,
 * copy and assignment of flyweight<std::string>s do not ever throw.
 */

user_entry::user_entry(const user_entry& x):
  first_name(x.first_name),
  last_name(x.last_name),
  age(x.age)
{}

/* flyweight<std::string> has operator==,!=,<,>,<=,>= with the same
 * semantics as those of std::string.
 */

bool same_name(const user_entry& user1,const user_entry& user2)
{
  bool b=user1.first_name==user2.first_name &&
         user1.last_name==user2.last_name;
  return b;
}

/* operator<< forwards to the std::string overload */

std::ostream& operator<<(std::ostream& os,const user_entry& user)
{
  return os<<user.first_name<<" "<<user.last_name<<" "<<user.age;
}

/* operator>> internally uses std::string's operator>> */

std::istream& operator>>(std::istream& is,user_entry& user)
{
  return is>>user.first_name>>user.last_name>>user.age;
}

std::string full_name(const user_entry& user)
{
  std::string full;

  /* get() returns the underlying const std::string& */

  full.reserve(
    user.first_name.get().size()+user.last_name.get().size()+1);

  /* here, on the other hand, implicit conversion is used */

  full+=user.first_name;
  full+=" ";
  full+=user.last_name;

  return full;
}

/* flyweight<std::string> value is immutable, but a flyweight object can
 * be assigned a different value.
 */

void change_name(user_entry& user,const std::string& f,const std::string& l)
{
  user.first_name=f;
  user.last_name=l;
}

int main()
{
  /* play a little with a vector of user_entry's */

  std::string users_txt=
  "olegh smith 31\n"
  "john brown 28\n"
  "anna jones 45\n"
  "maria garcia 30\n"
  "john fox 56\n"
  "anna brown 19\n"
  "thomas smith 46\n"
  "andrew martin 28";

  std::vector<user_entry> users;
  std::istringstream iss(users_txt);
  while(iss){
    user_entry u;
    if(iss>>u)users.push_back(u);
  }

  change_name(users[0],"oleg","smith");

  user_entry anna("anna","jones",20);
  std::replace_if(
    users.begin(),users.end(),
    boost::bind(same_name,_1,anna),
    anna);

  std::copy(
    users.begin(),users.end(),
    std::ostream_iterator<user_entry>(std::cout,"\n"));

  return 0;
}

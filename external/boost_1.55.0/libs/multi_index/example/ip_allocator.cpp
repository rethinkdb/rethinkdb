/* Boost.MultiIndex example of use of Boost.Interprocess allocators.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <algorithm>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

using boost::multi_index_container;
using namespace boost::multi_index;
namespace bip=boost::interprocess;

/* shared_string is a string type placeable in shared memory,
 * courtesy of Boost.Interprocess.
 */

typedef bip::basic_string<
  char,std::char_traits<char>,
  bip::allocator<char,bip::managed_mapped_file::segment_manager>
> shared_string;

/* Book record. All its members can be placed in shared memory,
 * hence the structure itself can too.
 */

struct book
{
  shared_string name;
  shared_string author;
  unsigned      pages;
  unsigned      prize;

  book(const shared_string::allocator_type& al):
    name(al),author(al),pages(0),prize(0)
  {}

  friend std::ostream& operator<<(std::ostream& os,const book& b)
  {
    os<<b.author<<": \""<<b.name<<"\", $"<<b.prize<<", "<<b.pages<<" pages\n";
    return os;
  }
};

/* partial_str_less allows for partial searches taking into account
 * only the first n chars of the strings compared against. See
 * Tutorial: Basics: Special lookup operations for more info on this
 * type of comparison functors.
 */

/* partial_string is a mere string holder used to differentiate from
 * a plain string.
 */

struct partial_string 
{
  partial_string(const shared_string& str):str(str){}
  shared_string str;
};

struct partial_str_less
{
  bool operator()(const shared_string& x,const shared_string& y)const
  {
    return x<y;
  }

  bool operator()(const shared_string& x,const partial_string& y)const
  {
    return x.substr(0,y.str.size())<y.str;
  }

  bool operator()(const partial_string& x,const shared_string& y)const
  {
    return x.str<y.substr(0,x.str.size());
  }
};

/* Define a multi_index_container of book records with indices on
 * author, name and prize. The index on names allows for partial
 * searches. This container can be placed in shared memory because:
 *   * book can be placed in shared memory.
 *   * We are using a Boost.Interprocess specific allocator.
 */

/* see Compiler specifics: Use of member_offset for info on
 * BOOST_MULTI_INDEX_MEMBER
 */

typedef multi_index_container<
  book,
  indexed_by<
    ordered_non_unique<
      BOOST_MULTI_INDEX_MEMBER(book,shared_string,author)
    >,
    ordered_non_unique<
      BOOST_MULTI_INDEX_MEMBER(book,shared_string,name),
      partial_str_less
    >,
    ordered_non_unique<
      BOOST_MULTI_INDEX_MEMBER(book,unsigned,prize)
    >
  >,
  bip::allocator<book,bip::managed_mapped_file::segment_manager>
> book_container;

/* A small utility to get data entered via std::cin */

template<typename T>
void enter(const char* msg,T& t)
{
  std::cout<<msg;
  std::string str;
  std::getline(std::cin,str);
  std::istringstream iss(str);
  iss>>t;
}

void enter(const char* msg,std::string& str)
{
  std::cout<<msg;
  std::getline(std::cin,str);
}

void enter(const char* msg,shared_string& str)
{
  std::cout<<msg;
  std::string stdstr;
  std::getline(std::cin,stdstr);
  str=stdstr.c_str();
}

int main()
{
  /* Create (or open) the memory mapped file where the book container
   * is stored, along with a mutex for synchronized access.
   */

  bip::managed_mapped_file seg(
    bip::open_or_create,"./book_container.db",
    65536);
  bip::named_mutex mutex(
    bip::open_or_create,"7FD6D7E8-320B-11DC-82CF-F0B655D89593");

  /* create or open the book container in shared memory */

  book_container* pbc=seg.find_or_construct<book_container>("book container")(
    book_container::ctor_args_list(),
    book_container::allocator_type(seg.get_segment_manager()));

  std::string command_info=
    "1. list books by author\n"
    "2. list all books by prize\n"
    "3. insert a book\n"
    "4. delete a book\n"
    "0. exit\n";

  std::cout<<command_info;

  /* main loop */

  for(bool exit=false;!exit;){
    int command=-1;
    enter("command: ",command);

    switch(command){
      case 0:{ /* exit */
        exit=true; 
        break;
      }
      case 1:{ /* list books by author */
        std::string author;
        enter("author (empty=all authors): ",author);

        /* operations with the container must be mutex protected */

        bip::scoped_lock<bip::named_mutex> lock(mutex);

        std::pair<book_container::iterator,book_container::iterator> rng;
        if(author.empty()){
          rng=std::make_pair(pbc->begin(),pbc->end());
        }
        else{
          rng=pbc->equal_range(
            shared_string(
              author.c_str(),
              shared_string::allocator_type(seg.get_segment_manager())));
        }

        if(rng.first==rng.second){
          std::cout<<"no entries\n";
        }
        else{
          std::copy(
            rng.first,rng.second,std::ostream_iterator<book>(std::cout));
        }
        break;
      }
      case 2:{ /* list all books by prize */
        bip::scoped_lock<bip::named_mutex> lock(mutex);

        std::copy(
          get<2>(*pbc).begin(),get<2>(*pbc).end(),
          std::ostream_iterator<book>(std::cout));
        break;
      }
      case 3:{ /* insert a book */
        book b(shared_string::allocator_type(seg.get_segment_manager()));

        enter("author: ",b.author);
        enter("name: "  ,b.name);
        enter("prize: " ,b.prize);
        enter("pages: " ,b.pages);

        std::cout<<"insert the following?\n"<<b<<"(y/n): ";
        char yn='n';
        enter("",yn);
        if(yn=='y'||yn=='Y'){
          bip::scoped_lock<bip::named_mutex> lock(mutex);
          pbc->insert(b);
        }

        break;
      }
      case 4:{ /* delete a book */
        shared_string name(
          shared_string::allocator_type(seg.get_segment_manager()));
        enter(
          "name of the book (you can enter\nonly the first few characters): ",
          name);

        typedef nth_index<book_container,1>::type index_by_name;
        index_by_name&          idx=get<1>(*pbc);
        index_by_name::iterator it;
        book b(shared_string::allocator_type(seg.get_segment_manager()));

        {
          /* Look for a book whose title begins with name. Note that we
           * are unlocking after doing the search so as to not leave the
           * container blocked during user prompting. That is also why a
           * local copy of the book is done.
           */

          bip::scoped_lock<bip::named_mutex> lock(mutex);

          it=idx.find(partial_string(name));
          if(it==idx.end()){
            std::cout<<"no such book found\n";
            break;
          }
          b=*it;
        }

        std::cout<<"delete the following?\n"<<b<<"(y/n): ";
        char yn='n';
        enter("",yn);
        if(yn=='y'||yn=='Y'){
          bip::scoped_lock<bip::named_mutex> lock(mutex);
          idx.erase(it);
        }

        break;
      }
      default:{
        std::cout<<"select one option:\n"<<command_info;
        break;
      }
    }
  }

  return 0;
}

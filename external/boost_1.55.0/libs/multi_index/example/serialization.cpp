/* Boost.MultiIndex example of serialization of a MRU list.
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
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

using namespace boost::multi_index;

/* An MRU (most recently used) list keeps record of the last n
 * inserted items, listing first the newer ones. Care has to be
 * taken when a duplicate item is inserted: instead of letting it
 * appear twice, the MRU list relocates it to the first position.
 */

template <typename Item>
class mru_list
{
  typedef multi_index_container<
    Item,
    indexed_by<
      sequenced<>,
      hashed_unique<identity<Item> >
    >
  > item_list;

public:
  typedef Item                         item_type;
  typedef typename item_list::iterator iterator;

  mru_list(std::size_t max_num_items_):max_num_items(max_num_items_){}

  void insert(const item_type& item)
  {
    std::pair<iterator,bool> p=il.push_front(item);

    if(!p.second){                     /* duplicate item */
      il.relocate(il.begin(),p.first); /* put in front */
    }
    else if(il.size()>max_num_items){  /* keep the length <= max_num_items */
      il.pop_back();
    }
  }

  iterator begin(){return il.begin();}
  iterator end(){return il.end();}

  /* Utilities to save and load the MRU list, internally
   * based on Boost.Serialization.
   */

  void save_to_file(const char* file_name)const
  {
    std::ofstream ofs(file_name);
    boost::archive::text_oarchive oa(ofs);
    oa<<boost::serialization::make_nvp("mru",*this);
  }

  void load_from_file(const char* file_name)
  {
    std::ifstream ifs(file_name);
    if(ifs){
      boost::archive::text_iarchive ia(ifs);
      ia>>boost::serialization::make_nvp("mru",*this);
    }
  }

private:
  item_list   il;
  std::size_t max_num_items;

  /* serialization support */

  friend class boost::serialization::access;
    
  template<class Archive>
  void serialize(Archive& ar,const unsigned int)
  {
    ar&BOOST_SERIALIZATION_NVP(il);
    ar&BOOST_SERIALIZATION_NVP(max_num_items);
  }
};

int main()
{
  const char* mru_store="mru_store";

  /* Construct a MRU limited to 10 items and retrieve its
   * previous contents.
   */

  mru_list<std::string> mru(10);
  mru.load_from_file(mru_store);

  /* main loop */

  for(;;){
    std::cout<<"enter a term: ";

    std::string line;
    std::getline(std::cin,line);
    if(line.empty())break;

    std::string term;
    std::istringstream iss(line);
    iss>>term;
    if(term.empty())break;

    mru.insert(term);

    std::cout<<"most recently entered terms:"<<std::endl;
    std::copy(
      mru.begin(),mru.end(),
      std::ostream_iterator<std::string>(std::cout,"\n"));
  }

  /* persist the MRU list */

  mru.save_to_file(mru_store);

  return 0;
}

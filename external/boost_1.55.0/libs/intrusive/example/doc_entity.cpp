/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
//[doc_entity_code
#include <boost/intrusive/list.hpp>

using namespace boost::intrusive;

//A class that can be inserted in an intrusive list
class entity : public list_base_hook<>
{
   public:
   virtual ~entity();
   //...
};

//"some_entity" derives from "entity"
class some_entity  :  public entity
{/**/};

//Definition of the intrusive list
typedef list<entity> entity_list;

//A global list
entity_list global_list;

//The destructor removes itself from the global list
entity::~entity()
{  global_list.erase(entity_list::s_iterator_to(*this));  }

//Function to insert a new "some_entity" in the global list
void insert_some_entity()
{  global_list.push_back (*new some_entity(/*...*/));  }

//Function to clear an entity from the intrusive global list
void clear_list ()
{
   // entity's destructor removes itself from the global list implicitly
   while (!global_list.empty())
      delete &global_list.front();
}

int main()
{
   //Insert some new entities
   insert_some_entity();
   insert_some_entity();
   //global_list's destructor will free objects
   return 0;
}

//]

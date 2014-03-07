//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/container/detail/config_begin.hpp>
#include <boost/container/detail/workaround.hpp>
//[doc_type_erasure_MyClassHolder_h
#include <boost/container/vector.hpp>

//MyClassHolder.h

//We don't need to include "MyClass.h"
//to store vector<MyClass>
class MyClass;

class MyClassHolder
{
   public:

   void AddNewObject(const MyClass &o);
   const MyClass & GetLastObject() const;

   private:
   ::boost::container::vector<MyClass> vector_;
};

//]

//[doc_type_erasure_MyClass_h

//MyClass.h

class MyClass
{
   private:
   int value_;

   public:
   MyClass(int val = 0) : value_(val){}

   friend bool operator==(const MyClass &l, const MyClass &r)
   {  return l.value_ == r.value_;  }
   //...
};

//]



//[doc_type_erasure_main_cpp

//Main.cpp

//=#include "MyClassHolder.h"
//=#include "MyClass.h"

#include <cassert>

int main()
{
   MyClass mc(7);
   MyClassHolder myclassholder;
   myclassholder.AddNewObject(mc);
   return myclassholder.GetLastObject() == mc ? 0 : 1;
}
//]

//[doc_type_erasure_MyClassHolder_cpp

//MyClassHolder.cpp

//=#include "MyClassHolder.h"

//In the implementation MyClass must be a complete
//type so we include the appropriate header
//=#include "MyClass.h"

void MyClassHolder::AddNewObject(const MyClass &o)
{  vector_.push_back(o);  }

const MyClass & MyClassHolder::GetLastObject() const
{  return vector_.back();  }

//]

#include <boost/container/detail/config_end.hpp>

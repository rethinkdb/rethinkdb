//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/move for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#include <boost/move/detail/config_begin.hpp>
#include <boost/move/utility.hpp>

//[clone_ptr_base_derived
class Base
{
   BOOST_COPYABLE_AND_MOVABLE(Base)

   public:
   Base(){}

   Base(const Base &x) {/**/}             // Copy ctor

   Base(BOOST_RV_REF(Base) x) {/**/}      // Move ctor

   Base& operator=(BOOST_RV_REF(Base) x)
   {/**/ return *this;}                   // Move assign

   Base& operator=(BOOST_COPY_ASSIGN_REF(Base) x)
   {/**/ return *this;}                   // Copy assign
   
   virtual Base *clone() const
   {  return new Base(*this);  }

   virtual ~Base(){}
};

class Member
{
   BOOST_COPYABLE_AND_MOVABLE(Member)

   public:
   Member(){}

   // Compiler-generated copy constructor...

   Member(BOOST_RV_REF(Member))  {/**/}      // Move ctor

   Member &operator=(BOOST_RV_REF(Member))   // Move assign
   {/**/ return *this;  }

   Member &operator=(BOOST_COPY_ASSIGN_REF(Member))   // Copy assign
   {/**/ return *this;  }
};

class Derived : public Base
{
   BOOST_COPYABLE_AND_MOVABLE(Derived)
   Member mem_;

   public:
   Derived(){}

   // Compiler-generated copy constructor...

   Derived(BOOST_RV_REF(Derived) x)             // Move ctor
      : Base(boost::move(static_cast<Base&>(x))), 
        mem_(boost::move(x.mem_)) { }

   Derived& operator=(BOOST_RV_REF(Derived) x)  // Move assign
   {
      Base::operator=(boost::move(static_cast<Base&>(x)));
      mem_  = boost::move(x.mem_);
      return *this;
   }

   Derived& operator=(BOOST_COPY_ASSIGN_REF(Derived) x)  // Copy assign
   {
      Base::operator=(static_cast<const Base&>(x));
      mem_  = x.mem_;
      return *this;
   }
   // ...
};
//]

//[clone_ptr_def
template <class T>
class clone_ptr
{
   private:
   // Mark this class copyable and movable
   BOOST_COPYABLE_AND_MOVABLE(clone_ptr)
   T* ptr;

   public:
   // Construction
   explicit clone_ptr(T* p = 0) : ptr(p) {}

   // Destruction
   ~clone_ptr() { delete ptr; }
   
   clone_ptr(const clone_ptr& p) // Copy constructor (as usual)
      : ptr(p.ptr ? p.ptr->clone() : 0) {}

   clone_ptr& operator=(BOOST_COPY_ASSIGN_REF(clone_ptr) p) // Copy assignment
   {
      if (this != &p){
         T *tmp_p = p.ptr ? p.ptr->clone() : 0;
         delete ptr;
         ptr = tmp_p;
      }
      return *this;
   }

   //Move semantics...
   clone_ptr(BOOST_RV_REF(clone_ptr) p)            //Move constructor
      : ptr(p.ptr) { p.ptr = 0; }

   clone_ptr& operator=(BOOST_RV_REF(clone_ptr) p) //Move assignment
   {
      if (this != &p){
         delete ptr;
         ptr = p.ptr;
         p.ptr = 0;
      }
      return *this;
   }
};
//]

int main()
{
   {
   //[copy_clone_ptr
   clone_ptr<Base> p1(new Derived());
   // ...
   clone_ptr<Base> p2 = p1;  // p2 and p1 each own their own pointer
   //]
   }
   {
   //[move_clone_ptr
   clone_ptr<Base> p1(new Derived());
   // ...
   clone_ptr<Base> p2 = boost::move(p1);  // p2 now owns the pointer instead of p1
   p2 = clone_ptr<Base>(new Derived());   // temporary is moved to p2
   }
   //]
   //[clone_ptr_move_derived
   Derived d;
   Derived d2(boost::move(d));
   d2 = boost::move(d);
   //]
   return 0;
}

#include <boost/move/detail/config_end.hpp>

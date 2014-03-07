//  (C) Copyright Jeremy Siek 1999. 
//  (C) Copyright David Abrahams 1999.
//  (C) Copyright John Maddock 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_OPERATORS_IN_NAMESPACE
//  TITLE:         friend operators in namespace
//  DESCRIPTION:   Compiler requires inherited operator
//                 friend functions to be defined at namespace scope,
//                 then using'ed to boost.
//                 Probably GCC specific.  See boost/operators.hpp for example.

namespace boost{

//
// the following is taken right out of <boost/operators.hpp>
//
template <class T>
struct addable1
{
     friend T operator+(T x, const T& y) { return x += y; }
     friend bool operator != (const T& a, const T& b) { return !(a == b); }
};

struct spoiler1
{};

spoiler1 operator+(const spoiler1&,const spoiler1&);
bool operator !=(const spoiler1&, const spoiler1&);


}  // namespace boost

namespace boost_no_operators_in_namespace{

struct spoiler2
{};

spoiler2 operator+(const spoiler2&,const spoiler2&);
bool operator !=(const spoiler2&, const spoiler2&);


class add : public boost::addable1<add>
{
   int val;
public:
   add(int i) { val = i; }
   add(const add& a){ val = a.val; }
   add& operator+=(const add& a) { val += a.val; return *this; }
   bool operator==(const add& a)const { return val == a.val; }
};

int test()
{
   add a1(2);
   add a2(3);
   add a3(0);
   a3 = a1 + a2;
   bool b1 = (a1 == a2);
   b1 = (a1 != a2);
   (void)b1;
   return 0;
}

}





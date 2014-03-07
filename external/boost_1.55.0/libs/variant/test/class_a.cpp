//-----------------------------------------------------------------------------
// boost-libs variant/test/class_a.cpp source file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm> // for std::swap
#include <sstream>
#include <iostream>
#include <assert.h>

#include "class_a.h"


using namespace std;

class_a::~class_a()
{
   assert(self_p_ == this);
}

class_a::class_a(int n)
{
   n_ = n;
   self_p_ = this;
}

class_a::class_a(const class_a& other)
{
   n_ = other.n_;
   self_p_ = this;
}


class_a& class_a::operator=(const class_a& rhs)
{
   class_a temp(rhs);
   swap(temp);

   return *this;
}

void class_a::swap(class_a& other)
{
   std::swap(n_, other.n_);
}

int class_a::get() const
{
   return n_;
}




std::ostream& operator<<(std::ostream& strm, const class_a& a)
{
   return strm << "class_a(" << a.get() << ")";
}

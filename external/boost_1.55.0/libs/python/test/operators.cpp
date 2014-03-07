// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/operators.hpp>
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include "test_class.hpp"
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/operators.hpp>
//#include <boost/python/str.hpp>
// Just use math.h here; trying to use std::pow() causes too much
// trouble for non-conforming compilers and libraries.
#include <math.h>

#if __GNUC__ != 2
# include <ostream>
#else
# include <ostream.h>
#endif

using namespace boost::python;


using namespace boost::python;

struct X : test_class<>
{
    typedef test_class<> base_t;
    
    X(int x) : base_t(x) {}
    X const operator+(X const& r) const { return X(value() + r.value()); }
    
//    typedef int (X::*safe_bool)() const;
//    operator safe_bool() const { return value() != 0 ? &X::value : 0; }
};

X operator-(X const& l, X const& r) { return X(l.value() - r.value()); }
X operator-(int l, X const& r) { return X(l - r.value()); }
X operator-(X const& l, int r) { return X(l.value() - r); }

X operator-(X const& x) { return X(-x.value()); }

X& operator-=(X& l, X const& r) { l.set(l.value() - r.value()); return l; }

bool operator<(X const& x, X const& y) { return x.value() < y.value(); }
bool operator<(X const& x, int y) { return x.value() < y; }
bool operator<(int x, X const& y) { return x < y.value(); }

X abs(X x) { return X(x.value() < 0 ? -x.value() : x.value()); }

X pow(X x, int y)
{
    return X(int(pow(double(x.value()), double(y))));
}

X pow(X x, X y)
{
    return X(int(pow(double(x.value()), double(y.value()))));
}

int pow(int x, X y)
{
    return int(pow(double(x), double(y.value())));
}

std::ostream& operator<<(std::ostream& s, X const& x)
{
    return s << x.value();
}

struct number
  : boost::integer_arithmetic<number>
{
    explicit number(long x_) : x(x_) {}
    operator long() const { return x; }

    template <class T>
    number& operator+=(T const& rhs)
    { x += rhs; return *this; }

    template <class T>
    number& operator-=(T const& rhs)
    { x -= rhs; return *this; }
    
    template <class T>
    number& operator*=(T const& rhs)
    { x *= rhs; return *this; }
    
    template <class T>
    number& operator/=(T const& rhs)
    { x /= rhs; return *this; }
    
    template <class T>
    number& operator%=(T const& rhs)
    { x %= rhs; return *this; }

   long x;
};

BOOST_PYTHON_MODULE(operators_ext)
{
    class_<X>("X", init<int>())
        .def("value", &X::value)
        .def(self + self)
        .def(self - self)
        .def(self - int())
        .def(other<int>() - self)
        .def(-self)
        .def(self < other<int>())
        .def(self < self)
        .def(1 < self)
        .def(self -= self)

        .def(abs(self))
        .def(str(self))
            
        .def(pow(self,self))
        .def(pow(self,int()))
        .def(pow(int(),self))
        .def(
            !self
            // "not self" is legal here but causes friction on a few
            // nonconforming compilers; it's cute because it looks
            // like python, but doing it here doesn't prove much and
            // just causes tests to fail or complicated workarounds to
            // be enacted.
        )
        ;

    class_<number>("number", init<long>())
      // interoperate with self
      .def(self += self)
      .def(self + self)
      .def(self -= self)
      .def(self - self)
      .def(self *= self)
      .def(self * self)
      .def(self /= self)
      .def(self / self)
      .def(self %= self)
      .def(self % self)

      // Convert to Python int
      .def(int_(self))

      // interoperate with long
      .def(self += long())
      .def(self + long())
      .def(long() + self)
      .def(self -= long())
      .def(self - long())
      .def(long() - self)
      .def(self *= long())
      .def(self * long())
      .def(long() * self)
      .def(self /= long())
      .def(self / long())
      .def(long() / self)
      .def(self %= long())
      .def(self % long())
      .def(long() % self)
      ;
   
    class_<test_class<1> >("Z", init<int>())
        .def(int_(self))
        .def(float_(self))
        .def(complex_(self))
        ;
}

#include "module_tail.cpp"

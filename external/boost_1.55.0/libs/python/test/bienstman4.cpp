// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/implicit.hpp>
#include <boost/mpl/list.hpp>

struct Type1 {};

struct Term {Term(Type1 const&) {} };

struct Expression {void add(Term const&) {} };

BOOST_PYTHON_MODULE(bienstman4_ext)
{
  using namespace boost::python;
  using boost::mpl::list;

  implicitly_convertible<Type1,Term>();

  class_<Expression>("Expression")
      .def("add", &Expression::add)
      ;
  
  class_<Type1>("T1")
      ;
  
  class_<Term>("Term", init<Type1&>())
      ;
  
  Type1 t1;
  Expression e;
  e.add(t1);
}


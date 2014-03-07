// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

using namespace boost;

class A : public enable_shared_from_this<A> {
 public:
   A() : val(0) {};
   int val;
   typedef shared_ptr<A> A_ptr;
   A_ptr self() {
      A_ptr self;
      self = shared_from_this();
      return self;
    }

};

class B {
  public:
    B() {
       a = A::A_ptr(new A());
    }
    void set(A::A_ptr _a) {
      this->a = _a;
    }
    A::A_ptr get() {
       return a;
    }
    A::A_ptr a;
};

template <class T>
void hold_python(shared_ptr<T>& x)
{
      x = python::extract<shared_ptr<T> >( python::object(x) );
}

A::A_ptr get_b_a(shared_ptr<B> b)
{
    hold_python(b->a);
    return b->get();
}

BOOST_PYTHON_MODULE(andreas_beyer_ext) {
  python::class_<A, noncopyable> ("A")
    .def("self", &A::self)
    .def_readwrite("val", &A::val)
  ;
  python::register_ptr_to_python< A::A_ptr >();
 
  python::class_<B>("B")
     .def("set", &B::set)
//     .def("get", &B::get)
     .def("get", get_b_a)
  ;
}

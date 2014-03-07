// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python.hpp>
#include <string>

struct Foo
{
    Foo(
        int a = 0
        , double b = 0
        , const std::string &n = std::string()
        ) : 
    a_(a)
        , b_(b)
        , n_(n)
    {}

    void set(int a=0, double b=0, const std::string &n=std::string()) 
    {
        a_ = a; 
        b_ = b;
        n_ = n;
    }

    int geta() const { return a_; }

    double getb() const { return b_; }

    std::string getn() const { return n_; }

private:
    int a_;
    double b_;
    std::string n_;
};

struct Bar
{
    Bar(
        int a = 0
        , double b = 0
        , const std::string &n = std::string()
        ) : 
    a_(a)
        , b_(b)
        , n_(n)
    {}

    void set(int a=0, double b=0, const std::string &n=std::string()) 
    {
        a_ = a; 
        b_ = b;
        n_ = n;
    }

    void seta(int a)
    {
        a_ = a; 
    }

    int geta() const { return a_; }

    double getb() const { return b_; }

    std::string getn() const { return n_; }

private:
    int a_;
    double b_;
    std::string n_;
};

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(bar_set, Bar::set, 0,3)

using namespace boost::python;
BOOST_PYTHON_MODULE(keywords)
{
#if BOOST_WORKAROUND(__GNUC__, == 2)
    using boost::python::arg;
#endif 
    
    class_<Foo>(
        "Foo"
      , init<int, double, const std::string&>(
          (  arg("a") = 0
             , arg("b") = 0.0
             , arg("n") = std::string()
          )
      ))

      .def("set", &Foo::set, (arg("a") = 0, arg("b") = 0.0, arg("n") = std::string()) )
       
      .def("set2", &Foo::set, (arg("a"), "b", "n") )
       
      .def("a", &Foo::geta)
      .def("b", &Foo::getb)
      .def("n", &Foo::getn)
      ;

   class_<Bar>("Bar"
               , init<optional<int, double, const std::string &> >()
   )
      .def("set", &Bar::set, bar_set())
      .def("set2", &Bar::set, bar_set("set2's docstring"))
      .def("seta", &Bar::seta, arg("a"))
       
      .def("a", &Bar::geta)
      .def("b", &Bar::getb)
      .def("n", &Bar::getn)
      ;

}



#include "module_tail.cpp"

// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

int a_instances = 0;

int num_a_instances() { return a_instances; }

struct inner
{
    inner(std::string const& s)
        : s(s)
    {}

    void change(std::string const& new_s)
    {
        this->s = new_s;
    }

    std::string s;
};

struct Base
{
    virtual ~Base() {}
};

struct A : Base
{
    A(std::string const& s)
        : x(s)
    {
        ++a_instances;
    }

    ~A()
    {
        --a_instances;
    }

    std::string content() const
    {
        return x.s;
    }

    inner& get_inner()
    {
        return x;
    }

    inner x;
};

struct B
{
    B() : x(0) {}
    B(A* x_) : x(x_) {}

    inner const* adopt(A* _x) { this->x = _x; return &_x->get_inner(); }

    std::string a_content()
    {
        return x ? x->content() : std::string("empty");
    }

    A* x;
};


A* create(std::string const& s)
{
    return new A(s);
}

A* as_A(Base* b)
{
    return dynamic_cast<A*>(b);
}

BOOST_PYTHON_MODULE(test_pointer_adoption_ext)
{
    def("num_a_instances", num_a_instances);

        // Specify the manage_new_object return policy to take
        // ownership of create's result
    def("create", create, return_value_policy<manage_new_object>());

    def("as_A", as_A, return_internal_reference<>());

    class_<Base>("Base")
        ;

    class_<A, bases<Base> >("A", no_init)
        .def("content", &A::content)
        .def("get_inner", &A::get_inner, return_internal_reference<>())
        ;

    class_<inner>("inner", no_init)
        .def("change", &inner::change)
        ;

    class_<B>("B")
        .def(init<A*>()[with_custodian_and_ward_postcall<1,2>()])

        .def("adopt", &B::adopt
             // Adopt returns a pointer referring to a subobject of its 2nd argument (1st being "self")
             , return_internal_reference<2
             // Meanwhile, self holds a reference to the 2nd argument.
             , with_custodian_and_ward<1,2> >()
            )

         .def("a_content", &B::a_content)
        ;
}

#include "module_tail.cpp"

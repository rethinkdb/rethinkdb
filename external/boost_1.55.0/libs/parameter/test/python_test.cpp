// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/python.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/parameter/python.hpp>
#include <boost/utility/enable_if.hpp>

namespace test {

BOOST_PARAMETER_KEYWORD(tags, x)
BOOST_PARAMETER_KEYWORD(tags, y)
BOOST_PARAMETER_KEYWORD(tags, z)

struct Xbase
{
    // We need the disable_if part for VC7.1/8.0.
    template <class Args>
    Xbase(
        Args const& args
      , typename boost::disable_if<
            boost::is_base_and_derived<Xbase, Args>
        >::type* = 0
    )
      : value(std::string(args[x | "foo"]) + args[y | "bar"])
    {}

    std::string value;
};

struct X : Xbase
{
    BOOST_PARAMETER_CONSTRUCTOR(X, (Xbase), tags,
        (optional
         (x, *)
         (y, *)
        )
    )

    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION((int), f, tags,
        (required
         (x, *)
         (y, *)
        )
        (optional
         (z, *)
        )
    )
    {
        return args[x] + args[y] + args[z | 0];
    }

    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION((std::string), g, tags,
        (optional
         (x, *)
         (y, *)
        )
    )
    {
        return std::string(args[x | "foo"]) + args[y | "bar"];
    }

    BOOST_PARAMETER_MEMBER_FUNCTION((X&), h, tags,
        (optional (x, *, "") (y, *, ""))
    )
    {
        return *this;
    }

    template <class A0>
    X& operator()(A0 const& a0)
    {
        return *this;
    }
};

} // namespace test

struct f_fwd
{
    template <class R, class T, class A0, class A1, class A2>
    R operator()(boost::type<R>, T& self, A0 const& a0, A1 const& a1, A2 const& a2)
    {
        return self.f(a0,a1,a2);
    }
};

struct g_fwd
{
    template <class R, class T, class A0, class A1>
    R operator()(boost::type<R>, T& self, A0 const& a0, A1 const& a1)
    {
        return self.g(a0,a1);
    }
};

struct h_fwd
{
    template <class R, class T>
    R operator()(boost::type<R>, T& self)
    {
        return self.h();
    }

    template <class R, class T, class A0>
    R operator()(boost::type<R>, T& self, A0 const& a0)
    {
        return self.h(a0);
    }

    template <class R, class T, class A0, class A1>
    R operator()(boost::type<R>, T& self, A0 const& a0, A1 const& a1)
    {
        return self.h(a0,a1);
    }
};

BOOST_PYTHON_MODULE(python_test_ext)
{
    namespace mpl = boost::mpl;
    using namespace test;
    using namespace boost::python;

    class_<X>("X")
        .def(
            boost::parameter::python::init<
                mpl::vector<
                    tags::x*(std::string), tags::y*(std::string)
                >
            >()
        )
        .def(
            "f"
          , boost::parameter::python::function<
                f_fwd
              , mpl::vector<
                    int, tags::x(int), tags::y(int), tags::z*(int)
                >
            >()
        )
        .def(
            "g"
          , boost::parameter::python::function<
                g_fwd
              , mpl::vector<
                    std::string, tags::x*(std::string), tags::y*(std::string)
                >
            >()
        )
        .def(
            "h"
          , boost::parameter::python::function<
                h_fwd
              , mpl::vector<
                    X&, tags::x**(std::string), tags::y**(std::string)
                >
            >()
          , return_arg<>()
        )
        .def(
            boost::parameter::python::call<
                mpl::vector<
                    X&, tags::x(int)
                >
            >() [ return_arg<>() ]
        )
        .def_readonly("value", &X::value);
}


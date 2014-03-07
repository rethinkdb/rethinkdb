// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/keyword.hpp>
#include <boost/type_traits/is_const.hpp>
#include <string>
#include "basics.hpp"

#ifndef BOOST_NO_SFINAE
# include <boost/utility/enable_if.hpp>
#endif

namespace test {

BOOST_PARAMETER_BASIC_FUNCTION((int), f, tag,
    (required
      (tester, *)
      (name, *)
    )
    (optional
      (value, *)
      (out(index), (int))
    )
)
{
    typedef typename boost::parameter::binding<
        Args, tag::index, int&
    >::type index_type;

    BOOST_MPL_ASSERT((boost::is_same<index_type, int&>));

    args[tester](
        args[name]
      , args[value | 1.f]
      , args[index | 2]
    );

    return 1;
}

BOOST_PARAMETER_BASIC_FUNCTION((int), g, tag,
    (required
      (tester, *)
      (name, *)
    )
    (optional
      (value, *)
      (out(index), (int))
    )
)
{
    typedef typename boost::parameter::binding<
        Args, tag::index, int const&
    >::type index_type;

    BOOST_MPL_ASSERT((boost::is_same<index_type, int const&>));

    args[tester](
        args[name]
      , args[value | 1.f]
      , args[index | 2]
    );

    return 1;
}

BOOST_PARAMETER_FUNCTION((int), h, tag,
    (required
      (tester, *)
      (name, *)
    )
    (optional
      (value, *, 1.f)
      (out(index), (int), 2)
    )
)
{
# if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564)) \
  && !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
    BOOST_MPL_ASSERT((boost::is_same<index_type, int>));
# endif

    tester(
        name
      , value
      , index
    );

    return 1;
}

BOOST_PARAMETER_FUNCTION((int), h2, tag,
    (required
      (tester, *)
      (name, *)
    )
    (optional
      (value, *, 1.f)
      (out(index), (int), (int)value * 2)
    )
)
{
# if !BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564)) \
  && !BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
    BOOST_MPL_ASSERT((boost::is_same<index_type, int>));
# endif

    tester(
        name
      , value
      , index
    );

    return 1;
}

struct base
{
    template <class Args>
    base(Args const& args)
    {
        args[tester](
            args[name]
          , args[value | 1.f]
          , args[index | 2]
        );
    }
};

struct class_ : base
{
    BOOST_PARAMETER_CONSTRUCTOR(class_, (base), tag,
        (required
          (tester, *)
          (name, *)
        )
        (optional
          (value, *)
          (index, *)
        )
    )

    BOOST_PARAMETER_BASIC_MEMBER_FUNCTION((int), f, tag,
        (required
          (tester, *)
          (name, *)
        )
        (optional
          (value, *)
          (index, *)
        )
    )
    {
        args[tester](
            args[name]
          , args[value | 1.f]
          , args[index | 2]
        );

        return 1;
    }

    BOOST_PARAMETER_BASIC_CONST_MEMBER_FUNCTION((int), f, tag,
        (required
          (tester, *)
          (name, *)
        )
        (optional
          (value, *)
          (index, *)
        )
    )
    {
        args[tester](
            args[name]
          , args[value | 1.f]
          , args[index | 2]
        );

        return 1;
    }

    BOOST_PARAMETER_MEMBER_FUNCTION((int), f2, tag,
        (required
          (tester, *)
          (name, *)
        )
        (optional
          (value, *, 1.f)
          (index, *, 2)
        )
    )
    {
        tester(name, value, index);
        return 1;
    }

    BOOST_PARAMETER_CONST_MEMBER_FUNCTION((int), f2, tag,
        (required
          (tester, *)
          (name, *)
        )
        (optional
          (value, *, 1.f)
          (index, *, 2)
        )
    )
    {
        tester(name, value, index);
        return 1;
    }


    BOOST_PARAMETER_MEMBER_FUNCTION((int), static f_static, tag,
        (required
          (tester, *)
          (name, *)
        )
        (optional
          (value, *, 1.f)
          (index, *, 2)
        )
    )
    {
        tester(name, value, index);
        return 1;
    }
};

BOOST_PARAMETER_FUNCTION(
    (int), sfinae, tag,
    (required
       (name, (std::string))
    )
)
{
    return 1;
}

#ifndef BOOST_NO_SFINAE
// On compilers that actually support SFINAE, add another overload
// that is an equally good match and can only be in the overload set
// when the others are not.  This tests that the SFINAE is actually
// working.  On all other compilers we're just checking that
// everything about SFINAE-enabled code will work, except of course
// the SFINAE.
template<class A0>
typename boost::enable_if<boost::is_same<int,A0>, int>::type
sfinae(A0 const& a0)
{
    return 0;
}
#endif

#if BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x580))

// Sun has problems with this syntax:
//
//   template1< r* ( template2<x> ) >
//
// Workaround: factor template2<x> into a separate typedef
typedef boost::is_convertible<boost::mpl::_, std::string> predicate;

BOOST_PARAMETER_FUNCTION(
    (int), sfinae1, tag,
    (required
       (name, *(predicate))
    )
)
{
    return 1;
}

#else

BOOST_PARAMETER_FUNCTION(
    (int), sfinae1, tag,
    (required
       (name, *(boost::is_convertible<boost::mpl::_, std::string>))
    )
)
{
    return 1;
}
#endif 

#ifndef BOOST_NO_SFINAE
// On compilers that actually support SFINAE, add another overload
// that is an equally good match and can only be in the overload set
// when the others are not.  This tests that the SFINAE is actually
// working.  On all other compilers we're just checking that
// everything about SFINAE-enabled code will work, except of course
// the SFINAE.
template<class A0>
typename boost::enable_if<boost::is_same<int,A0>, int>::type
sfinae1(A0 const& a0)
{
    return 0;
}
#endif

template <class T>
T const& as_lvalue(T const& x)
{
    return x;
}

struct udt
{
    udt(int foo, int bar)
      : foo(foo)
      , bar(bar)
    {}

    int foo;
    int bar;
};

BOOST_PARAMETER_FUNCTION((int), lazy_defaults, tag,
    (required
      (name, *)
    )
    (optional
      (value, *, name.foo)
      (index, *, name.bar)
    )
)
{
    return 0;
}

} // namespace test

int main()
{
    using namespace test;

    f(
        values(S("foo"), 1.f, 2)
      , S("foo")
    );

    f(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

    int index_lvalue = 2;

    f(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
      , value = 1.f
      , test::index = index_lvalue
    );

    f(
        values(S("foo"), 1.f, 2)
      , S("foo")
      , 1.f
      , index_lvalue
    );

    g(
        values(S("foo"), 1.f, 2)
      , S("foo")
      , 1.f
#if BOOST_WORKAROUND(BOOST_MSVC, == 1300)
      , as_lvalue(2)
#else
      , 2
#endif
    );

    h(
        values(S("foo"), 1.f, 2)
      , S("foo")
      , 1.f
#if BOOST_WORKAROUND(BOOST_MSVC, == 1300)
      , as_lvalue(2)
#else
      , 2
#endif
    );

    h2(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
      , value = 1.f
    );

    class_ x(
        values(S("foo"), 1.f, 2)
      , S("foo"), test::index = 2
    );

    x.f(
        values(S("foo"), 1.f, 2)
      , S("foo")
    );

    x.f(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

    x.f2(
        values(S("foo"), 1.f, 2)
      , S("foo")
    );

    x.f2(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

    class_ const& x_const = x;

    x_const.f(
        values(S("foo"), 1.f, 2)
      , S("foo")
    );

    x_const.f(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

    x_const.f2(
        values(S("foo"), 1.f, 2)
      , S("foo")
    );

    x_const.f2(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

    x_const.f2(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

    class_::f_static(
        values(S("foo"), 1.f, 2)
      , S("foo")
    );

    class_::f_static(
        tester = values(S("foo"), 1.f, 2)
      , name = S("foo")
    );

#if ! defined(BOOST_NO_SFINAE) && ! BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x592))
    assert(sfinae("foo") == 1);
    assert(sfinae(1) == 0);

# if !BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x580))
    // Sun actually eliminates the desired overload for some reason.
    // Disabling this part of the test because SFINAE abilities are
    // not the point of this test.
    assert(sfinae1("foo") == 1);
# endif
    
    assert(sfinae1(1) == 0);
#endif

    lazy_defaults(
        name = udt(0,1)
    );

    lazy_defaults(
        name = 0
      , value = 1
      , test::index = 2
    );

    return 0;
}


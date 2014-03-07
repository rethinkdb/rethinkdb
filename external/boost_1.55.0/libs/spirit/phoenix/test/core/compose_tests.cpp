/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/spirit/include/phoenix_core.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;

struct X
{
    template <
        typename Env
      , typename A0 = void_
      , typename A1 = void_
      , typename A2 = void_
    >
    struct result
    {
        typedef int type;
    };

    template <typename RT, typename Env
      , typename A0, typename A1, typename A2>
    static RT
    eval(Env const& env, A0& a0, A1& a1, A2& a2)
    {
        return a0.eval(env) + a1.eval(env) + a2.eval(env);
    }
};

int
main()
{
    using boost::fusion::at_c;
    {
        //  testing as_actor
        BOOST_STATIC_ASSERT((boost::is_same<
            as_actor<actor<argument<0> > >::type, actor<argument<0> > >::value));
        BOOST_STATIC_ASSERT((boost::is_same<
            as_actor<int>::type, actor<value<int> > >::value));
    }

    {
        //  testing compose
        char const* s = "Hi";
        int x = 123;

        BOOST_TEST(at_c<0>(compose<X>(1, arg1, val(1)))
            .eval(basic_environment<>()) == 1);
        BOOST_TEST(at_c<1>(compose<X>(1, arg1, val(456)))
            .eval(basic_environment<char const*>(s)) == s);
        BOOST_TEST(at_c<2>(compose<X>(1, arg1, val(456)))
            .eval(basic_environment<>()) == 456);
        BOOST_TEST(compose<X>(9876, arg1, val(456))
            .eval(basic_environment<int>(x)) == 10455);

        //  testing composite sizes
        cout << "sizeof(arg1) is: "
            << sizeof(arg1) << endl;
        cout << "sizeof(compose<X>(arg1)) is: "
            << sizeof(compose<X>(arg1)) << endl;
        cout << "sizeof(compose<X>(1, arg1, val(456))) is: "
            << sizeof(compose<X>(1, arg1, val(456))) << endl;
        cout << "sizeof(compose<X>()) is: "
            << sizeof(compose<X>()) << endl;
        cout << "sizeof(compose<X>('x')) is: "
            << sizeof(compose<X>('x')) << endl;
        cout << "sizeof(compose<X>('x', 3)) is: "
            << sizeof(compose<X>('x', 3)) << endl;
        cout << "sizeof(compose<X>('x', 'y', 3)) is: "
            << sizeof(compose<X>('x', 'y', 3)) << endl;
    }

    return boost::report_errors();
}

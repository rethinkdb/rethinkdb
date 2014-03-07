/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <functional>
#include <boost/detail/lightweight_test.hpp>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>

using namespace phoenix;
using namespace std;

    ///////////////////////////////////////////////////////////////////////////////
    struct print_ { // a typical STL style monomorphic functor

        typedef void result_type;
        void operator()()               { cout << "got no args\n"; }
        void operator()(int n0)         { cout << "got 1 arg " << n0 << " \n"; }
        void operator()(int n0, int n1) { cout << "got 2 args " << n0 << ", " << n1 << " \n"; }

        void foo0() const               { cout << "print_::foo0\n"; }
        void foo1(int n0)               { cout << "print_::foo1 " << n0 << " \n"; }
        void foo2(int n0, int n1)       { cout << "print_::foo2 " << n0 << ", " << n1 << " \n"; }

        int x;
    };

    functor<print_> print = print_();
    member_function_ptr<void, print_, int> print_foo1 = &print_::foo1;
    member_function_ptr<void, print_, int, int> print_foo2 = &print_::foo2;
    member_var_ptr<int, print_> print_x = &print_::x;
    print_ p;
    bound_member<void, print_, int> bound_print_foo1(p,&print_::foo1);
    bound_member<void, print_, int, int> bound_print_foo2(&p,&print_::foo2);

    ///////////////////////////////////////////////////////////////////////////////
    void foo0()                         //  a function w/ 0 args
    { cout << "foo0\n"; }

    void foo1(int n0)                   //  a function w/ 1 arg
    { cout << "foo1 " << n0 << " \n"; }

    void foo2(int n0, int n1)           //  a function w/ 2 args
    { cout << "foo2 " << n0 << ", " << n1 << " \n"; }

    void foo3_(int n0, int n1, int n2)  //  a function w/ 3 args
    { cout << "foo3 " << n0 << ", " << n1 << ", " << n2 << " \n"; }

    function_ptr<void, int, int, int> foo3 = &foo3_;

///////////////////////////////////////////////////////////////////////////////
int
main()
{
    int     i50 = 50, i20 = 20, i100 = 100;

///////////////////////////////////////////////////////////////////////////////
//
//  Binders
//
///////////////////////////////////////////////////////////////////////////////

//  Functor binders

    print()();
    print(111)();
    print(111, arg1)(i100);
    print(111, 222)();
    cout << bind(std::negate<int>())(arg1)(i20) << endl;
    cout << bind(std::plus<int>())(arg1, arg2)(i20, i50) << endl;

//  Function binders

    bind(&foo0)()();
    bind(&foo1)(111)();
    bind(&foo2)(111, arg1)(i100);
    bind(&foo2)(111, 222)();

    foo3(111, 222, 333)();
    foo3(arg1, arg2, arg3)(i20, i50, i100);
    foo3(111, arg1, arg2)(i50, i100);

//  Member function binders

    print_ printer;
    bind(&print_::foo0)(arg1)(printer);

    bind(&print_::foo1)(arg1, 111)(printer);
    print_foo1(arg1, 111)(printer);
    print_foo1(var(printer), 111)();
    print_foo2(var(printer), 111, 222)();
    print_foo2(var(printer), 111, arg1)(i100);

//  Member var binders

    printer.x = 3;
    BOOST_TEST(bind(&print_::x)(arg1)(printer) == 3);
    BOOST_TEST(print_x(arg1)(printer) == 3);
    BOOST_TEST(print_x(printer)() == 3);
    BOOST_TEST(0 != (print_x(var(printer))() = 4));
    BOOST_TEST(printer.x == 4);

//  Bound member functions

    bind(&printer,&print_::foo0)()();

    bind(printer,&print_::foo1)(111)();
    bound_print_foo1(111)();
    bound_print_foo1(111)();
    bound_print_foo2(111, 222)();
    bound_print_foo2(111, arg1)(i100);

    return boost::report_errors();
}

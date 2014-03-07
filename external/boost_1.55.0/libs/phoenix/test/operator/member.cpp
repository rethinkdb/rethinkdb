/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <memory>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

namespace
{
    struct Test
    {
        int value;

        int func(int n) const { return n; }
        int dunc() { return 10; }
        int kunc() const { return 555; }
    };
}

namespace phoenix = boost::phoenix;

int main()
{
    using boost::scoped_ptr;
    using boost::shared_ptr;
    using phoenix::val;
    using phoenix::ref;
    using phoenix::arg_names::arg1;
    using phoenix::arg_names::arg2;

    Test test = {1};
    const Test* cptr = &test;
    Test* ptr = &test;

    BOOST_TEST((val(ptr)->*&Test::value)() == 1);
    BOOST_TEST((val(cptr)->*&Test::value)() == 1);
    BOOST_TEST((arg1->*&Test::value)(cptr) == 1);

    ((val(ptr)->*&Test::value) = 2)();
    BOOST_TEST(test.value == 2);


    BOOST_TEST((val(ptr)->*&Test::func)(val(3))() == 3);
    int i = 33;
    BOOST_TEST((arg1->*&Test::func)(arg2)(cptr, i) == i);
    //BOOST_TEST((val(cptr)->*&Test::func)(4)() == 4);
    BOOST_TEST((val(ptr)->*&Test::dunc)()() == 10);

    BOOST_TEST((arg1->*&Test::func)(5)(ptr) == 5);
    BOOST_TEST((arg1->*&Test::kunc)()(ptr));

    shared_ptr<Test> sptr(new Test(test));

    BOOST_TEST((arg1->*&Test::value)(sptr) == 2);
    BOOST_TEST((arg1->*&Test::func)(6)(sptr) == 6);

    scoped_ptr<Test> scptr(new Test(test));

    BOOST_TEST((arg1->*&Test::value)(scptr) == 2);
    BOOST_TEST((arg1->*&Test::func)(7)(scptr) == 7);

    shared_ptr<const Test> csptr(new Test(test));

    BOOST_TEST((arg1->*&Test::value)(csptr) == 2);
    BOOST_TEST((arg1->*&Test::func)(8)(csptr) == 8);

    scoped_ptr<const Test> cscptr(new Test(test));

    BOOST_TEST((arg1->*&Test::value)(cscptr) == 2);
    BOOST_TEST((arg1->*&Test::func)(9)(cscptr) == 9);

    std::auto_ptr<Test> aptr(new Test(test));

    BOOST_TEST((arg1->*&Test::value)(aptr) == 2);
    BOOST_TEST((arg1->*&Test::func)(10)(aptr) == 10);

    std::auto_ptr<const Test> captr(new Test(test));

    BOOST_TEST((arg1->*&Test::value)(captr) == 2);
    BOOST_TEST((arg1->*&Test::func)(11)(captr) == 11);

    return boost::report_errors();
}

/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2005 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <string>
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/list/cons.hpp>
#include <boost/fusion/container/generation/make_cons.hpp>
#include <boost/fusion/container/generation/cons_tie.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/filter_if.hpp>
#include <boost/fusion/sequence/io/out.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/lambda.hpp>

int
main()
{
    using namespace boost::fusion;
    using boost::is_same;
    namespace fusion = boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

/// Testing cons
    
    {
        std::string hello("hello");
        cons<int, cons<std::string> > ns =
            make_cons(1, make_cons(hello));

        BOOST_TEST((*begin(ns) == 1));
        BOOST_TEST((*fusion::next(begin(ns)) == hello));

        *begin(ns) += 1;
        *fusion::next(begin(ns)) += ' ';

        BOOST_TEST((*begin(ns) == 2));
        BOOST_TEST((*fusion::next(begin(ns)) == hello + ' '));

        for_each(ns, boost::lambda::_1 += ' ');

        BOOST_TEST((*begin(ns) == 2 + ' '));
        BOOST_TEST((*fusion::next(begin(ns)) == hello + ' ' + ' '));
    }

    {
        BOOST_TEST(
            make_cons("hello") == make_vector(std::string("hello"))
        );

        BOOST_TEST(
            make_cons(123, make_cons("hello")) == 
            make_vector(123, std::string("hello"))
        );
    }

    {
        vector<int, float> t(1, 1.1f);
        cons<int, cons<float> > nf =
            make_cons(1, make_cons(1.1f));

        BOOST_TEST((t == nf));
        BOOST_TEST((vector<int>(1) == filter_if<is_same<boost::mpl::_, int> >(nf)));

        std::cout << nf << std::endl;
        std::cout << filter_if<is_same<boost::mpl::_, int> >(nf) << std::endl;
    }
    
    {
        int i = 3;
        cons<int&> tie(cons_tie(i));
        BOOST_TEST((*begin(tie) == 3));
    }

    return boost::report_errors();
}


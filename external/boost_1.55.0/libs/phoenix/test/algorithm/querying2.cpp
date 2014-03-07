/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/stl/algorithm/querying.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <boost/range.hpp>

#include <functional>

namespace
{
    void includes_test()
    {
        using boost::phoenix::includes;
        using boost::phoenix::arg_names::arg1;
        using boost::phoenix::arg_names::arg2;
        int array[] = {1,2,3};
        int array2[] = {1,2};
        BOOST_TEST(includes(arg1, arg2)(array, array2));
        boost::iterator_range<int*> rng(array + 1, array + 3);
        BOOST_TEST(!includes(arg1, arg2)(rng, array2));

        int array3[] = {3,2,1};
        int array4[] = {2,1};
        BOOST_TEST(boost::phoenix::includes(arg1, arg2, std::greater<int>())(array3, array4));
        boost::iterator_range<int*> rng2(array3, array3 + 2);
        BOOST_TEST(!boost::phoenix::includes(arg1, arg2, std::greater<int>())(rng2, array4));
        return;
    }

    void min_element_test()
    {
        using boost::phoenix::min_element;
        using boost::phoenix::arg_names::arg1;
        int array[] = {1,3,2};
        BOOST_TEST(min_element(arg1)(array) == array);
        BOOST_TEST(boost::phoenix::min_element(arg1, std::greater<int>())(array) == array + 1);
        return;
    }

    void max_element_test()
    {
        using boost::phoenix::max_element;
        using boost::phoenix::arg_names::arg1;
        int array[] = {1,3,2};
        BOOST_TEST(max_element(arg1)(array) == array + 1);
        BOOST_TEST(boost::phoenix::max_element(arg1, std::greater<int>())(array) == array);
        return;
    }

    void lexicographical_compare_test()
    {
        using boost::phoenix::lexicographical_compare;
        using boost::phoenix::arg_names::arg1;
        using boost::phoenix::arg_names::arg2;
        int array[] = {1,2,3};
        int array2[] = {1,2,4};

        BOOST_TEST(lexicographical_compare(arg1, arg2)(array, array2));
        BOOST_TEST(!lexicographical_compare(arg1, arg2)(array2, array));
        BOOST_TEST(!lexicographical_compare(arg1, arg2)(array, array));

        BOOST_TEST(!boost::phoenix::lexicographical_compare(arg1, arg2, std::greater<int>())(array, array2));
        BOOST_TEST(boost::phoenix::lexicographical_compare(arg1, arg2, std::greater<int>())(array2, array));
        BOOST_TEST(!boost::phoenix::lexicographical_compare(arg1, arg2, std::greater<int>())(array, array));

        return;
    }
}

int main()
{
    includes_test();
    min_element_test();
    max_element_test();
    lexicographical_compare_test();
    return boost::report_errors();
}

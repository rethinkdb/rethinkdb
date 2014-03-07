/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/spirit/home/phoenix/stl/algorithm/transformation.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <vector>
#include <functional>
#include <algorithm>

namespace
{
    void heap_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        std::vector<int> vec(array, array + 3);
        boost::phoenix::make_heap(arg1)(vec);
        vec.push_back(5);
        boost::phoenix::push_heap(arg1)(vec);
        vec.push_back(4);
        boost::phoenix::push_heap(arg1)(vec);
        boost::phoenix::pop_heap(arg1)(vec);
        BOOST_TEST(vec.back() == 5);
        vec.pop_back();
        boost::phoenix::sort_heap(arg1)(vec);
        int expected_result[] = {1,2,3,4};
        BOOST_TEST(std::equal(vec.begin(), vec.end(), expected_result));

        int array2[] = {3,2,1};
        std::vector<int> vec2(array2, array2 + 3);
        boost::phoenix::make_heap(arg1, std::greater<int>())(vec2);
        vec2.push_back(5);
        boost::phoenix::push_heap(arg1, std::greater<int>())(vec2);
        vec2.push_back(4);
        boost::phoenix::push_heap(arg1, std::greater<int>())(vec2);
        boost::phoenix::pop_heap(arg1, std::greater<int>())(vec2);
        BOOST_TEST(vec2.back() == 1);
        vec2.pop_back();
        boost::phoenix::sort_heap(arg1, std::greater<int>())(vec2);
        int expected_result2[] = {5,4,3,2};
        BOOST_TEST(std::equal(vec2.begin(), vec2.end(), expected_result2));

        return;
    }

    void next_permutation_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2};
        int expected_result[] = {2,1};
        int expected_result2[] = {1,2};

        BOOST_TEST(next_permutation(arg1)(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result));
        BOOST_TEST(!next_permutation(arg1)(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result2));

        std::reverse(array, array + 2);
        BOOST_TEST(boost::phoenix::next_permutation(arg1, std::greater<int>())(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result2));
        BOOST_TEST(!boost::phoenix::next_permutation(arg1, std::greater<int>())(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result));
        return;
    }

    void prev_permutation_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {2,1};
        int expected_result[] = {1,2};
        int expected_result2[] = {2,1};

        BOOST_TEST(prev_permutation(arg1)(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result));
        BOOST_TEST(!prev_permutation(arg1)(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result2));

        std::reverse(array, array + 2);
        BOOST_TEST(boost::phoenix::prev_permutation(arg1, std::greater<int>())(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result2));
        BOOST_TEST(!boost::phoenix::prev_permutation(arg1, std::greater<int>())(array));
        BOOST_TEST(std::equal(array, array + 2, expected_result));
        return;
    }

    void inner_product_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int lhs[] = {1,2,3};
        int rhs[] = {4,5,6};
        BOOST_TEST(inner_product(arg1, arg2, 0)
                   (lhs, rhs) == 1*4 + 2*5 + 3*6);
        BOOST_TEST(boost::phoenix::inner_product(arg1, arg2, 1, std::multiplies<int>(), std::minus<int>())
                   (lhs, rhs) == (1 - 4) * (2 - 5) * (3 - 6));
        return;
    }

    void partial_sum_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int output[3];
        BOOST_TEST(partial_sum(arg1, arg2)(array, output) == output + 3);
        int expected_result[] = {1, 3, 6};
        BOOST_TEST(std::equal(output, output + 3, expected_result));

        BOOST_TEST(boost::phoenix::partial_sum(arg1, arg2, std::multiplies<int>())
                   (array, output) == output + 3);
        int expected_result2[] = {1, 2, 6};
        BOOST_TEST(std::equal(output, output + 3, expected_result2));
        return;
    }

    void adjacent_difference_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int output[3];
        BOOST_TEST(adjacent_difference(arg1, arg2)(array, output) == output + 3);
        int expected_result[] = {1, 1, 1};
        BOOST_TEST(std::equal(output, output + 3, expected_result));
        BOOST_TEST(boost::phoenix::adjacent_difference(arg1, arg2, std::plus<int>())
                   (array, output) == output + 3);
        int expected_result2[] = {1, 3, 5};
        BOOST_TEST(std::equal(output, output + 3, expected_result2));
        return;
    }

}

int main()
{
    heap_test();
    next_permutation_test();
    prev_permutation_test();
    inner_product_test();
    partial_sum_test();
    adjacent_difference_test();
    return boost::report_errors();
}

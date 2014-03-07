/*=============================================================================
    Copyright (c) 2005-2007 Dan Marsden
    Copyright (c) 2005-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/spirit/home/phoenix/stl/algorithm/transformation.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <list>

namespace
{
    void nth_element_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {5,1,4,3,2};
        nth_element(arg1, array + 2)(array);
        BOOST_TEST(array[0] < 3);
        BOOST_TEST(array[1] < 3);
        BOOST_TEST(array[2] == 3);
        BOOST_TEST(array[3] > 3);
        BOOST_TEST(array[4] > 3);

        boost::phoenix::nth_element(arg1, array + 2, std::greater<int>())(array);
        BOOST_TEST(array[0] > 3);
        BOOST_TEST(array[1] > 3);
        BOOST_TEST(array[2] == 3);
        BOOST_TEST(array[3] < 3);
        BOOST_TEST(array[4] < 3);

        return;
    }

    void merge_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int array2[] = {2,3,4};
        int output[6];

        BOOST_TEST(merge(arg1, arg2, arg3)(array, array2, output) == output + 6);
        int expected_result[] = {1,2,2,3,3,4};
        BOOST_TEST(std::equal(output, output + 6, expected_result));

        int array3[] = {5,4,3};
        int array4[] = {3,2,1};
        int output2[6];
        BOOST_TEST(boost::phoenix::merge(arg1, arg2, arg3, std::greater<int>())(array3, array4, output2) ==
                   output2 + 6);
        int expected_result2[] = {5,4,3,3,2,1};
        BOOST_TEST(std::equal(output2, output2 + 6, expected_result2));
        return;
    }

    void inplace_merge_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3,2,3,4};
        inplace_merge(arg1, array + 3)(array);
        int expected_result[] = {1,2,2,3,3,4};
        BOOST_TEST(std::equal(array, array + 6, expected_result));

        int array2[] = {5,4,3,4,3,2};
        boost::phoenix::inplace_merge(arg1, array2 + 3, std::greater<int>())(array2);
        int expected_result2[] = {5,4,4,3,3,2};
        BOOST_TEST(std::equal(array2, array2 + 6, expected_result2));
        return;
    }

    void set_union_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int array2[] = {2,3,4};
        int output[4];
        BOOST_TEST(set_union(arg1, arg2, arg3)(array, array2, output) == output + 4);
        int expected_result[] = {1,2,3,4};
        BOOST_TEST(std::equal(output, output + 4, expected_result));

        int array3[] = {3,2,1};
        int array4[] = {4,3,2};
        int output2[4];
        BOOST_TEST(boost::phoenix::set_union(arg1, arg2, arg3, std::greater<int>())
                   (array3, array4, output2) == 
                   output2 + 4);
        int expected_result2[] = {4,3,2,1};
        BOOST_TEST(std::equal(output2, output2 + 4, expected_result2));
        return;
    }

    void set_intersection_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int array2[] = {2,3,4};
        int output[2];
        BOOST_TEST(set_intersection(arg1, arg2, arg3)(array, array2, output) == output + 2);
        int expected_result[] = {2,3};
        BOOST_TEST(std::equal(output, output + 2, expected_result));

        int array3[] = {3,2,1};
        int array4[] = {4,3,2};
        int output2[2];
        BOOST_TEST(boost::phoenix::set_intersection(arg1, arg2, arg3, std::greater<int>())
                   (array3, array4, output2) == 
                   output2 + 2);
        int expected_result2[] = {3,2};
        BOOST_TEST(std::equal(output2, output2 + 2, expected_result2));
        return;
    }

    void set_difference_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int array2[] = {2,3,4};
        int output[1];
        BOOST_TEST(set_difference(arg1, arg2, arg3)(array, array2, output) == output + 1);
        int expected_result[] = {1};
        BOOST_TEST(std::equal(output, output + 1, expected_result));

        int array3[] = {3,2,1};
        int array4[] = {4,3,2};
        int output2[1];
        BOOST_TEST(boost::phoenix::set_difference(arg1, arg2, arg3, std::greater<int>())
                   (array3, array4, output2) == 
                   output2 + 1);
        int expected_result2[] = {1};
        BOOST_TEST(std::equal(output2, output2 + 1, expected_result2));
        return;
    }

    void set_symmetric_difference_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int array2[] = {2,3,4};
        int output[2];
        BOOST_TEST(set_symmetric_difference(arg1, arg2, arg3)(array, array2, output) == output + 2);
        int expected_result[] = {1,4};
        BOOST_TEST(std::equal(output, output + 2, expected_result));

        int array3[] = {3,2,1};
        int array4[] = {4,3,2};
        int output2[2];
        BOOST_TEST(boost::phoenix::set_symmetric_difference(arg1, arg2, arg3, std::greater<int>())
                   (array3, array4, output2) == 
                   output2 + 2);
        int expected_result2[] = {4,1};
        BOOST_TEST(std::equal(output2, output2 + 2, expected_result2));
        return;
    }
}

int main()
{
    nth_element_test();
    merge_test();
    inplace_merge_test();
    set_union_test();
    set_intersection_test();
    set_difference_test();
    set_symmetric_difference_test();
    return boost::report_errors();
}

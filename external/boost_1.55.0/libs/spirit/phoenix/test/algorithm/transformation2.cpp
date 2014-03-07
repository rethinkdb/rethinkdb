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
    struct even
    {
        bool operator()(const int i) const
        {
            return i % 2 == 0;
        }
    };

    void rotate_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        rotate(arg1, array + 1)(array);
        std::cout << array[0] << array[1] << array[2] << std::endl;
        BOOST_TEST(array[0] == 2);
        BOOST_TEST(array[1] == 3);
        BOOST_TEST(array[2] == 1);
        
        return;
    }

    void rotate_copy_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int array2[3];
        rotate_copy(arg1, array + 1, arg2)(array, array2);
        BOOST_TEST(array2[0] == 2);
        BOOST_TEST(array2[1] == 3);
        BOOST_TEST(array2[2] == 1);
        
        return;
    }

    void random_shuffle_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        random_shuffle(arg1)(array);
        const int first = array[0];
        BOOST_TEST(first == 1 || first == 2 || first == 3);
        const int second = array[1];
        BOOST_TEST(second == 1 || second == 2 || second == 3);
        BOOST_TEST(first != second);
        const int third = array[2];
        BOOST_TEST(third == 1 || third == 2 || third == 3);
        BOOST_TEST(first != third && second != third);
        return;
    }
    
    void partition_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int* const end = partition(arg1, even())(array);
        BOOST_TEST(end == array + 1);
        BOOST_TEST(array[0] % 2 == 0);
        BOOST_TEST(array[1] % 2 != 0);
        BOOST_TEST(array[2] % 2 != 0);
        return;
    }

    void stable_partition_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {1,2,3};
        int* const end = stable_partition(arg1, even())(array);
        BOOST_TEST(end == array + 1);
        BOOST_TEST(array[0] == 2);
        BOOST_TEST(array[1] == 1);
        BOOST_TEST(array[2] == 3);
        return;
    }

    void sort_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {3,1,2};
        std::list<int> test_list(array, array + 3);
        sort(arg1)(array);
        BOOST_TEST(array[0] == 1);
        BOOST_TEST(array[1] == 2);
        BOOST_TEST(array[2] == 3);

        sort(arg1)(test_list);
        std::list<int>::const_iterator it(test_list.begin());
        BOOST_TEST(*it++ == 1);
        BOOST_TEST(*it++ == 2);
        BOOST_TEST(*it++ == 3);

        boost::phoenix::sort(arg1, std::greater<int>())(array);
        BOOST_TEST(array[0] == 3);
        BOOST_TEST(array[1] == 2);
        BOOST_TEST(array[2] == 1);

        boost::phoenix::sort(arg1, std::greater<int>())(test_list);
        std::list<int>::const_iterator jt(test_list.begin());
        BOOST_TEST(*jt++ == 3);
        BOOST_TEST(*jt++ == 2);
        BOOST_TEST(*jt++ == 1);

        return;
    }

    void stable_sort_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {3,1,2};
        stable_sort(arg1)(array);
        BOOST_TEST(array[0] == 1);
        BOOST_TEST(array[1] == 2);
        BOOST_TEST(array[2] == 3);

        boost::phoenix::stable_sort(arg1, std::greater<int>())(array);
        BOOST_TEST(array[0] == 3);
        BOOST_TEST(array[1] == 2);
        BOOST_TEST(array[2] == 1);
        
        return;
    }

    void partial_sort_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {2,4,1,3};
        partial_sort(arg1, array + 2)(array);
        BOOST_TEST(array[0] == 1);
        BOOST_TEST(array[1] == 2);

        boost::phoenix::partial_sort(arg1, array + 2, std::greater<int>())(array);
        BOOST_TEST(array[0] == 4);
        BOOST_TEST(array[1] == 3);
        return;
    }

    void partial_sort_copy_test()
    {
        using namespace boost::phoenix;
        using namespace boost::phoenix::arg_names;
        int array[] = {2,4,1,3};
        int array2[2];
        partial_sort_copy(arg1, arg2)(array, array2);
        BOOST_TEST(array2[0] == 1);
        BOOST_TEST(array2[1] == 2);

        boost::phoenix::partial_sort(arg1, arg2, std::greater<int>())(array, array2);
        BOOST_TEST(array2[0] == 4);
        BOOST_TEST(array2[1] == 3);
        return;
    }
}

int main()
{
    rotate_test();
    rotate_copy_test();
    random_shuffle_test();
    partition_test();
    stable_partition_test();
    sort_test();
    stable_sort_test();
    partial_sort_test();
    return boost::report_errors();
}

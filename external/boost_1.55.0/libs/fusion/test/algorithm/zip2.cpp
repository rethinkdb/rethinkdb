/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/mpl/vector.hpp>

int main()
{
    using namespace boost::fusion;
    {
        const vector2<int,int> shorter(1,2);
        const vector3<char,char,char> longer('a', 'b', 'c');
        const vector2<vector2<int,char>, vector2<int,char> > result(vector2<int,char>(1,'a'), vector2<int,char>(2,'b'));
        BOOST_TEST(zip(shorter, longer) == result);
    }
    {
        const vector3<int,int,int> longer(1,2,3);
        const vector2<char,char> shorter('a', 'b');
        const vector2<vector2<int,char>, vector2<int,char> > result(vector2<int,char>(1,'a'), vector2<int,char>(2,'b'));
        BOOST_TEST(zip(longer, shorter) == result);
    }
    return boost::report_errors();
}

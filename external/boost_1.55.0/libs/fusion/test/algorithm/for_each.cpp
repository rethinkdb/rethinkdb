/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/mpl/vector_c.hpp>

struct print
{
    template <typename T>
    void operator()(T const& v) const
    {
        std::cout << "[ " << v << " ] ";
    }
};

struct increment
{
    template <typename T>
    void operator()(T& v) const
    {
        ++v;
    }
};

int
main()
{
    using namespace boost::fusion;
    using boost::mpl::vector_c;
    namespace fusion = boost::fusion;

    {
        typedef vector<int, char, double, char const*> vector_type;
        vector_type v(1, 'x', 3.3, "Ruby");
        for_each(v, print());
        std::cout << std::endl;
    }

    {
        typedef vector<int, char, double, char const*> vector_type;
        vector_type v(1, 'x', 3.3, "Ruby");
        for_each(v, increment());
        std::cout << v << std::endl;
    }

    {
        typedef vector_c<int, 2, 3, 4, 5, 6> mpl_vec;
        fusion::for_each(mpl_vec(), print());
        std::cout << std::endl;
    }

    return boost::report_errors();
}


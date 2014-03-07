/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <string>
#include <sstream>
#include <iostream>
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include "../sequence/tree.hpp"

struct write_string
{
    typedef std::ostream* result_type;

    template<typename T>
    std::ostream* operator()(std::ostream* sout, T const& t) const
    {
        return &(*sout << t << " ");
    }
};

template<typename Tree>
void 
process_tree(Tree const &tree)
{
    using namespace boost;

    std::stringstream str;
    fusion::fold(tree, &str, write_string());
    std::string res = str.str();

    BOOST_TEST_EQ(res, "a b c 1 2 3 100 e f 0 B 1 h i 4 5 6 j k l ");
}

int
main()
{
    using namespace boost::fusion;
    process_tree(
        make_tree(
            make_vector(double(0),'B')
            , make_tree(
                make_vector(1,2,long(3))
                , make_tree(make_vector('a','b','c'))
                , make_tree(make_vector(short('d'),'e','f'))
            )
            , make_tree(
                make_vector(4,5,6)
                , make_tree(make_vector(float(1),'h','i'))
                , make_tree(make_vector('j','k','l'))
            )
        )
    );

    return boost::report_errors();
}


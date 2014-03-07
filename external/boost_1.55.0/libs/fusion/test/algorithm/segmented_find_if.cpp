/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/algorithm/query/find_if.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_same.hpp>
#include "../sequence/tree.hpp"

struct not_there {};

template<typename Tree>
void 
process_tree(Tree const &tree)
{
    using namespace boost;
    using mpl::_;

    typedef typename boost::fusion::result_of::find_if<Tree const, is_same<_,short> >::type short_iter;
    typedef typename boost::fusion::result_of::find_if<Tree const, is_same<_,float> >::type float_iter;
    typedef typename boost::fusion::result_of::find_if<Tree const, is_same<_,not_there> >::type not_there_iter;

    // find_if of a segmented data structure returns generic
    // segmented iterators
    short_iter si = fusion::find_if<is_same<_,short> >(tree);
    float_iter fi = fusion::find_if<is_same<_,float> >(tree);

    // they behave like ordinary Fusion iterators ...
    BOOST_TEST((*si == short('d')));
    BOOST_TEST((*fi == float(1)));

    // Searching for something that's not there should return the end iterator.
    not_there_iter nti = fusion::find_if<is_same<_,not_there> >(tree);
    BOOST_TEST((nti == fusion::end(tree)));
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


/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <sstream>
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/query/find_if.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/view/iterator_range/iterator_range.hpp>
#include <boost/fusion/sequence/comparison/equal_to.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/static_assert.hpp>
#include "tree.hpp"

struct ostream_fun
{
    ostream_fun(std::ostream &sout)
        : sout_(sout)
    {}
    template<typename T>
    void operator ()(T const &t) const
    {
        sout_ << t << ' ';
    }
private:
    std::ostream & sout_;
};

template<typename Tree>
void
process_tree(Tree const &tree)
{
    using namespace boost;
    using namespace fusion;
    using mpl::_;

    typedef typename boost::fusion::result_of::find_if<Tree const, is_same<_,short> >::type short_iter;
    typedef typename boost::fusion::result_of::find_if<Tree const, is_same<_,float> >::type float_iter;

    typedef iterator_range<short_iter, float_iter> slice_t;
    BOOST_STATIC_ASSERT(traits::is_segmented<slice_t>::value);

    // find_if of a segmented data structure returns generic
    // segmented iterators
    short_iter si = find_if<is_same<_,short> >(tree);
    float_iter fi = find_if<is_same<_,float> >(tree);

    // If you put them in an iterator range, the range
    // is automatically a segmented data structure.
    slice_t slice(si, fi);

    std::stringstream sout;
    fusion::for_each(slice, ostream_fun(sout));
    BOOST_TEST((sout.str() == "100 e f 0 B "));
}

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        char const* s = "Ruby";
        typedef vector<int, char, double, char const*> vector_type;
        vector_type vec(1, 'x', 3.3, s);

        {
            typedef vector_iterator<vector_type, 1> i1t;
            typedef vector_iterator<vector_type, 3> i3t;

            i1t i1(vec);
            i3t i3(vec);

            typedef iterator_range<i1t, i3t> slice_t;
            slice_t slice(i1, i3);
            std::cout << slice << std::endl;
            BOOST_TEST((slice == make_vector('x', 3.3)));
            BOOST_STATIC_ASSERT(boost::fusion::result_of::size<slice_t>::value == 2);
        }

        {
            typedef vector_iterator<vector_type, 0> i1t;
            typedef vector_iterator<vector_type, 0> i3t;

            i1t i1(vec);
            i3t i3(vec);

            typedef iterator_range<i1t, i3t> slice_t;
            slice_t slice(i1, i3);
            std::cout << slice << std::endl;
            BOOST_TEST(slice == make_vector());
            BOOST_STATIC_ASSERT(boost::fusion::result_of::size<slice_t>::value == 0);
        }
    }

    {
        typedef boost::mpl::vector_c<int, 2, 3, 4, 5, 6> mpl_vec;
        typedef boost::mpl::begin<mpl_vec>::type it0;
        typedef boost::mpl::next<it0>::type it1;
        typedef boost::mpl::next<it1>::type it2;
        typedef boost::mpl::next<it2>::type it3;

        it1 f;
        it3 l;
        
        typedef iterator_range<it1, it3> slice_t;
        slice_t slice(f, l);
        std::cout << slice << std::endl;
        BOOST_TEST((slice == make_vector(3, 4)));
        BOOST_STATIC_ASSERT(boost::fusion::result_of::size<slice_t>::value == 2);
    }

    {
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
    }

    return boost::report_errors();
}


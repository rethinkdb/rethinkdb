/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/sequence/io/out.hpp>
#include <boost/fusion/view/single_view/single_view.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/front.hpp>
#include <boost/fusion/sequence/intrinsic/back.hpp>
#include <boost/fusion/sequence/intrinsic/empty.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/prior.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

struct X {};

template <typename OS>
OS& operator<<(OS& os, X const&)
{
    os << "<X-object>";
    return os;
}

void foo() {}

int
main()
{
    using namespace boost::fusion;

    std::cout << tuple_open('[');
    std::cout << tuple_close(']');
    std::cout << tuple_delimiter(", ");

    {
        single_view<int> view1(3);
        std::cout << view1 << std::endl;
        
#ifdef FUSION_TEST_FAIL
        // single_view is immutable
        *begin(view1) += 4;
#endif
        std::cout << view1 << std::endl;
        BOOST_TEST(*begin(view1) == 3);
        BOOST_TEST(at<boost::mpl::int_<0> >(view1) == 3);
        BOOST_TEST(view1.val == 3);
        BOOST_TEST(3 == front(view1));
        BOOST_TEST(3 == back(view1));
        BOOST_TEST(!empty(view1));
        BOOST_TEST(next(begin(view1)) == end(view1));
        BOOST_TEST(prior(end(view1)) == begin(view1));
        BOOST_TEST(!(next(begin(view1)) != end(view1)));
        BOOST_TEST(!(prior(end(view1)) != begin(view1)));
        BOOST_TEST(1 == distance(begin(view1), end(view1)));
        BOOST_TEST(0 == distance(end(view1), end(view1)));
        BOOST_TEST(0 == distance(begin(view1), begin(view1)));
        BOOST_TEST(end(view1) == advance<boost::mpl::int_<1> >(begin(view1)));
        BOOST_TEST(begin(view1) == advance<boost::mpl::int_<0> >(begin(view1)));
        BOOST_TEST(end(view1) == advance<boost::mpl::int_<0> >(end(view1)));
        BOOST_TEST(begin(view1) == advance<boost::mpl::int_<-1> >(end(view1)));
        BOOST_TEST(end(view1) == advance_c<1>(begin(view1)));
        BOOST_TEST(begin(view1) == advance_c<0>(begin(view1)));
        BOOST_TEST(end(view1) == advance_c<0>(end(view1)));
        BOOST_TEST(begin(view1) == advance_c<-1>(end(view1)));
        BOOST_TEST(1 == size(view1));
        BOOST_MPL_ASSERT((boost::is_same<int, boost::fusion::result_of::value_at<single_view<int>, boost::mpl::int_<0> >::type>));

        single_view<X> view2;
        std::cout << view2 << std::endl;
    }
    
    {
        std::cout << make_single_view(1) << std::endl;
        std::cout << make_single_view("Hello, World") << std::endl;
        std::cout << make_single_view(&foo) << std::endl;
    }

    return boost::report_errors();
}


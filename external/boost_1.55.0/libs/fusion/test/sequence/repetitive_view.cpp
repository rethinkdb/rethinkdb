/*=============================================================================
    Copyright (c) 2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/detail/lightweight_test.hpp>
#include <boost/fusion/view/repetitive_view.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/view/joint_view.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>

#include <boost/type_traits/is_same.hpp>

#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/iterator/next.hpp>

#include <boost/fusion/functional/generation/make_fused_procedure.hpp>

struct check_equal
{
    template<typename LHS, typename RHS>
    void operator()(LHS const& lhs, RHS const& rhs) const
    {
        BOOST_TEST(( boost::is_same<LHS,RHS>::value ));
        BOOST_TEST(( lhs == rhs )); 
    }
};

int main()
{
    using namespace boost::fusion;

    typedef boost::fusion::vector<int,long,float,double> seq_t;
    seq_t seq(1,2l,3.0f,4.0);

    typedef repetitive_view<seq_t> view_t;
    view_t view(seq);

    typedef joint_view<seq_t,seq_t> seq_twice_t;
    typedef joint_view<seq_t,seq_twice_t> seq_repeated_t;
    seq_twice_t seq_twice(seq,seq);
    seq_repeated_t seq_repeated(seq,seq_twice);

    for_each(zip(view,seq_repeated), make_fused_procedure(check_equal()));

    return boost::report_errors();
}


/*=============================================================================
    Copyright (c) 2011 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_SEGMENTED_FOR_EACH_HPP_INCLUDED)
#define BOOST_FUSION_SEGMENTED_FOR_EACH_HPP_INCLUDED

#include <boost/mpl/bool.hpp>
#include <boost/fusion/support/void.hpp>
#include <boost/fusion/algorithm/iteration/for_each_fwd.hpp>
#include <boost/fusion/support/segmented_fold_until.hpp>

namespace boost { namespace fusion { namespace detail
{
    template <typename Fun>
    struct segmented_for_each_fun
    {
        explicit segmented_for_each_fun(Fun const& f)
          : fun(f)
        {}

        Fun const& fun;

        template <typename Sequence, typename State, typename Context>
        struct apply
        {
            typedef void_ type;
            typedef mpl::true_ continue_type;

            static type call(Sequence& seq, State const&, Context const&, segmented_for_each_fun const& fun)
            {
                fusion::for_each(seq, fun.fun);
                return void_();
            }
        };
    };

    template <typename Sequence, typename F>
    inline void
    for_each(Sequence& seq, F const& f, mpl::true_) // segmented implementation
    {
        fusion::segmented_fold_until(seq, void_(), segmented_for_each_fun<F>(f));
    }
}}}

#endif

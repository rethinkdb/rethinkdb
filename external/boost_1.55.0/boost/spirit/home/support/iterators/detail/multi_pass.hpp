//  Copyright (c) 2001 Daniel C. Nuffer
//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_MULTI_PASS_MAR_16_2007_1122AM)
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_MAR_16_2007_1122AM

#include <boost/config.hpp>
#include <boost/spirit/home/support/iterators/multi_pass_fwd.hpp>
#include <boost/iterator.hpp>
#include <boost/mpl/bool.hpp>
#include <iterator>
#include <algorithm> 

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace detail
{
#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    ///////////////////////////////////////////////////////////////////////////
    //  Meta-function to generate a std::iterator<> base class for multi_pass. 
    //  This is used mainly to improve conformance of compilers not supporting 
    //  PTS and thus relying on inheritance to recognize an iterator.
    //
    //  We are using boost::iterator<> because it offers an automatic 
    //  workaround for broken std::iterator<> implementations.
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename InputPolicy>
    struct iterator_base_creator
    {
        typedef typename InputPolicy::BOOST_NESTED_TEMPLATE unique<T> input_type;

        typedef boost::iterator <
            std::forward_iterator_tag
          , typename input_type::value_type
          , typename input_type::difference_type
          , typename input_type::pointer
          , typename input_type::reference
        > type;
    };
#endif

    ///////////////////////////////////////////////////////////////////////////
    //  Default implementations of the different policies to be used with a 
    //  multi_pass iterator
    ///////////////////////////////////////////////////////////////////////////
    struct default_input_policy
    {
        default_input_policy() {}

        template <typename Functor>
        default_input_policy(Functor const&) {}

        template <typename MultiPass>
        static void destroy(MultiPass&) {}

        void swap(default_input_policy&) {}

        template <typename MultiPass, typename TokenType>
        static void advance_input(MultiPass& mp);

        template <typename MultiPass>
        static typename MultiPass::reference get_input(MultiPass& mp);

        template <typename MultiPass>
        static bool input_at_eof(MultiPass const& mp);

        template <typename MultiPass, typename TokenType>
        static bool input_is_valid(MultiPass& mp, TokenType& curtok);
    };

    struct default_ownership_policy
    {
        template <typename MultiPass>
        static void destroy(MultiPass&) {}

        void swap(default_ownership_policy&) {}

        template <typename MultiPass>
        static void clone(MultiPass&) {}

        template <typename MultiPass>
        static bool release(MultiPass& mp);

        template <typename MultiPass>
        static bool is_unique(MultiPass const& mp);
    };

    struct default_storage_policy
    {
        template <typename MultiPass>
        static void destroy(MultiPass&) {}

        void swap(default_storage_policy&) {}

        template <typename MultiPass>
        static typename MultiPass::reference dereference(MultiPass const& mp);

        template <typename MultiPass>
        static void increment(MultiPass&) {}

        template <typename MultiPass>
        static void clear_queue(MultiPass&) {}

        template <typename MultiPass>
        static bool is_eof(MultiPass const& mp);

        template <typename MultiPass>
        static bool equal_to(MultiPass const& mp, MultiPass const& x);

        template <typename MultiPass>
        static bool less_than(MultiPass const& mp, MultiPass const& x);
    };

    struct default_checking_policy
    {
        template <typename MultiPass>
        static void destroy(MultiPass&) {}

        void swap(default_checking_policy&) {}

        template <typename MultiPass>
        static void docheck(MultiPass const&) {}

        template <typename MultiPass>
        static void clear_queue(MultiPass&) {}
    };

}}}

#endif

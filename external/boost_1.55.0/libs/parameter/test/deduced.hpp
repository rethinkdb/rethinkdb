// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_DEDUCED_060920_HPP
# define BOOST_DEDUCED_060920_HPP

# include <boost/mpl/for_each.hpp>
# include "basics.hpp"

struct not_present_tag {};
not_present_tag not_present;

template <class E, class ArgPack>
struct assert_expected
{
    assert_expected(E const& e, ArgPack const& args)
      : expected(e)
      , args(args)
    {}

    template <class T>
    bool check_not_present(T const&) const
    {
        BOOST_MPL_ASSERT((boost::is_same<T,not_present_tag>));
        return true;
    }

    template <class K>
    bool check1(K const& k, not_present_tag const&, long) const
    {
        return check_not_present(args[k | not_present]);
    }

    template <class K, class Expected>
    bool check1(K const& k, Expected const& expected, int) const
    {
        return test::equal(args[k], expected);
    }

    template <class K>
    void operator()(K) const
    {
        boost::parameter::keyword<K> const& k = boost::parameter::keyword<K>::get();
        assert(check1(k, expected[k], 0L));
    }

    E const& expected;
    ArgPack const& args;
};

template <class E, class ArgPack>
void check0(E const& e, ArgPack const& args)
{
    boost::mpl::for_each<E>(assert_expected<E,ArgPack>(e, args));
}

template <class P, class E, class A0>
void check(E const& e, A0 const& a0)
{
    check0(e, P()(a0));
}

template <class P, class E, class A0, class A1>
void check(E const& e, A0 const& a0, A1 const& a1)
{
    check0(e, P()(a0,a1));
}

template <class P, class E, class A0, class A1, class A2>
void check(E const& e, A0 const& a0, A1 const& a1, A2 const& a2)
{
    check0(e, P()(a0,a1,a2));
}

#endif // BOOST_DEDUCED_060920_HPP


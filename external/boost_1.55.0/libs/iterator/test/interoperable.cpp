// Copyright David Abrahams 2004. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/pending/iterator_tests.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <cassert>

struct mutable_it : boost::iterator_adaptor<mutable_it,int*>
{
    typedef boost::iterator_adaptor<mutable_it,int*> super_t;
    
    mutable_it();
    explicit mutable_it(int* p) : super_t(p) {}
    
    bool equal(mutable_it const& rhs) const
    {
        return this->base() == rhs.base();
    }
};

struct constant_it : boost::iterator_adaptor<constant_it,int const*>
{
    typedef boost::iterator_adaptor<constant_it,int const*> super_t;
    
    constant_it();
    explicit constant_it(int* p) : super_t(p) {}
    constant_it(mutable_it const& x) : super_t(x.base()) {}

    bool equal(constant_it const& rhs) const
    {
        return this->base() == rhs.base();
    }
};

int main()
{
    int data[] = { 49, 77 };
    
    mutable_it i(data);
    constant_it j(data + 1);
    BOOST_TEST(i < j);
    BOOST_TEST(j > i);
    BOOST_TEST(i <= j);
    BOOST_TEST(j >= i);
    BOOST_TEST(j - i == 1);
    BOOST_TEST(i - j == -1);
    
    constant_it k = i;

    BOOST_TEST(!(i < k));
    BOOST_TEST(!(k > i));
    BOOST_TEST(i <= k);
    BOOST_TEST(k >= i);
    BOOST_TEST(k - i == 0);
    BOOST_TEST(i - k == 0);
    
    return boost::report_errors();
}

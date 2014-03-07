/* Copyright (C) 2011 Kwan Ting Chan
 * Based from bug report submitted by Xiaohan Wang
 * 
 * Use, modification and distribution is subject to the 
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 */

// Test of bug #3349 (https://svn.boost.org/trac/boost/ticket/3349)

#include <boost/pool/pool.hpp>

#include <boost/detail/lightweight_test.hpp>

int main()
{
    boost::pool<> p(256, 4);

    void* pBlock1 = p.ordered_malloc( 1 );
    void* pBlock2 = p.ordered_malloc( 4 );
    (void)pBlock2; // warning suppression

    p.ordered_free( pBlock1 );

    BOOST_TEST(p.release_memory());
}

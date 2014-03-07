/*=============================================================================
    Copyright (C) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Nota bene: the actual locking is _not_ tested here!

#include <iostream>
#include <boost/config.hpp>

void banner()
{
    std::cout << "/////////////////////////////////////////////////////////\n";
    std::cout << "\n";
    std::cout << "          scoped_lock test\n";
    std::cout << "\n";
    std::cout << "/////////////////////////////////////////////////////////\n";
    std::cout << "\n";
}

#if defined(DONT_HAVE_BOOST) || !defined(BOOST_HAS_THREADS) || defined(BOOST_DISABLE_THREADS)
// if boost libraries are not available we have to skip the tests
int
main()
{
    banner();
    std::cout << "Test skipped (Boost libaries not available)\n";
    return 0;
}
#else

#include <boost/thread/mutex.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_scoped_lock.hpp>
#include <boost/detail/lightweight_test.hpp>

int
main()
{
    banner();

    using BOOST_SPIRIT_CLASSIC_NS::rule;
    using BOOST_SPIRIT_CLASSIC_NS::scoped_lock_d;
    using BOOST_SPIRIT_CLASSIC_NS::parse_info;
    using BOOST_SPIRIT_CLASSIC_NS::parse;
    using boost::mutex;

    mutex m;
    rule<> r = scoped_lock_d(m)['x'];
    parse_info<> pi = parse("x", r);
    BOOST_TEST(pi.hit);
    BOOST_TEST(pi.full);

    return boost::report_errors();
}

#endif // defined(DONT_HAVE_BOOST)

/*=============================================================================
    Copyright (c) 2003 Martin Wille
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

  // see http://article.gmane.org/gmane.comp.parsers.spirit.general/4575
  // or https://sf.net/mailarchive/forum.php?thread_id=2692308&forum_id=1595
  // for a description of the bug being tested for by this program
  //
  // This code is borrowed from Spirit's bug_000008.cpp test for multithreads.
#include <iostream>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/home/phoenix/scope/dynamic.hpp>

#if defined(DONT_HAVE_BOOST)                        \
    || !defined(BOOST_HAS_THREADS)                  \
    || defined(BOOST_DISABLE_THREADS)               \
    || (defined(__GNUC__) && defined(__WIN32__))    // MinGW
#define SKIP_TEST
#endif


#if defined(SKIP_TEST)
// we end here if we can't do multithreading
static void skipped()
{
    std::cout << "skipped\n";
}

int
main()
{
    skipped();
    return boost::report_errors();
}

#else
// the real MT stuff

#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_scope.hpp>
#include <boost/thread.hpp>

static const int number_of_calls_per_thread=20000;

struct test_dynamic : boost::phoenix::dynamic<int>
{
    test_dynamic() : b(*this) {}
    member1 b;
};

void
in_thread(void)
{
    test_dynamic s; // should now be a local

    for (int i = 0; i < number_of_calls_per_thread; ++i)
    {
        boost::phoenix::dynamic_frame<test_dynamic::self_type> frame(s);
        (s.b = 123)();
        {
            boost::phoenix::dynamic_frame<test_dynamic::self_type> frame(s);
            (s.b = 456)();
            BOOST_ASSERT((s.b == 456)());
        }
        BOOST_ASSERT((s.b == 123)());
    }
}

void
bug_000008()
{
    boost::thread t1(in_thread);
    boost::thread t2(in_thread);
    boost::thread t3(in_thread);
    boost::thread t4(in_thread);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

int
main()
{
    bug_000008();
    return boost::report_errors();
}

#endif


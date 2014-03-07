/*=============================================================================
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

  // see http://article.gmane.org/gmane.comp.parsers.spirit.general/4575
  // or https://sf.net/mailarchive/forum.php?thread_id=2692308&forum_id=1595
  // for a description of the bug being tested for by this program
  //
  // the problem should be solved with version 1.3 of phoenix/closures.hpp>

#if defined(BOOST_SPIRIT_DEBUG) && defined(__GNUC__) && defined(__WIN32__)
// It seems that MinGW has some problems with threads and iostream ?
// This code crashes MinGW when BOOST_SPIRIT_DEBUG is defined. The reason
// is beyond me. Disable BOOST_SPIRIT_DEBUG for now.
#undef BOOST_SPIRIT_DEBUG
#endif

#include <iostream>
#include <boost/config.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined(DONT_HAVE_BOOST) || !defined(BOOST_HAS_THREADS) || defined(BOOST_DISABLE_THREADS)
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

#undef BOOST_SPIRIT_THREADSAFE
#define BOOST_SPIRIT_THREADSAFE
#undef PHOENIX_THREADSAFE
#define PHOENIX_THREADSAFE

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_closure.hpp>
#include <boost/thread.hpp>

static const int number_of_calls_to_parse_per_thread=20000;

struct test_closure
    : BOOST_SPIRIT_CLASSIC_NS::closure<test_closure, char const*>
{
    member1 b;
};

struct test_grammar
    : BOOST_SPIRIT_CLASSIC_NS::grammar<test_grammar, test_closure::context_t>
{
    test_grammar() {}

    template <typename ScannerT>
    struct definition
    {
        definition(test_grammar const &self)
        {
            using namespace phoenix;
            rule = BOOST_SPIRIT_CLASSIC_NS::epsilon_p[self.b = arg1];
        }

        BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT> const &start() const { return rule; }

        BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT> rule;
    };
};

test_grammar const g;

void
in_thread(void)
{
    char const text[]="foo";
    for(int i=0; i<number_of_calls_to_parse_per_thread; ++i)
    {
        BOOST_SPIRIT_CLASSIC_NS::parse(&text[0], text+sizeof(text), g);
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


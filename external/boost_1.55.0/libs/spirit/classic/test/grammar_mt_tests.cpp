/*=============================================================================
    Copyright (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
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
    return 0;
}

#else
// the real MT stuff

#undef BOOST_SPIRIT_THREADSAFE
#define BOOST_SPIRIT_THREADSAFE

#include <boost/thread/thread.hpp>
#include <boost/spirit/include/classic_grammar.hpp>
#include <boost/spirit/include/classic_rule.hpp>
#include <boost/spirit/include/classic_epsilon.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/ref.hpp>

static boost::mutex simple_mutex;
static int simple_definition_count = 0;

struct simple : public BOOST_SPIRIT_CLASSIC_NS::grammar<simple>
{
    template <typename ScannerT>
    struct definition
    {
        definition(simple const& /*self*/)
        {
            top = BOOST_SPIRIT_CLASSIC_NS::epsilon_p;
            boost::mutex::scoped_lock lock(simple_mutex);
            simple_definition_count++;
        }

        BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT> top;
        BOOST_SPIRIT_CLASSIC_NS::rule<ScannerT> const &start() const { return top; }
    };
};

struct count_guard
{
    count_guard(int &c) : counter(c) {}
    ~count_guard() { counter = 0; }
private:
    int &counter;
};

static void
milli_sleep(unsigned long milliseconds)
{
    static long const nanoseconds_per_second = 1000L*1000L*1000L;
    boost::xtime xt;
    boost::xtime_get(&xt, boost::TIME_UTC_);
    xt.nsec+=1000*1000*milliseconds;
    while (xt.nsec > nanoseconds_per_second)
    {
        xt.nsec -= nanoseconds_per_second;
        xt.sec++;
    }
        
    boost::thread::sleep(xt);
}

static void
nap()
{
    // this function is called by various threads to ensure
    // that thread lifetime actually overlap
    milli_sleep(300);
}

template <typename GrammarT>
static void
make_definition(GrammarT &g)
{
    char const *text="blah";
    BOOST_SPIRIT_CLASSIC_NS::scanner<> s(text, text+4);

    g.parse(s);
}

template <typename GrammarT>
static void
make_definition3(GrammarT &g)
{
    char const *text="blah";
    BOOST_SPIRIT_CLASSIC_NS::scanner<> s(text, text+4);

    g.parse(s);
    nap();
    g.parse(s);
    g.parse(s);
}
////////////////////////////////////////////////////////////////////////////////
#define exactly_one_instance_created simple_definition_count == 1
#define exactly_two_instances_created simple_definition_count == 2
#define exactly_four_instances_created simple_definition_count == 4
#define exactly_eight_instances_created simple_definition_count == 8

////////////////////////////////////////////////////////////////////////////////
static void
multiple_attempts_to_instantiate_a_definition_from_a_single_thread()
{
    // checks wether exactly one definition per grammar
    // object is created

    count_guard guard(simple_definition_count);

    simple simple1_p;
    simple simple2_p;

    make_definition(simple1_p);
    make_definition(simple1_p);
    make_definition(simple1_p);

    BOOST_TEST(exactly_one_instance_created);

    make_definition(simple2_p);
    make_definition(simple2_p);
    make_definition(simple2_p);

    BOOST_TEST(exactly_two_instances_created);
}

////////////////////////////////////////////////////////////////////////////////
struct single_grammar_object_task
{
    void operator()() const
    {
        make_definition3(simple1_p);
    };

    simple simple1_p;
};
////////////////////////////////////////////////////////////////////////////////
template <typename T>
class callable_reference_wrapper
    : public boost::reference_wrapper<T>
{
public:
    explicit callable_reference_wrapper(T& t)
        : boost::reference_wrapper<T>(t)
    {}
    inline void operator()() { this->get().operator()(); }
};

template <typename T>
callable_reference_wrapper<T>
callable_ref(T &t)
{
    return callable_reference_wrapper<T>(t);
}
////////////////////////////////////////////////////////////////////////////////
static void
single_local_grammar_object_multiple_threads()
{
    // check wether independent definition objects are
    // created
    count_guard guard(simple_definition_count);
    single_grammar_object_task task1, task2, task3, task4;

    boost::thread t1(callable_ref(task1));
    boost::thread t2(callable_ref(task2));
    boost::thread t3(callable_ref(task3));
    boost::thread t4(callable_ref(task4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    BOOST_TEST(exactly_four_instances_created);
}

////////////////////////////////////////////////////////////////////////////////
struct two_grammar_objects_task
{
    void operator()() const
    {
        make_definition3(simple1_p);
        make_definition3(simple2_p);
    };

    simple simple1_p;
    simple simple2_p;
};

static void
multiple_local_grammar_objects_multiple_threads()
{
    // check wether exactly one definition per thread 
    // and per grammar object is created
    count_guard guard(simple_definition_count);
    two_grammar_objects_task task1, task2, task3, task4;

    boost::thread t1(callable_ref(task1));
    boost::thread t2(callable_ref(task2));
    boost::thread t3(callable_ref(task3));
    boost::thread t4(callable_ref(task4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    BOOST_TEST(exactly_eight_instances_created);
}

////////////////////////////////////////////////////////////////////////////////
static simple global_simple1_p;

struct single_global_grammar_object_task
{
    void operator()() const
    {
        make_definition3(global_simple1_p);
    };
};

static void
single_global_grammar_object_multiple_threads()
{
    // check wether exactly one definition per thread is
    // created
    count_guard guard(simple_definition_count);
    single_global_grammar_object_task task1, task2, task3, task4;

    boost::thread t1(callable_ref(task1));
    boost::thread t2(callable_ref(task2));
    boost::thread t3(callable_ref(task3));
    boost::thread t4(callable_ref(task4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    BOOST_TEST(exactly_four_instances_created);
}

////////////////////////////////////////////////////////////////////////////////
static simple global_simple2_p;
static simple global_simple3_p;

struct multiple_global_grammar_objects_task
{
    void operator()() const
    {
        make_definition3(global_simple2_p);
        make_definition3(global_simple3_p);
    };
};

static void
multiple_global_grammar_objects_multiple_threads()
{
    // check wether exactly one definition per thread 
    // and per grammar object is created
    count_guard guard(simple_definition_count);
    multiple_global_grammar_objects_task task1, task2, task3, task4;

    boost::thread t1(callable_ref(task1));
    boost::thread t2(callable_ref(task2));
    boost::thread t3(callable_ref(task3));
    boost::thread t4(callable_ref(task4));

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    BOOST_TEST(exactly_eight_instances_created);
}
////////////////////////////////////////////////////////////////////////////////
int
main()
{
    multiple_attempts_to_instantiate_a_definition_from_a_single_thread();
    single_local_grammar_object_multiple_threads();
    multiple_local_grammar_objects_multiple_threads();
    single_global_grammar_object_multiple_threads();
    multiple_global_grammar_objects_multiple_threads();

    return boost::report_errors();
}

////////////////////////////////////////////////////////////////////////////////

static BOOST_SPIRIT_CLASSIC_NS::parse_info<char const *> pi;

////////////////////////////////////////////////
// These macros are used with BOOST_TEST



#endif // MT mode

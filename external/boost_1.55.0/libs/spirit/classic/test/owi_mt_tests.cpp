/*=============================================================================
    Copyright (c) 2002-2004 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// std::lower_bound seems to perform awfully slow with _GLIBCXX_DEBUG enabled
#undef _GLIBCXX_DEBUG

#include <iostream>
#include <boost/config.hpp>
#include <boost/detail/lightweight_test.hpp>

#if !defined(BOOST_HAS_THREADS) || defined(DONT_HAVE_BOOST) || defined(BOOST_DISABLE_THREADS)
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

////////////////////////////////////////////////////////////////////////////////

static const unsigned long initial_test_size = 5000UL;
#if defined(_DEBUG) && (BOOST_MSVC >= 1400)
static const unsigned long maximum_test_size = 10000UL;
#else
static const unsigned long maximum_test_size = 1000000UL;
#endif

////////////////////////////////////////////////////////////////////////////////

#undef BOOST_SPIRIT_THREADSAFE
#define BOOST_SPIRIT_THREADSAFE

#include <boost/thread/thread.hpp>
#include <boost/spirit/home/classic/core/non_terminal/impl/object_with_id.ipp>
#include <boost/ref.hpp>
#include <boost/thread/xtime.hpp>
#include <vector>
#include <algorithm>
#include <boost/detail/lightweight_test.hpp>

using BOOST_SPIRIT_CLASSIC_NS::impl::object_with_id;

struct tag1 {};
typedef object_with_id<tag1> class1;

unsigned long test_size = initial_test_size;
boost::xtime start_time;

template <typename ClassT>
struct test_task
{
    test_task() : v(), m(), progress(0) {}

    void operator ()()
    {   // create lots of objects
        unsigned long i = 0;

        v.reserve(maximum_test_size);
        do
        {
            for (; i<test_size; ++i)
                v.push_back(new ClassT);
        }
        while ( i < increase_test_size(i) );
    }

    static unsigned long
    increase_test_size(unsigned long size)
    {
        static boost::mutex  m;
        boost::mutex::scoped_lock l(m);

        if (size<test_size || test_size == maximum_test_size)
            return test_size;

        boost::xtime now;
        boost::xtime_get(&now, boost::TIME_UTC_);
        unsigned long seconds = now.sec - start_time.sec;
        if (seconds < 4)
        {
            test_size *= 2;
            if (test_size > maximum_test_size)
                test_size = maximum_test_size;
        }

        return test_size;
    }

    std::vector<ClassT*> const &data() const
    {
        return v;
    }

private:
    std::vector<ClassT*> v;
    boost::mutex         m;
    unsigned int         progress;
};

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

test_task<class1> test1;
test_task<class1> test2;
test_task<class1> test3;


template <typename ClassT>
void
check_ascending(test_task<ClassT> const &t)
{
    typedef typename std::vector<ClassT*>::const_iterator iter;
    iter p(t.data().begin());
    iter const e(t.data().end());
    iter n(p);

    while (++n!=e)
    {
        if ((**n).get_object_id()<=(**p).get_object_id())
        {
            using namespace std;
            throw std::runtime_error("object ids out of order");
        }
        p = n;
    }
}

struct less1
{
    bool operator()(class1 const *p, class1 const *q) const
    {
        return p->get_object_id() < q->get_object_id();
    }
};

template <typename ClassT>
void
check_not_contained_in(
    test_task<ClassT> const &candidate,
    test_task<ClassT> const &in
)
{
    typedef typename std::vector<ClassT*>::const_iterator iter;
    iter p(candidate.data().begin());
    iter const e(candidate.data().end());

    while (p!=e)
    {
        iter found = std::lower_bound(in.data().begin(),in.data().end(),*p,less1());
        if  (found!=in.data().end() &&
            (**found).get_object_id() == (**p).get_object_id())
        {
            using namespace std;
            throw std::runtime_error("object ids not unqiue");
        }
        ++p;
    }
}

void concurrent_creation_of_objects()
{
    {
        boost::xtime_get(&start_time, boost::TIME_UTC_);
        boost::thread thread1(callable_ref(test1));
        boost::thread thread2(callable_ref(test2));
        boost::thread thread3(callable_ref(test3));

        thread1.join();
        thread2.join();
        thread3.join();
    }
}

void local_uniqueness()
{


    BOOST_TEST(test1.data().size()==test_size);
    BOOST_TEST(test2.data().size()==test_size);
    BOOST_TEST(test3.data().size()==test_size);
}

void local_ordering_and_uniqueness()
{
    // now all objects should have unique ids,
    // the ids must be ascending within each vector
    // check for ascending ids
    check_ascending(test1);
    check_ascending(test2);
    check_ascending(test3);
}

void global_uniqueness()
{
    check_not_contained_in(test1,test3);
    check_not_contained_in(test1,test2);
    check_not_contained_in(test2,test1);
    check_not_contained_in(test2,test3);
    check_not_contained_in(test3,test2);
    check_not_contained_in(test3,test1);
}

int
main()
{
    concurrent_creation_of_objects();
    local_ordering_and_uniqueness();
    global_uniqueness();
    return boost::report_errors();
}

#endif // BOOST_HAS_THREADS


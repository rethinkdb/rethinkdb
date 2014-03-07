/*
 *             Copyright Andrey Semashev 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   intrusive_ref_counter_test.cpp
 * \author Andrey Semashev
 * \date   31.08.2013
 *
 * This file contains tests for the \c intrusive_ref_counter base class.
 */

#include <boost/config.hpp>

#if defined(BOOST_MSVC)

#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#pragma warning(disable: 4355)  // 'this' : used in base member initializer list
#pragma warning(disable: 4511)  // copy constructor could not be generated
#pragma warning(disable: 4512)  // assignment operator could not be generated

#if (BOOST_MSVC >= 1310)
#pragma warning(disable: 4675)  // resolved overload found with Koenig lookup
#endif

#endif

#include <cstddef>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace N1 {

class my_class :
    public boost::intrusive_ref_counter< my_class >
{
public:
    static unsigned int destructor_count;

    ~my_class()
    {
        ++destructor_count;
    }
};

unsigned int my_class::destructor_count = 0;

} // namespace N1

namespace N2 {

class my_class :
    public boost::intrusive_ref_counter< my_class, boost::thread_unsafe_counter >
{
public:
    static unsigned int destructor_count;

    ~my_class()
    {
        ++destructor_count;
    }
};

unsigned int my_class::destructor_count = 0;

} // namespace N2

namespace N3 {

struct root :
    public boost::intrusive_ref_counter< root >
{
    virtual ~root() {}
};

} // namespace N3

namespace N4 {

struct X :
    public virtual N3::root
{
};

} // namespace N4

namespace N5 {

struct Y :
    public virtual N3::root
{
};

} // namespace N5

namespace N6 {

struct Z :
    public N4::X,
    public N5::Y
{
    static unsigned int destructor_count;

    ~Z()
    {
        ++destructor_count;
    }
};

unsigned int Z::destructor_count = 0;

} // namespace N6


int main()
{
    // The test check that ADL works
    {
        boost::intrusive_ptr< N1::my_class > p = new N1::my_class();
        p = NULL;
        BOOST_TEST(N1::my_class::destructor_count == 1);
    }
    {
        boost::intrusive_ptr< N2::my_class > p = new N2::my_class();
        p = NULL;
        BOOST_TEST(N2::my_class::destructor_count == 1);
    }
    {
        N1::my_class* p = new N1::my_class();
        intrusive_ptr_add_ref(p);
        intrusive_ptr_release(p);
        BOOST_TEST(N1::my_class::destructor_count == 2);
    }

    // The test checks that destroying through the base class works
    {
        boost::intrusive_ptr< N6::Z > p1 = new N6::Z();
        BOOST_TEST(p1->use_count() == 1);
        BOOST_TEST(N6::Z::destructor_count == 0);
        boost::intrusive_ptr< N3::root > p2 = p1;
        BOOST_TEST(p1->use_count() == 2);
        BOOST_TEST(N6::Z::destructor_count == 0);
        p1 = NULL;
        BOOST_TEST(N6::Z::destructor_count == 0);
        p2 = NULL;
        BOOST_TEST(N6::Z::destructor_count == 1);
    }

    return boost::report_errors();
}

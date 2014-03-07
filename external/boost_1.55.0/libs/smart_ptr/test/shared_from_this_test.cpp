#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  shared_from_this_test.cpp
//
//  Copyright (c) 2002, 2003 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/detail/lightweight_test.hpp>

//

class X
{
public:

    virtual void f() = 0;

protected:

    ~X() {}
};

class Y
{
public:

    virtual boost::shared_ptr<X> getX() = 0;

protected:

    ~Y() {}
};

boost::shared_ptr<Y> createY();

void test()
{
    boost::shared_ptr<Y> py = createY();
    BOOST_TEST(py.get() != 0);
    BOOST_TEST(py.use_count() == 1);

    try
    {
        boost::shared_ptr<X> px = py->getX();
        BOOST_TEST(px.get() != 0);
        BOOST_TEST(py.use_count() == 2);

        px->f();

#if !defined( BOOST_NO_RTTI )
        boost::shared_ptr<Y> py2 = boost::dynamic_pointer_cast<Y>(px);
        BOOST_TEST(py.get() == py2.get());
        BOOST_TEST(!(py < py2 || py2 < py));
        BOOST_TEST(py.use_count() == 3);
#endif
    }
    catch( boost::bad_weak_ptr const& )
    {
        BOOST_ERROR( "py->getX() failed" );
    }
}

void test2();
void test3();

int main()
{
    test();
    test2();
    test3();
    return boost::report_errors();
}

// virtual inheritance to stress the implementation
// (prevents Y* -> impl*, enable_shared_from_this<impl>* -> impl* casts)

class impl: public X, public virtual Y, public virtual boost::enable_shared_from_this<impl>
{
public:

    virtual void f()
    {
    }

    virtual boost::shared_ptr<X> getX()
    {
        boost::shared_ptr<impl> pi = shared_from_this();
        BOOST_TEST(pi.get() == this);
        return pi;
    }
};

// intermediate impl2 to stress the implementation

class impl2: public impl
{
};

boost::shared_ptr<Y> createY()
{
    boost::shared_ptr<Y> pi(new impl2);
    return pi;
}

void test2()
{
    boost::shared_ptr<Y> pi(static_cast<impl2*>(0));
}

//

struct V: public boost::enable_shared_from_this<V>
{
};

void test3()
{
    boost::shared_ptr<V> p(new V);

    try
    {
        boost::shared_ptr<V> q = p->shared_from_this();
        BOOST_TEST(p == q);
        BOOST_TEST(!(p < q) && !(q < p));
    }
    catch( boost::bad_weak_ptr const & )
    {
        BOOST_ERROR( "p->shared_from_this() failed" );
    }

    V v2(*p);

    try
    {
        boost::shared_ptr<V> r = v2.shared_from_this();
        BOOST_ERROR("v2.shared_from_this() failed to throw");
    }
    catch( boost::bad_weak_ptr const & )
    {
    }

    try
    {
        *p = V();
        boost::shared_ptr<V> r = p->shared_from_this();
        BOOST_TEST(p == r);
        BOOST_TEST(!(p < r) && !(r < p));
    }
    catch( boost::bad_weak_ptr const & )
    {
        BOOST_ERROR("p->shared_from_this() threw bad_weak_ptr after *p = V()");
    }
}

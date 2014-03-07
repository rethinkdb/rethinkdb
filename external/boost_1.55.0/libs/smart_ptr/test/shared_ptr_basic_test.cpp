#include <boost/config.hpp>

#if defined(BOOST_MSVC)

#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#pragma warning(disable: 4355)  // 'this' : used in base member initializer list

#if (BOOST_MSVC >= 1310)
#pragma warning(disable: 4675)  // resolved overload found with Koenig lookup
#endif

#endif

//
//  shared_ptr_basic_test.cpp
//
//  Copyright (c) 2001, 2002 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/detail/lightweight_test.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

int cnt = 0;

struct X
{
    X()
    {
        ++cnt;
    }

    ~X() // virtual destructor deliberately omitted
    {
        --cnt;
    }

    virtual int id() const
    {
        return 1;
    }

private:

    X(X const &);
    X & operator= (X const &);
};

struct Y: public X
{
    Y()
    {
        ++cnt;
    }

    ~Y()
    {
        --cnt;
    }

    virtual int id() const
    {
        return 2;
    }

private:

    Y(Y const &);
    Y & operator= (Y const &);
};

int * get_object()
{
    ++cnt;
    return &cnt;
}

void release_object(int * p)
{
    BOOST_TEST(p == &cnt);
    --cnt;
}

template<class T> void test_is_X(boost::shared_ptr<T> const & p)
{
    BOOST_TEST(p->id() == 1);
    BOOST_TEST((*p).id() == 1);
}

template<class T> void test_is_X(boost::weak_ptr<T> const & p)
{
    BOOST_TEST(p.get() != 0);
    BOOST_TEST(p.get()->id() == 1);
}

template<class T> void test_is_Y(boost::shared_ptr<T> const & p)
{
    BOOST_TEST(p->id() == 2);
    BOOST_TEST((*p).id() == 2);
}

template<class T> void test_is_Y(boost::weak_ptr<T> const & p)
{
    boost::shared_ptr<T> q = p.lock();
    BOOST_TEST(q.get() != 0);
    BOOST_TEST(q->id() == 2);
}

template<class T> void test_eq(T const & a, T const & b)
{
    BOOST_TEST(a == b);
    BOOST_TEST(!(a != b));
    BOOST_TEST(!(a < b));
    BOOST_TEST(!(b < a));
}

template<class T> void test_ne(T const & a, T const & b)
{
    BOOST_TEST(!(a == b));
    BOOST_TEST(a != b);
    BOOST_TEST(a < b || b < a);
    BOOST_TEST(!(a < b && b < a));
}

template<class T, class U> void test_shared(boost::weak_ptr<T> const & a, boost::weak_ptr<U> const & b)
{
    BOOST_TEST(!(a < b));
    BOOST_TEST(!(b < a));
}

template<class T, class U> void test_nonshared(boost::weak_ptr<T> const & a, boost::weak_ptr<U> const & b)
{
    BOOST_TEST(a < b || b < a);
    BOOST_TEST(!(a < b && b < a));
}

template<class T, class U> void test_eq2(T const & a, U const & b)
{
    BOOST_TEST(a == b);
    BOOST_TEST(!(a != b));
}

template<class T, class U> void test_ne2(T const & a, U const & b)
{
    BOOST_TEST(!(a == b));
    BOOST_TEST(a != b);
}

template<class T> void test_is_zero(boost::shared_ptr<T> const & p)
{
    BOOST_TEST(!p);
    BOOST_TEST(p.get() == 0);
}

template<class T> void test_is_nonzero(boost::shared_ptr<T> const & p)
{
    // p? true: false is used to test p in a boolean context.
    // BOOST_TEST(p) is not guaranteed to test the conversion,
    // as the macro might test !!p instead.
    BOOST_TEST(p? true: false);
    BOOST_TEST(p.get() != 0);
}

int main()
{
    using namespace boost;

    {
        shared_ptr<X> p(new Y);
        shared_ptr<X> p2(new X);

        test_is_nonzero(p);
        test_is_nonzero(p2);
        test_is_Y(p);
        test_is_X(p2);
        test_ne(p, p2);

        {
            shared_ptr<X> q(p);
            test_eq(p, q);
        }

#if !defined( BOOST_NO_RTTI )
        shared_ptr<Y> p3 = dynamic_pointer_cast<Y>(p);
        shared_ptr<Y> p4 = dynamic_pointer_cast<Y>(p2);

        test_is_nonzero(p3);
        test_is_zero(p4);

        BOOST_TEST(p.use_count() == 2);
        BOOST_TEST(p2.use_count() == 1);
        BOOST_TEST(p3.use_count() == 2);

        test_is_Y(p3);
        test_eq2(p, p3);
        test_ne2(p2, p4);
#endif

        shared_ptr<void> p5(p);

        test_is_nonzero(p5);
        test_eq2(p, p5);

        weak_ptr<X> wp1(p2);

        BOOST_TEST(!wp1.expired());
        BOOST_TEST(wp1.use_count() != 0);

        p.reset();
        p2.reset();
#if !defined( BOOST_NO_RTTI )
        p3.reset();
        p4.reset();
#endif

        test_is_zero(p);
        test_is_zero(p2);
#if !defined( BOOST_NO_RTTI )
        test_is_zero(p3);
        test_is_zero(p4);
#endif

        BOOST_TEST(p5.use_count() == 1);

        BOOST_TEST(wp1.expired());
        BOOST_TEST(wp1.use_count() == 0);

        try
        {
            shared_ptr<X> sp1(wp1);
            BOOST_ERROR("shared_ptr<X> sp1(wp1) failed to throw");
        }
        catch(boost::bad_weak_ptr const &)
        {
        }

        test_is_zero(wp1.lock());

        weak_ptr<X> wp2 = static_pointer_cast<X>(p5);

        BOOST_TEST(wp2.use_count() == 1);
        test_is_Y(wp2);
        test_nonshared(wp1, wp2);

        // Scoped to not affect the subsequent use_count() tests.
        {
            shared_ptr<X> sp2(wp2);
            test_is_nonzero(wp2.lock());
        }

#if !defined( BOOST_NO_RTTI )
        weak_ptr<Y> wp3 = dynamic_pointer_cast<Y>(wp2.lock());

        BOOST_TEST(wp3.use_count() == 1);
        test_shared(wp2, wp3);

        weak_ptr<X> wp4(wp3);

        BOOST_TEST(wp4.use_count() == 1);
        test_shared(wp2, wp4);
#endif

        wp1 = p2;
        test_is_zero(wp1.lock());

#if !defined( BOOST_NO_RTTI )
        wp1 = p4;
        wp1 = wp3;
#endif
        wp1 = wp2;

        BOOST_TEST(wp1.use_count() == 1);
        test_shared(wp1, wp2);

        weak_ptr<X> wp5;

        bool b1 = wp1 < wp5;
        bool b2 = wp5 < wp1;

        p5.reset();

        BOOST_TEST(wp1.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);
#if !defined( BOOST_NO_RTTI )
        BOOST_TEST(wp3.use_count() == 0);
#endif

        // Test operator< stability for std::set< weak_ptr<> >
        // Thanks to Joe Gottman for pointing this out

        BOOST_TEST(b1 == (wp1 < wp5));
        BOOST_TEST(b2 == (wp5 < wp1));

        {
            // note that both get_object and release_object deal with int*
            shared_ptr<void> p6(get_object(), release_object);
        }
    }

    BOOST_TEST(cnt == 0);

    return boost::report_errors();
}

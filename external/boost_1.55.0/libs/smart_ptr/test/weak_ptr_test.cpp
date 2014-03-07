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
//  weak_ptr_test.cpp
//
//  Copyright (c) 2002-2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/detail/lightweight_test.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <map>
#include <vector>

//

namespace n_element_type
{

void f(int &)
{
}

void test()
{
    typedef boost::weak_ptr<int>::element_type T;
    T t;
    f(t);
}

} // namespace n_element_type

class incomplete;

boost::shared_ptr<incomplete> create_incomplete();

struct X
{
    int dummy;
};

struct Y
{
    int dummy2;
};

struct Z: public X, public virtual Y
{
};

namespace n_constructors
{

void default_constructor()
{
    {
        boost::weak_ptr<int> wp;
        BOOST_TEST(wp.use_count() == 0);
    }

    {
        boost::weak_ptr<void> wp;
        BOOST_TEST(wp.use_count() == 0);
    }

    {
        boost::weak_ptr<incomplete> wp;
        BOOST_TEST(wp.use_count() == 0);
    }
}

void shared_ptr_constructor()
{
    {
        boost::shared_ptr<int> sp;

        boost::weak_ptr<int> wp(sp);
        BOOST_TEST(wp.use_count() == sp.use_count());

        boost::weak_ptr<void> wp2(sp);
        BOOST_TEST(wp2.use_count() == sp.use_count());
    }

    {
        boost::shared_ptr<int> sp(static_cast<int*>(0));

        {
            boost::weak_ptr<int> wp(sp);
            BOOST_TEST(wp.use_count() == sp.use_count());
            BOOST_TEST(wp.use_count() == 1);
            boost::shared_ptr<int> sp2(wp);
            BOOST_TEST(wp.use_count() == 2);
            BOOST_TEST(!(sp < sp2 || sp2 < sp));
        }

        {
            boost::weak_ptr<void> wp(sp);
            BOOST_TEST(wp.use_count() == sp.use_count());
            BOOST_TEST(wp.use_count() == 1);
            boost::shared_ptr<void> sp2(wp);
            BOOST_TEST(wp.use_count() == 2);
            BOOST_TEST(!(sp < sp2 || sp2 < sp));
        }
    }

    {
        boost::shared_ptr<int> sp(new int);

        {
            boost::weak_ptr<int> wp(sp);
            BOOST_TEST(wp.use_count() == sp.use_count());
            BOOST_TEST(wp.use_count() == 1);
            boost::shared_ptr<int> sp2(wp);
            BOOST_TEST(wp.use_count() == 2);
            BOOST_TEST(!(sp < sp2 || sp2 < sp));
        }

        {
            boost::weak_ptr<void> wp(sp);
            BOOST_TEST(wp.use_count() == sp.use_count());
            BOOST_TEST(wp.use_count() == 1);
            boost::shared_ptr<void> sp2(wp);
            BOOST_TEST(wp.use_count() == 2);
            BOOST_TEST(!(sp < sp2 || sp2 < sp));
        }
    }

    {
        boost::shared_ptr<void> sp;

        boost::weak_ptr<void> wp(sp);
        BOOST_TEST(wp.use_count() == sp.use_count());
    }

    {
        boost::shared_ptr<void> sp(static_cast<int*>(0));

        boost::weak_ptr<void> wp(sp);
        BOOST_TEST(wp.use_count() == sp.use_count());
        BOOST_TEST(wp.use_count() == 1);
        boost::shared_ptr<void> sp2(wp);
        BOOST_TEST(wp.use_count() == 2);
        BOOST_TEST(!(sp < sp2 || sp2 < sp));
    }

    {
        boost::shared_ptr<void> sp(new int);

        boost::weak_ptr<void> wp(sp);
        BOOST_TEST(wp.use_count() == sp.use_count());
        BOOST_TEST(wp.use_count() == 1);
        boost::shared_ptr<void> sp2(wp);
        BOOST_TEST(wp.use_count() == 2);
        BOOST_TEST(!(sp < sp2 || sp2 < sp));
    }

    {
        boost::shared_ptr<incomplete> sp;

        boost::weak_ptr<incomplete> wp(sp);
        BOOST_TEST(wp.use_count() == sp.use_count());

        boost::weak_ptr<void> wp2(sp);
        BOOST_TEST(wp2.use_count() == sp.use_count());
    }

    {
        boost::shared_ptr<incomplete> sp = create_incomplete();

        {
            boost::weak_ptr<incomplete> wp(sp);
            BOOST_TEST(wp.use_count() == sp.use_count());
            BOOST_TEST(wp.use_count() == 1);
            boost::shared_ptr<incomplete> sp2(wp);
            BOOST_TEST(wp.use_count() == 2);
            BOOST_TEST(!(sp < sp2 || sp2 < sp));
        }

        {
            boost::weak_ptr<void> wp(sp);
            BOOST_TEST(wp.use_count() == sp.use_count());
            BOOST_TEST(wp.use_count() == 1);
            boost::shared_ptr<void> sp2(wp);
            BOOST_TEST(wp.use_count() == 2);
            BOOST_TEST(!(sp < sp2 || sp2 < sp));
        }
    }

    {
        boost::shared_ptr<void> sp = create_incomplete();

        boost::weak_ptr<void> wp(sp);
        BOOST_TEST(wp.use_count() == sp.use_count());
        BOOST_TEST(wp.use_count() == 1);
        boost::shared_ptr<void> sp2(wp);
        BOOST_TEST(wp.use_count() == 2);
        BOOST_TEST(!(sp < sp2 || sp2 < sp));
    }
}

void copy_constructor()
{
    {
        boost::weak_ptr<int> wp;
        boost::weak_ptr<int> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 0);
    }

    {
        boost::weak_ptr<void> wp;
        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 0);
    }

    {
        boost::weak_ptr<incomplete> wp;
        boost::weak_ptr<incomplete> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 0);
    }

    {
        boost::shared_ptr<int> sp(static_cast<int*>(0));
        boost::weak_ptr<int> wp(sp);

        boost::weak_ptr<int> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<int> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<int> sp(new int);
        boost::weak_ptr<int> wp(sp);

        boost::weak_ptr<int> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<int> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<void> sp(static_cast<int*>(0));
        boost::weak_ptr<void> wp(sp);

        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<void> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<void> sp(new int);
        boost::weak_ptr<void> wp(sp);

        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<void> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<incomplete> sp = create_incomplete();
        boost::weak_ptr<incomplete> wp(sp);

        boost::weak_ptr<incomplete> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<incomplete> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }
}

void conversion_constructor()
{
    {
        boost::weak_ptr<int> wp;
        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 0);
    }

    {
        boost::weak_ptr<incomplete> wp;
        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 0);
    }

    {
        boost::weak_ptr<Z> wp;

        boost::weak_ptr<X> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 0);

        boost::weak_ptr<Y> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
    }

    {
        boost::shared_ptr<int> sp(static_cast<int*>(0));
        boost::weak_ptr<int> wp(sp);

        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<void> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<int> sp(new int);
        boost::weak_ptr<int> wp(sp);

        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<void> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<incomplete> sp = create_incomplete();
        boost::weak_ptr<incomplete> wp(sp);

        boost::weak_ptr<void> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<void> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<Z> sp(static_cast<Z*>(0));
        boost::weak_ptr<Z> wp(sp);

        boost::weak_ptr<X> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<X> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<Z> sp(static_cast<Z*>(0));
        boost::weak_ptr<Z> wp(sp);

        boost::weak_ptr<Y> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<Y> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<Z> sp(new Z);
        boost::weak_ptr<Z> wp(sp);

        boost::weak_ptr<X> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<X> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }

    {
        boost::shared_ptr<Z> sp(new Z);
        boost::weak_ptr<Z> wp(sp);

        boost::weak_ptr<Y> wp2(wp);
        BOOST_TEST(wp2.use_count() == wp.use_count());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        sp.reset();
        BOOST_TEST(!(wp < wp2 || wp2 < wp));

        boost::weak_ptr<Y> wp3(wp);
        BOOST_TEST(wp3.use_count() == wp.use_count());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));
    }
}

void test()
{
    default_constructor();
    shared_ptr_constructor();
    copy_constructor();
    conversion_constructor();
}

} // namespace n_constructors

namespace n_assignment
{

template<class T> void copy_assignment(boost::shared_ptr<T> & sp)
{
    BOOST_TEST(sp.unique());

    boost::weak_ptr<T> p1;

    p1 = p1;
    BOOST_TEST(p1.use_count() == 0);

    boost::weak_ptr<T> p2;

    p1 = p2;
    BOOST_TEST(p1.use_count() == 0);

    boost::weak_ptr<T> p3(p1);

    p1 = p3;
    BOOST_TEST(p1.use_count() == 0);

    boost::weak_ptr<T> p4(sp);

    p4 = p4;
    BOOST_TEST(p4.use_count() == 1);

    p1 = p4;
    BOOST_TEST(p1.use_count() == 1);

    p4 = p2;
    BOOST_TEST(p4.use_count() == 0);

    sp.reset();

    p1 = p1;
    BOOST_TEST(p1.use_count() == 0);

    p4 = p1;
    BOOST_TEST(p4.use_count() == 0);
}

void conversion_assignment()
{
    {
        boost::weak_ptr<void> p1;

        boost::weak_ptr<incomplete> p2;

        p1 = p2;
        BOOST_TEST(p1.use_count() == 0);

        boost::shared_ptr<incomplete> sp = create_incomplete();
        boost::weak_ptr<incomplete> p3(sp);

        p1 = p3;
        BOOST_TEST(p1.use_count() == 1);

        sp.reset();

        p1 = p3;
        BOOST_TEST(p1.use_count() == 0);

        p1 = p2;
        BOOST_TEST(p1.use_count() == 0);
    }

    {
        boost::weak_ptr<X> p1;

        boost::weak_ptr<Z> p2;

        p1 = p2;
        BOOST_TEST(p1.use_count() == 0);

        boost::shared_ptr<Z> sp(new Z);
        boost::weak_ptr<Z> p3(sp);

        p1 = p3;
        BOOST_TEST(p1.use_count() == 1);

        sp.reset();

        p1 = p3;
        BOOST_TEST(p1.use_count() == 0);

        p1 = p2;
        BOOST_TEST(p1.use_count() == 0);
    }

    {
        boost::weak_ptr<Y> p1;

        boost::weak_ptr<Z> p2;

        p1 = p2;
        BOOST_TEST(p1.use_count() == 0);

        boost::shared_ptr<Z> sp(new Z);
        boost::weak_ptr<Z> p3(sp);

        p1 = p3;
        BOOST_TEST(p1.use_count() == 1);

        sp.reset();

        p1 = p3;
        BOOST_TEST(p1.use_count() == 0);

        p1 = p2;
        BOOST_TEST(p1.use_count() == 0);
    }
}

template<class T, class U> void shared_ptr_assignment(boost::shared_ptr<U> & sp, T * = 0)
{
    BOOST_TEST(sp.unique());

    boost::weak_ptr<T> p1;
    boost::weak_ptr<T> p2(p1);
    boost::weak_ptr<T> p3(sp);
    boost::weak_ptr<T> p4(p3);

    p1 = sp;
    BOOST_TEST(p1.use_count() == 1);

    p2 = sp;
    BOOST_TEST(p2.use_count() == 1);

    p3 = sp;
    BOOST_TEST(p3.use_count() == 1);

    p4 = sp;
    BOOST_TEST(p4.use_count() == 1);

    sp.reset();

    BOOST_TEST(p1.use_count() == 0);
    BOOST_TEST(p2.use_count() == 0);
    BOOST_TEST(p3.use_count() == 0);
    BOOST_TEST(p4.use_count() == 0);

    p1 = sp;
}

void test()
{
    {
        boost::shared_ptr<int> p( new int );
        copy_assignment( p );
    }

    {
        boost::shared_ptr<X> p( new X );
        copy_assignment( p );
    }

    {
        boost::shared_ptr<void> p( new int );
        copy_assignment( p );
    }

    {
        boost::shared_ptr<incomplete> p = create_incomplete();
        copy_assignment( p );
    }

    conversion_assignment();

    {
        boost::shared_ptr<int> p( new int );
        shared_ptr_assignment<int>( p );
    }

    {
        boost::shared_ptr<int> p( new int );
        shared_ptr_assignment<void>( p );
    }

    {
        boost::shared_ptr<X> p( new X );
        shared_ptr_assignment<X>( p );
    }

    {
        boost::shared_ptr<X> p( new X );
        shared_ptr_assignment<void>( p );
    }

    {
        boost::shared_ptr<void> p( new int );
        shared_ptr_assignment<void>( p );
    }

    {
        boost::shared_ptr<incomplete> p = create_incomplete();
        shared_ptr_assignment<incomplete>( p );
    }

    {
        boost::shared_ptr<incomplete> p = create_incomplete();
        shared_ptr_assignment<void>( p );
    }
}

} // namespace n_assignment

namespace n_reset
{

template<class T, class U> void test2( boost::shared_ptr<U> & sp, T * = 0 )
{
    BOOST_TEST(sp.unique());

    boost::weak_ptr<T> p1;
    boost::weak_ptr<T> p2(p1);
    boost::weak_ptr<T> p3(sp);
    boost::weak_ptr<T> p4(p3);
    boost::weak_ptr<T> p5(sp);
    boost::weak_ptr<T> p6(p5);

    p1.reset();
    BOOST_TEST(p1.use_count() == 0);

    p2.reset();
    BOOST_TEST(p2.use_count() == 0);

    p3.reset();
    BOOST_TEST(p3.use_count() == 0);

    p4.reset();
    BOOST_TEST(p4.use_count() == 0);

    sp.reset();

    p5.reset();
    BOOST_TEST(p5.use_count() == 0);

    p6.reset();
    BOOST_TEST(p6.use_count() == 0);
}

void test()
{
    {
        boost::shared_ptr<int> p( new int );
        test2<int>( p );
    }

    {
        boost::shared_ptr<int> p( new int );
        test2<void>( p );
    }

    {
        boost::shared_ptr<X> p( new X );
        test2<X>( p );
    }

    {
        boost::shared_ptr<X> p( new X );
        test2<void>( p );
    }

    {
        boost::shared_ptr<void> p( new int );
        test2<void>( p );
    }

    {
        boost::shared_ptr<incomplete> p = create_incomplete();
        test2<incomplete>( p );
    }

    {
        boost::shared_ptr<incomplete> p = create_incomplete();
        test2<void>( p );
    }
}

} // namespace n_reset

namespace n_use_count
{

void test()
{
    {
        boost::weak_ptr<X> wp;
        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp.expired());

        boost::weak_ptr<X> wp2;
        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp.expired());

        boost::weak_ptr<X> wp3(wp);
        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp.expired());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(wp3.expired());
    }

    {
        boost::shared_ptr<X> sp(static_cast<X*>(0));

        boost::weak_ptr<X> wp(sp);
        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(!wp.expired());

        boost::weak_ptr<X> wp2(sp);
        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(!wp.expired());

        boost::weak_ptr<X> wp3(wp);
        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(!wp.expired());
        BOOST_TEST(wp3.use_count() == 1);
        BOOST_TEST(!wp3.expired());

        boost::shared_ptr<X> sp2(sp);

        BOOST_TEST(wp.use_count() == 2);
        BOOST_TEST(!wp.expired());
        BOOST_TEST(wp2.use_count() == 2);
        BOOST_TEST(!wp2.expired());
        BOOST_TEST(wp3.use_count() == 2);
        BOOST_TEST(!wp3.expired());

        boost::shared_ptr<void> sp3(sp);

        BOOST_TEST(wp.use_count() == 3);
        BOOST_TEST(!wp.expired());
        BOOST_TEST(wp2.use_count() == 3);
        BOOST_TEST(!wp2.expired());
        BOOST_TEST(wp3.use_count() == 3);
        BOOST_TEST(!wp3.expired());

        sp.reset();

        BOOST_TEST(wp.use_count() == 2);
        BOOST_TEST(!wp.expired());
        BOOST_TEST(wp2.use_count() == 2);
        BOOST_TEST(!wp2.expired());
        BOOST_TEST(wp3.use_count() == 2);
        BOOST_TEST(!wp3.expired());

        sp2.reset();

        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(!wp.expired());
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!wp2.expired());
        BOOST_TEST(wp3.use_count() == 1);
        BOOST_TEST(!wp3.expired());

        sp3.reset();

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp.expired());
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(wp2.expired());
        BOOST_TEST(wp3.use_count() == 0);
        BOOST_TEST(wp3.expired());
    }
}

} // namespace n_use_count

namespace n_swap
{

void test()
{
    {
        boost::weak_ptr<X> wp;
        boost::weak_ptr<X> wp2;

        wp.swap(wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);

        using std::swap;
        swap(wp, wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);
    }

    {
        boost::shared_ptr<X> sp(new X);
        boost::weak_ptr<X> wp;
        boost::weak_ptr<X> wp2(sp);
        boost::weak_ptr<X> wp3(sp);

        wp.swap(wp2);

        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));

        using std::swap;
        swap(wp, wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));

        sp.reset();

        wp.swap(wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));

        swap(wp, wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));
    }

    {
        boost::shared_ptr<X> sp(new X);
        boost::shared_ptr<X> sp2(new X);
        boost::weak_ptr<X> wp(sp);
        boost::weak_ptr<X> wp2(sp2);
        boost::weak_ptr<X> wp3(sp2);

        wp.swap(wp2);

        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));

        using std::swap;
        swap(wp, wp2);

        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));

        sp.reset();

        wp.swap(wp2);

        BOOST_TEST(wp.use_count() == 1);
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));

        swap(wp, wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 1);
        BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));

        sp2.reset();

        wp.swap(wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(!(wp < wp3 || wp3 < wp));

        swap(wp, wp2);

        BOOST_TEST(wp.use_count() == 0);
        BOOST_TEST(wp2.use_count() == 0);
        BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));
    }
}

} // namespace n_swap

namespace n_comparison
{

void test()
{
    {
        boost::weak_ptr<X> wp;
        BOOST_TEST(!(wp < wp));

        boost::weak_ptr<X> wp2;
        BOOST_TEST(!(wp < wp2 && wp2 < wp));

        boost::weak_ptr<X> wp3(wp);
        BOOST_TEST(!(wp3 < wp3));
        BOOST_TEST(!(wp < wp3 && wp3 < wp));
    }

    {
        boost::shared_ptr<X> sp(new X);

        boost::weak_ptr<X> wp(sp);
        BOOST_TEST(!(wp < wp));

        boost::weak_ptr<X> wp2;
        BOOST_TEST(wp < wp2 || wp2 < wp);
        BOOST_TEST(!(wp < wp2 && wp2 < wp));

        bool b1 = wp < wp2;
        bool b2 = wp2 < wp;

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(wp2 < wp3 || wp3 < wp2);
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(wp4 < wp3 || wp3 < wp4);
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }

        sp.reset();

        BOOST_TEST(b1 == (wp < wp2));
        BOOST_TEST(b2 == (wp2 < wp));

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(wp2 < wp3 || wp3 < wp2);
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(wp4 < wp3 || wp3 < wp4);
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }
    }

    {
        boost::shared_ptr<X> sp(new X);
        boost::shared_ptr<X> sp2(new X);

        boost::weak_ptr<X> wp(sp);
        boost::weak_ptr<X> wp2(sp2);

        BOOST_TEST(wp < wp2 || wp2 < wp);
        BOOST_TEST(!(wp < wp2 && wp2 < wp));

        bool b1 = wp < wp2;
        bool b2 = wp2 < wp;

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(wp2 < wp3 || wp3 < wp2);
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(wp4 < wp3 || wp3 < wp4);
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }

        sp.reset();

        BOOST_TEST(b1 == (wp < wp2));
        BOOST_TEST(b2 == (wp2 < wp));

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(wp2 < wp3 || wp3 < wp2);
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(wp4 < wp3 || wp3 < wp4);
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }

        sp2.reset();

        BOOST_TEST(b1 == (wp < wp2));
        BOOST_TEST(b2 == (wp2 < wp));

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(wp2 < wp3 || wp3 < wp2);
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(wp4 < wp3 || wp3 < wp4);
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }
    }

    {
        boost::shared_ptr<X> sp(new X);
        boost::shared_ptr<X> sp2(sp);

        boost::weak_ptr<X> wp(sp);
        boost::weak_ptr<X> wp2(sp2);

        BOOST_TEST(!(wp < wp2 || wp2 < wp));
        BOOST_TEST(!(wp < wp2 && wp2 < wp));

        bool b1 = wp < wp2;
        bool b2 = wp2 < wp;

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(!(wp4 < wp3 || wp3 < wp4));
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }

        sp.reset();
        sp2.reset();

        BOOST_TEST(b1 == (wp < wp2));
        BOOST_TEST(b2 == (wp2 < wp));

        {
            boost::weak_ptr<X> wp3(wp);

            BOOST_TEST(!(wp < wp3 || wp3 < wp));
            BOOST_TEST(!(wp < wp3 && wp3 < wp));

            BOOST_TEST(!(wp2 < wp3 || wp3 < wp2));
            BOOST_TEST(!(wp2 < wp3 && wp3 < wp2));

            boost::weak_ptr<X> wp4(wp2);

            BOOST_TEST(!(wp4 < wp3 || wp3 < wp4));
            BOOST_TEST(!(wp4 < wp3 && wp3 < wp4));
        }
    }

    {
        boost::shared_ptr<X> spx(new X);
        boost::shared_ptr<Y> spy(new Y);
        boost::shared_ptr<Z> spz(new Z);

        boost::weak_ptr<X> px(spx);
        boost::weak_ptr<Y> py(spy);
        boost::weak_ptr<Z> pz(spz);

        BOOST_TEST(px < py || py < px);
        BOOST_TEST(px < pz || pz < px);
        BOOST_TEST(py < pz || pz < py);

        BOOST_TEST(!(px < py && py < px));
        BOOST_TEST(!(px < pz && pz < px));
        BOOST_TEST(!(py < pz && pz < py));

        boost::weak_ptr<void> pvx(px);
        BOOST_TEST(!(pvx < pvx));

        boost::weak_ptr<void> pvy(py);
        BOOST_TEST(!(pvy < pvy));

        boost::weak_ptr<void> pvz(pz);
        BOOST_TEST(!(pvz < pvz));

        BOOST_TEST(pvx < pvy || pvy < pvx);
        BOOST_TEST(pvx < pvz || pvz < pvx);
        BOOST_TEST(pvy < pvz || pvz < pvy);

        BOOST_TEST(!(pvx < pvy && pvy < pvx));
        BOOST_TEST(!(pvx < pvz && pvz < pvx));
        BOOST_TEST(!(pvy < pvz && pvz < pvy));

        spx.reset();
        spy.reset();
        spz.reset();

        BOOST_TEST(px < py || py < px);
        BOOST_TEST(px < pz || pz < px);
        BOOST_TEST(py < pz || pz < py);

        BOOST_TEST(!(px < py && py < px));
        BOOST_TEST(!(px < pz && pz < px));
        BOOST_TEST(!(py < pz && pz < py));

        BOOST_TEST(!(pvx < pvx));
        BOOST_TEST(!(pvy < pvy));
        BOOST_TEST(!(pvz < pvz));

        BOOST_TEST(pvx < pvy || pvy < pvx);
        BOOST_TEST(pvx < pvz || pvz < pvx);
        BOOST_TEST(pvy < pvz || pvz < pvy);

        BOOST_TEST(!(pvx < pvy && pvy < pvx));
        BOOST_TEST(!(pvx < pvz && pvz < pvx));
        BOOST_TEST(!(pvy < pvz && pvz < pvy));
    }

    {
        boost::shared_ptr<Z> spz(new Z);
        boost::shared_ptr<X> spx(spz);

        boost::weak_ptr<Z> pz(spz);
        boost::weak_ptr<X> px(spx);
        boost::weak_ptr<Y> py(spz);

        BOOST_TEST(!(px < px));
        BOOST_TEST(!(py < py));

        BOOST_TEST(!(px < py || py < px));
        BOOST_TEST(!(px < pz || pz < px));
        BOOST_TEST(!(py < pz || pz < py));

        boost::weak_ptr<void> pvx(px);
        boost::weak_ptr<void> pvy(py);
        boost::weak_ptr<void> pvz(pz);

        BOOST_TEST(!(pvx < pvy || pvy < pvx));
        BOOST_TEST(!(pvx < pvz || pvz < pvx));
        BOOST_TEST(!(pvy < pvz || pvz < pvy));

        spx.reset();
        spz.reset();

        BOOST_TEST(!(px < px));
        BOOST_TEST(!(py < py));

        BOOST_TEST(!(px < py || py < px));
        BOOST_TEST(!(px < pz || pz < px));
        BOOST_TEST(!(py < pz || pz < py));

        BOOST_TEST(!(pvx < pvy || pvy < pvx));
        BOOST_TEST(!(pvx < pvz || pvz < pvx));
        BOOST_TEST(!(pvy < pvz || pvz < pvy));
    }
}

} // namespace n_comparison

namespace n_lock
{

void test()
{
}

} // namespace n_lock

namespace n_map
{

void test()
{
    std::vector< boost::shared_ptr<int> > vi;

    {
        boost::shared_ptr<int> pi1(new int);
        boost::shared_ptr<int> pi2(new int);
        boost::shared_ptr<int> pi3(new int);

        vi.push_back(pi1);
        vi.push_back(pi1);
        vi.push_back(pi1);
        vi.push_back(pi2);
        vi.push_back(pi1);
        vi.push_back(pi2);
        vi.push_back(pi1);
        vi.push_back(pi3);
        vi.push_back(pi3);
        vi.push_back(pi2);
        vi.push_back(pi1);
    }

    std::vector< boost::shared_ptr<X> > vx;

    {
        boost::shared_ptr<X> px1(new X);
        boost::shared_ptr<X> px2(new X);
        boost::shared_ptr<X> px3(new X);

        vx.push_back(px2);
        vx.push_back(px2);
        vx.push_back(px1);
        vx.push_back(px2);
        vx.push_back(px1);
        vx.push_back(px1);
        vx.push_back(px1);
        vx.push_back(px2);
        vx.push_back(px1);
        vx.push_back(px3);
        vx.push_back(px2);
    }

    std::map< boost::weak_ptr<void>, long > m;

    {
        for(std::vector< boost::shared_ptr<int> >::iterator i = vi.begin(); i != vi.end(); ++i)
        {
            ++m[*i];
        }
    }

    {
        for(std::vector< boost::shared_ptr<X> >::iterator i = vx.begin(); i != vx.end(); ++i)
        {
            ++m[*i];
        }
    }

    {
        for(std::map< boost::weak_ptr<void>, long >::iterator i = m.begin(); i != m.end(); ++i)
        {
            BOOST_TEST(i->first.use_count() == i->second);
        }
    }
}

} // namespace n_map

int main()
{
    n_element_type::test();
    n_constructors::test();
    n_assignment::test();
    n_reset::test();
    n_use_count::test();
    n_swap::test();
    n_comparison::test();
    n_lock::test();

    n_map::test();

    return boost::report_errors();
}

class incomplete
{
};

boost::shared_ptr<incomplete> create_incomplete()
{
    boost::shared_ptr<incomplete> px(new incomplete);
    return px;
}

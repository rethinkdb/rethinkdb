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

//
//  intrusive_ptr_test.cpp
//
//  Copyright (c) 2002-2005 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/detail/lightweight_test.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <functional>

//

namespace N
{

class base
{
private:

    boost::detail::atomic_count use_count_;

    base(base const &);
    base & operator=(base const &);

protected:

    base(): use_count_(0)
    {
    }

    virtual ~base()
    {
    }

public:

    long use_count() const
    {
        return use_count_;
    }

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)

    inline friend void intrusive_ptr_add_ref(base * p)
    {
        ++p->use_count_;
    }

    inline friend void intrusive_ptr_release(base * p)
    {
        if(--p->use_count_ == 0) delete p;
    }

#else

    void add_ref()
    {
        ++use_count_;
    }

    void release()
    {
        if(--use_count_ == 0) delete this;
    }

#endif
};

} // namespace N

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)

namespace boost
{

inline void intrusive_ptr_add_ref(N::base * p)
{
    p->add_ref();
}

inline void intrusive_ptr_release(N::base * p)
{
    p->release();
}

} // namespace boost

#endif

//

struct X: public virtual N::base
{
};

struct Y: public X
{
};

//

namespace n_element_type
{

void f(X &)
{
}

void test()
{
    typedef boost::intrusive_ptr<X>::element_type T;
    T t;
    f(t);
}

} // namespace n_element_type

namespace n_constructors
{

void default_constructor()
{
    boost::intrusive_ptr<X> px;
    BOOST_TEST(px.get() == 0);
}

void pointer_constructor()
{
    {
        boost::intrusive_ptr<X> px(0);
        BOOST_TEST(px.get() == 0);
    }

    {
        boost::intrusive_ptr<X> px(0, false);
        BOOST_TEST(px.get() == 0);
    }

    {
        X * p = new X;
        BOOST_TEST(p->use_count() == 0);

        boost::intrusive_ptr<X> px(p);
        BOOST_TEST(px.get() == p);
        BOOST_TEST(px->use_count() == 1);
    }

    {
        X * p = new X;
        BOOST_TEST(p->use_count() == 0);

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
        using boost::intrusive_ptr_add_ref;
#endif
        intrusive_ptr_add_ref(p);
        BOOST_TEST(p->use_count() == 1);

        boost::intrusive_ptr<X> px(p, false);
        BOOST_TEST(px.get() == p);
        BOOST_TEST(px->use_count() == 1);
    }
}

void copy_constructor()
{
    {
        boost::intrusive_ptr<X> px;
        boost::intrusive_ptr<X> px2(px);
        BOOST_TEST(px2.get() == px.get());
    }

    {
        boost::intrusive_ptr<Y> py;
        boost::intrusive_ptr<X> px(py);
        BOOST_TEST(px.get() == py.get());
    }

    {
        boost::intrusive_ptr<X> px(0);
        boost::intrusive_ptr<X> px2(px);
        BOOST_TEST(px2.get() == px.get());
    }

    {
        boost::intrusive_ptr<Y> py(0);
        boost::intrusive_ptr<X> px(py);
        BOOST_TEST(px.get() == py.get());
    }

    {
        boost::intrusive_ptr<X> px(0, false);
        boost::intrusive_ptr<X> px2(px);
        BOOST_TEST(px2.get() == px.get());
    }

    {
        boost::intrusive_ptr<Y> py(0, false);
        boost::intrusive_ptr<X> px(py);
        BOOST_TEST(px.get() == py.get());
    }

    {
        boost::intrusive_ptr<X> px(new X);
        boost::intrusive_ptr<X> px2(px);
        BOOST_TEST(px2.get() == px.get());
    }

    {
        boost::intrusive_ptr<Y> py(new Y);
        boost::intrusive_ptr<X> px(py);
        BOOST_TEST(px.get() == py.get());
    }
}

void test()
{
    default_constructor();
    pointer_constructor();
    copy_constructor();
}

} // namespace n_constructors

namespace n_destructor
{

void test()
{
    boost::intrusive_ptr<X> px(new X);
    BOOST_TEST(px->use_count() == 1);

    {
        boost::intrusive_ptr<X> px2(px);
        BOOST_TEST(px->use_count() == 2);
    }

    BOOST_TEST(px->use_count() == 1);
}

} // namespace n_destructor

namespace n_assignment
{

void copy_assignment()
{
}

void conversion_assignment()
{
}

void pointer_assignment()
{
}

void test()
{
    copy_assignment();
    conversion_assignment();
    pointer_assignment();
}

} // namespace n_assignment

namespace n_access
{

void test()
{
    {
        boost::intrusive_ptr<X> px;
        BOOST_TEST(px? false: true);
        BOOST_TEST(!px);

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
        using boost::get_pointer;
#endif

        BOOST_TEST(get_pointer(px) == px.get());
    }

    {
        boost::intrusive_ptr<X> px(0);
        BOOST_TEST(px? false: true);
        BOOST_TEST(!px);

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
        using boost::get_pointer;
#endif

        BOOST_TEST(get_pointer(px) == px.get());
    }

    {
        boost::intrusive_ptr<X> px(new X);
        BOOST_TEST(px? true: false);
        BOOST_TEST(!!px);
        BOOST_TEST(&*px == px.get());
        BOOST_TEST(px.operator ->() == px.get());

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
        using boost::get_pointer;
#endif

        BOOST_TEST(get_pointer(px) == px.get());
    }
}

} // namespace n_access

namespace n_swap
{

void test()
{
    {
        boost::intrusive_ptr<X> px;
        boost::intrusive_ptr<X> px2;

        px.swap(px2);

        BOOST_TEST(px.get() == 0);
        BOOST_TEST(px2.get() == 0);

        using std::swap;
        swap(px, px2);

        BOOST_TEST(px.get() == 0);
        BOOST_TEST(px2.get() == 0);
    }

    {
        X * p = new X;
        boost::intrusive_ptr<X> px;
        boost::intrusive_ptr<X> px2(p);
        boost::intrusive_ptr<X> px3(px2);

        px.swap(px2);

        BOOST_TEST(px.get() == p);
        BOOST_TEST(px->use_count() == 2);
        BOOST_TEST(px2.get() == 0);
        BOOST_TEST(px3.get() == p);
        BOOST_TEST(px3->use_count() == 2);

        using std::swap;
        swap(px, px2);

        BOOST_TEST(px.get() == 0);
        BOOST_TEST(px2.get() == p);
        BOOST_TEST(px2->use_count() == 2);
        BOOST_TEST(px3.get() == p);
        BOOST_TEST(px3->use_count() == 2);
    }

    {
        X * p1 = new X;
        X * p2 = new X;
        boost::intrusive_ptr<X> px(p1);
        boost::intrusive_ptr<X> px2(p2);
        boost::intrusive_ptr<X> px3(px2);

        px.swap(px2);

        BOOST_TEST(px.get() == p2);
        BOOST_TEST(px->use_count() == 2);
        BOOST_TEST(px2.get() == p1);
        BOOST_TEST(px2->use_count() == 1);
        BOOST_TEST(px3.get() == p2);
        BOOST_TEST(px3->use_count() == 2);

        using std::swap;
        swap(px, px2);

        BOOST_TEST(px.get() == p1);
        BOOST_TEST(px->use_count() == 1);
        BOOST_TEST(px2.get() == p2);
        BOOST_TEST(px2->use_count() == 2);
        BOOST_TEST(px3.get() == p2);
        BOOST_TEST(px3->use_count() == 2);
    }
}

} // namespace n_swap

namespace n_comparison
{

template<class T, class U> void test2(boost::intrusive_ptr<T> const & p, boost::intrusive_ptr<U> const & q)
{
    BOOST_TEST((p == q) == (p.get() == q.get()));
    BOOST_TEST((p != q) == (p.get() != q.get()));
}

template<class T> void test3(boost::intrusive_ptr<T> const & p, boost::intrusive_ptr<T> const & q)
{
    BOOST_TEST((p == q) == (p.get() == q.get()));
    BOOST_TEST((p.get() == q) == (p.get() == q.get()));
    BOOST_TEST((p == q.get()) == (p.get() == q.get()));
    BOOST_TEST((p != q) == (p.get() != q.get()));
    BOOST_TEST((p.get() != q) == (p.get() != q.get()));
    BOOST_TEST((p != q.get()) == (p.get() != q.get()));

    // 'less' moved here as a g++ 2.9x parse error workaround
    std::less<T*> less;
    BOOST_TEST((p < q) == less(p.get(), q.get()));
}

void test()
{
    {
        boost::intrusive_ptr<X> px;
        test3(px, px);

        boost::intrusive_ptr<X> px2;
        test3(px, px2);

        boost::intrusive_ptr<X> px3(px);
        test3(px3, px3);
        test3(px, px3);
    }

    {
        boost::intrusive_ptr<X> px;

        boost::intrusive_ptr<X> px2(new X);
        test3(px, px2);
        test3(px2, px2);

        boost::intrusive_ptr<X> px3(new X);
        test3(px2, px3);

        boost::intrusive_ptr<X> px4(px2);
        test3(px2, px4);
        test3(px4, px4);
    }

    {
        boost::intrusive_ptr<X> px(new X);

        boost::intrusive_ptr<Y> py(new Y);
        test2(px, py);

        boost::intrusive_ptr<X> px2(py);
        test2(px2, py);
        test3(px, px2);
        test3(px2, px2);
    }
}

} // namespace n_comparison

namespace n_static_cast
{

void test()
{
}

} // namespace n_static_cast

namespace n_dynamic_cast
{

void test()
{
}

} // namespace n_dynamic_cast

namespace n_transitive
{

struct X: public N::base
{
    boost::intrusive_ptr<X> next;
};

void test()
{
    boost::intrusive_ptr<X> p(new X);
    p->next = boost::intrusive_ptr<X>(new X);
    BOOST_TEST(!p->next->next);
    p = p->next;
    BOOST_TEST(!p->next);
}

} // namespace n_transitive

namespace n_report_1
{

class foo: public N::base
{ 
public: 

    foo(): m_self(this)
    {
    } 

    void suicide()
    {
        m_self = 0;
    }

private:

    boost::intrusive_ptr<foo> m_self;
}; 

void test()
{
    foo * foo_ptr = new foo;
    foo_ptr->suicide();
} 

} // namespace n_report_1

int main()
{
    n_element_type::test();
    n_constructors::test();
    n_destructor::test();
    n_assignment::test();
    n_access::test();
    n_swap::test();
    n_comparison::test();
    n_static_cast::test();
    n_dynamic_cast::test();

    n_transitive::test();
    n_report_1::test();

    return boost::report_errors();
}

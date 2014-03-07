//
// ip_hash_test.cpp
//
// Copyright 2011 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <boost/intrusive_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <boost/detail/lightweight_test.hpp>

class base
{
private:

    int use_count_;

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

    inline friend void intrusive_ptr_add_ref(base * p)
    {
        ++p->use_count_;
    }

    inline friend void intrusive_ptr_release(base * p)
    {
        if(--p->use_count_ == 0) delete p;
    }
};

struct X: public base
{
};

int main()
{
    boost::hash< boost::intrusive_ptr<X> > hasher;

    boost::intrusive_ptr<X> p1, p2( p1 ), p3( new X ), p4( p3 ), p5( new X );

    BOOST_TEST_EQ( p1, p2 );
    BOOST_TEST_EQ( hasher( p1 ), hasher( p2 ) );

    BOOST_TEST_NE( p1, p3 );
    BOOST_TEST_NE( hasher( p1 ), hasher( p3 ) );

    BOOST_TEST_EQ( p3, p4 );
    BOOST_TEST_EQ( hasher( p3 ), hasher( p4 ) );

    BOOST_TEST_NE( p3, p5 );
    BOOST_TEST_NE( hasher( p3 ), hasher( p5 ) );

    return boost::report_errors();
}

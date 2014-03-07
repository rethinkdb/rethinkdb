//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/exception.hpp>
#include <boost/detail/lightweight_test.hpp>

struct
test_type
    {
    test_type( int & count ):
        count_(count)
        {
        BOOST_TEST(count_==42);
        count_=0;
        }

    ~test_type()
        {
        BOOST_TEST(!count_);
        count_=42;
        }

    void
    add_ref()
        {
        ++count_;
        }

    bool
    release()
        {
        if( --count_ )
            return false;
        else
            {
            delete this;
            return true;
            }
        }

    private:

    test_type( test_type const & );
    test_type & operator=( test_type const & );

    int & count_;
    };

int
main()
    {
    using boost::exception_detail::refcount_ptr;

        {
        refcount_ptr<test_type> x;
        BOOST_TEST(!x.get());
        }

        {
        int count=42;
        test_type * a=new test_type(count);
        BOOST_TEST(!count);
            {
            refcount_ptr<test_type> p;
            BOOST_TEST(0==count);
            p.adopt(a);
            BOOST_TEST(p.get()==a);
            BOOST_TEST(1==count);
                {
                refcount_ptr<test_type> q;
                q.adopt(p.get());
                BOOST_TEST(q.get()==a);
                BOOST_TEST(2==count);
                    {
                    refcount_ptr<test_type> t(p);
                    BOOST_TEST(t.get()==a);
                    BOOST_TEST(3==count);
                        {
                        refcount_ptr<test_type> n;
                        n=t;
                        BOOST_TEST(n.get()==a);
                        BOOST_TEST(4==count);
                        int cb=42;
                        test_type * b=new test_type(cb);
                        BOOST_TEST(0==cb);
                        n.adopt(b);
                        BOOST_TEST(1==cb);
                        BOOST_TEST(n.get()==b);
                        BOOST_TEST(3==count);
                        n.adopt(0);
                        BOOST_TEST(42==cb);
                        }
                    BOOST_TEST(t.get()==a);
                    BOOST_TEST(3==count);
                    }
                BOOST_TEST(q.get()==a);
                BOOST_TEST(2==count);
                }
            BOOST_TEST(p.get()==a);
            BOOST_TEST(1==count);
            }
        BOOST_TEST(42==count);
        }

    return boost::report_errors();
    }

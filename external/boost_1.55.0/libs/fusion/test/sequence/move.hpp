/*=============================================================================
    Copyright (c) 2012 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <boost/config.hpp>

#if defined(BOOST_NO_RVALUE_REFERENCES)
#error "Valid only on compilers that support rvalues"
#endif

#include <boost/detail/lightweight_test.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <vector>


namespace test_detail
{
    int copies = 0;

    void incr_copy()
    {
        copies++;
    }

    struct x
    {
        int i;
        x() : i(123) {}

        x(x&& rhs) : i(rhs.i) {}

        x& operator=(x&& rhs)
        {
            i = rhs.i;
            return *this;
        }

        x(x const& rhs)
        {
            incr_copy();
        }

        x& operator=(x const& rhs)
        {
            incr_copy();
            return *this;
        }
    };

    typedef std::vector<x> vector_type;
    extern bool disable_rvo; // to disable RVO

    vector_type
    generate_vec()
    {
        vector_type v;
        v.push_back(x());
        if (disable_rvo)
            return v;
        return vector_type();
   }


    template <typename T>
    T move_me(T && val)
    {
        T r(std::move(val));
        if (disable_rvo)
            return r;
        return T();
    }

    typedef FUSION_SEQUENCE<std::vector<x>> return_type;

    return_type
    generate()
    {
        return_type r(generate_vec());
        if (disable_rvo)
            return r;
        return return_type();
    }

    typedef FUSION_SEQUENCE<std::vector<x>, x> return_type2;

    return_type2
    generate2()
    {
        return_type2 r(generate_vec(), x());
        if (disable_rvo)
            return r;
        return return_type2();
    }
}

void test()
{
    using namespace boost::fusion;
    using namespace test_detail;

    return_type v = move_me(generate());
    BOOST_TEST(copies == 0);

    return_type2 v2 = move_me(generate2());
    BOOST_TEST(copies == 0);

    v2 = move_me(generate2());
    BOOST_TEST(copies == 0);

    std::cout << "Copies: " << copies << std::endl;
}

namespace test_detail
{
   bool disable_rvo = true;
}


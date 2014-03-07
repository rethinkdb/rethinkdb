/*=============================================================================
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_multi_pass.hpp>
#include <boost/scoped_ptr.hpp>
#include <iterator>
#include <string>
#include <boost/detail/lightweight_test.hpp>
#include "impl/sstream.hpp"

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

sstream_t res;

typedef multi_pass<istream_iterator<char> > default_multi_pass_t;

typedef look_ahead<istream_iterator<char>, 6> fixed_multi_pass_t;

typedef multi_pass<
    istream_iterator<char>,
    multi_pass_policies::input_iterator,
    multi_pass_policies::first_owner,
    multi_pass_policies::buf_id_check,
    multi_pass_policies::std_deque
> first_owner_multi_pass_t;


// a functor to test out the functor_multi_pass
class my_functor
{
    public:
        typedef char result_type;
        my_functor()
            : c('A')
        {}

        char operator()()
        {
            if (c == 'M')
                return eof;
            else
                return c++;
        }

        static result_type eof;
    private:
        char c;
};

my_functor::result_type my_functor::eof = '\0';

typedef multi_pass<
    my_functor,
    multi_pass_policies::functor_input,
    multi_pass_policies::first_owner,
    multi_pass_policies::no_check,
    multi_pass_policies::std_deque
> functor_multi_pass_t;

void test_default_multi_pass()
{
    res << "-*= test_default_multi_pass =*-\n";
    istream_iterator<char> end;
    boost::scoped_ptr<default_multi_pass_t> mpend(new default_multi_pass_t(end));

    {
        sstream_t ss;
        ss << "test string";

        istream_iterator<char> a(ss);
        boost::scoped_ptr<default_multi_pass_t> mp1(new default_multi_pass_t(a));

        while (*mp1 != *mpend)
        {
            res << *((*mp1)++);
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<default_multi_pass_t> mp2(new default_multi_pass_t(b));
        boost::scoped_ptr<default_multi_pass_t> mp3(new default_multi_pass_t(b));
        *mp3 = *mp2;

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp3.reset();

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<default_multi_pass_t> mp1(new default_multi_pass_t(a));
        boost::scoped_ptr<default_multi_pass_t> mp2(new default_multi_pass_t(*mp1));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp1;
            ++*mp1;
        }

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        while (*mp1 != *mpend)
        {
            res << **mp1;
            ++*mp1;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<default_multi_pass_t> mp2(new default_multi_pass_t(b));
        boost::scoped_ptr<default_multi_pass_t> mp3(new default_multi_pass_t(b));
        *mp3 = *mp2;

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp3.reset();
        ++*mp2;

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<default_multi_pass_t> mp1(new default_multi_pass_t(a));
        boost::scoped_ptr<default_multi_pass_t> mp2(new default_multi_pass_t(*mp1));

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        for (int i = 0; i < 4; ++i)
        {
            res << **mp1;
            ++*mp1;
        }

        BOOST_TEST(*mp1 != *mp2);
        BOOST_TEST(*mp1 > *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp2 < *mp1);
        BOOST_TEST(*mp2 <= *mp1);
        while (*mp2 != *mp1)
        {
            res << **mp2;
            ++*mp2;
        }

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        while (*mp1 != *mpend)
        {
            res << **mp1;
            ++*mp1;
        }

        BOOST_TEST(*mp1 != *mp2);
        BOOST_TEST(*mp1 > *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp2 < *mp1);
        BOOST_TEST(*mp2 <= *mp1);
        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<default_multi_pass_t> mp1(new default_multi_pass_t(a));
        boost::scoped_ptr<default_multi_pass_t> mp2(new default_multi_pass_t(a));
        BOOST_TEST(*mp1 != *mp2);
        ++*mp1;
        BOOST_TEST(*mp1 != *mp2);

    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<default_multi_pass_t> mp2(new default_multi_pass_t(b));
        boost::scoped_ptr<default_multi_pass_t> mp3(new default_multi_pass_t(b));
        *mp3 = *mp2;

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp2->clear_queue();

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        try
        {
            res << **mp3; // this should throw illegal_backtracking
            BOOST_TEST(0);
        }
        catch (const BOOST_SPIRIT_CLASSIC_NS::multi_pass_policies::illegal_backtracking& /*e*/)
        {
        }
        res << endl;
    }


}

void test_fixed_multi_pass()
{
    res << "-*= test_fixed_multi_pass =*-\n";
    istream_iterator<char> end;
    boost::scoped_ptr<fixed_multi_pass_t> mpend(new fixed_multi_pass_t(end));

    {
        sstream_t ss;
        ss << "test string";

        istream_iterator<char> a(ss);
        boost::scoped_ptr<fixed_multi_pass_t> mp1(new fixed_multi_pass_t(a));

        while (*mp1 != *mpend)
        {
            res << *((*mp1)++);
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<fixed_multi_pass_t> mp2(new fixed_multi_pass_t(b));
        boost::scoped_ptr<fixed_multi_pass_t> mp3(new fixed_multi_pass_t(*mp2));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp3.reset();

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<fixed_multi_pass_t> mp1(new fixed_multi_pass_t(a));
        boost::scoped_ptr<fixed_multi_pass_t> mp2(new fixed_multi_pass_t(*mp1));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp1;
            ++*mp1;
        }

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        while (*mp1 != *mpend)
        {
            res << **mp1;
            ++*mp1;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<fixed_multi_pass_t> mp2(new fixed_multi_pass_t(b));
        boost::scoped_ptr<fixed_multi_pass_t> mp3(new fixed_multi_pass_t(*mp2));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp3.reset();
        ++*mp2;

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<fixed_multi_pass_t> mp1(new fixed_multi_pass_t(a));
        boost::scoped_ptr<fixed_multi_pass_t> mp2(new fixed_multi_pass_t(*mp1));

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        for (int i = 0; i < 4; ++i)
        {
            res << **mp1;
            ++*mp1;
        }

        BOOST_TEST(*mp1 != *mp2);
        BOOST_TEST(*mp1 > *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp2 < *mp1);
        BOOST_TEST(*mp2 <= *mp1);
        while (*mp2 != *mp1)
        {
            res << **mp2;
            ++*mp2;
        }

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        while (*mp1 != *mpend)
        {
            res << **mp1;
            ++*mp1;
        }

        BOOST_TEST(*mp1 != *mp2);
        BOOST_TEST(*mp1 > *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp2 < *mp1);
        BOOST_TEST(*mp2 <= *mp1);
        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<fixed_multi_pass_t> mp1(new fixed_multi_pass_t(a));
        boost::scoped_ptr<fixed_multi_pass_t> mp2(new fixed_multi_pass_t(a));
        BOOST_TEST(*mp1 != *mp2);
        ++*mp1;
        BOOST_TEST(*mp1 != *mp2);

    }

}

void test_first_owner_multi_pass()
{
    res << "-*= test_first_owner_multi_pass =*-\n";
    istream_iterator<char> end;
    boost::scoped_ptr<first_owner_multi_pass_t> mpend(new first_owner_multi_pass_t(end));

    {
        sstream_t ss;
        ss << "test string";

        istream_iterator<char> a(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp1(new first_owner_multi_pass_t(a));

        while (*mp1 != *mpend)
        {
            res << *((*mp1)++);
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp2(new first_owner_multi_pass_t(b));
        boost::scoped_ptr<first_owner_multi_pass_t> mp3(new first_owner_multi_pass_t(*mp2));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp3.reset();

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp1(new first_owner_multi_pass_t(a));
        boost::scoped_ptr<first_owner_multi_pass_t> mp2(new first_owner_multi_pass_t(*mp1));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp1;
            ++*mp1;
        }

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        while (*mp1 != *mpend)
        {
            res << **mp1;
            ++*mp1;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp2(new first_owner_multi_pass_t(b));
        boost::scoped_ptr<first_owner_multi_pass_t> mp3(new first_owner_multi_pass_t(*mp2));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp3.reset();
        ++*mp2;

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp1(new first_owner_multi_pass_t(a));
        boost::scoped_ptr<first_owner_multi_pass_t> mp2(new first_owner_multi_pass_t(*mp1));

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        for (int i = 0; i < 4; ++i)
        {
            res << **mp1;
            ++*mp1;
        }

        BOOST_TEST(*mp1 != *mp2);
        BOOST_TEST(*mp1 > *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp2 < *mp1);
        BOOST_TEST(*mp2 <= *mp1);
        while (*mp2 != *mp1)
        {
            res << **mp2;
            ++*mp2;
        }

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        while (*mp1 != *mpend)
        {
            res << **mp1;
            ++*mp1;
        }

        BOOST_TEST(*mp1 != *mp2);
        BOOST_TEST(*mp1 > *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp2 < *mp1);
        BOOST_TEST(*mp2 <= *mp1);
        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        BOOST_TEST(*mp1 == *mp2);
        BOOST_TEST(*mp1 >= *mp2);
        BOOST_TEST(*mp1 <= *mp2);
        res << endl;
    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> a(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp1(new first_owner_multi_pass_t(a));
        boost::scoped_ptr<first_owner_multi_pass_t> mp2(new first_owner_multi_pass_t(a));
        BOOST_TEST(*mp1 != *mp2);
        ++*mp1;
        BOOST_TEST(*mp1 != *mp2);

    }

    {
        sstream_t ss;
        ss << "test string";
        istream_iterator<char> b(ss);
        boost::scoped_ptr<first_owner_multi_pass_t> mp2(new first_owner_multi_pass_t(b));
        boost::scoped_ptr<first_owner_multi_pass_t> mp3(new first_owner_multi_pass_t(*mp2));

        for (int i = 0; i < 4; ++i)
        {
            res << **mp2;
            ++*mp2;
        }

        mp2->clear_queue();

        while (*mp2 != *mpend)
        {
            res << **mp2;
            ++*mp2;
        }

        try
        {
            res << **mp3; // this should throw illegal_backtracking
            BOOST_TEST(0);
        }
        catch (const BOOST_SPIRIT_CLASSIC_NS::multi_pass_policies::illegal_backtracking& /*e*/)
        {
        }
        res << endl;
    }

}


void test_functor_multi_pass()
{
    res << "-*= test_functor_multi_pass =*-\n";
    functor_multi_pass_t mpend;

    {
        functor_multi_pass_t mp1 = functor_multi_pass_t(my_functor());

        while (mp1 != mpend)
        {
            res << *(mp1++);
        }

        res << endl;
    }

    {
        functor_multi_pass_t mp1 = functor_multi_pass_t(my_functor());
        functor_multi_pass_t mp2 = functor_multi_pass_t(mp1);

        for (int i = 0; i < 4; ++i)
        {
            res << *mp1;
            ++mp1;
        }

        while (mp2 != mpend)
        {
            res << *mp2;
            ++mp2;
        }

        while (mp1 != mpend)
        {
            res << *mp1;
            ++mp1;
        }

        res << endl;
    }

    {
        functor_multi_pass_t mp1 = functor_multi_pass_t(my_functor());
        functor_multi_pass_t mp2 = functor_multi_pass_t(mp1);

        BOOST_TEST(mp1 == mp2);
        BOOST_TEST(mp1 >= mp2);
        BOOST_TEST(mp1 <= mp2);
        for (int i = 0; i < 4; ++i)
        {
            res << *mp1;
            ++mp1;
        }

        BOOST_TEST(mp1 != mp2);
        BOOST_TEST(mp1 > mp2);
        BOOST_TEST(mp1 >= mp2);
        BOOST_TEST(mp2 < mp1);
        BOOST_TEST(mp2 <= mp1);
        while (mp2 != mp1)
        {
            res << *mp2;
            ++mp2;
        }

        BOOST_TEST(mp1 == mp2);
        BOOST_TEST(mp1 >= mp2);
        BOOST_TEST(mp1 <= mp2);
        while (mp1 != mpend)
        {
            res << *mp1;
            ++mp1;
        }

        BOOST_TEST(mp1 != mp2);
        BOOST_TEST(mp1 > mp2);
        BOOST_TEST(mp1 >= mp2);
        BOOST_TEST(mp2 < mp1);
        BOOST_TEST(mp2 <= mp1);
        while (mp2 != mpend)
        {
            res << *mp2;
            ++mp2;
        }

        BOOST_TEST(mp1 == mp2);
        BOOST_TEST(mp1 >= mp2);
        BOOST_TEST(mp1 <= mp2);
        res << endl;
    }

    {
        functor_multi_pass_t mp1 = functor_multi_pass_t(my_functor());
        functor_multi_pass_t mp2 = functor_multi_pass_t(my_functor());
        BOOST_TEST(mp1 != mp2);
        ++mp1;
        BOOST_TEST(mp1 != mp2);

    }
}

int main(int, char**)
{

    test_default_multi_pass();
    test_fixed_multi_pass();
    test_first_owner_multi_pass();
    test_functor_multi_pass();

    BOOST_TEST(getstring(res) == "-*= test_default_multi_pass =*-\n"
            "teststring\n"
            "teststring\n"
            "testteststringstring\n"
            "testtring\n"
            "testteststringstring\n"
            "teststring\n"
            "-*= test_fixed_multi_pass =*-\n"
            "teststring\n"
            "teststring\n"
            "testteststringstring\n"
            "testtring\n"
            "testteststringstring\n"
            "-*= test_first_owner_multi_pass =*-\n"
            "teststring\n"
            "teststring\n"
            "testteststringstring\n"
            "testtring\n"
            "testteststringstring\n"
            "teststring\n"
            "-*= test_functor_multi_pass =*-\n"
            "ABCDEFGHIJKL\n"
            "ABCDABCDEFGHIJKLEFGHIJKL\n"
            "ABCDABCDEFGHIJKLEFGHIJKL\n");

    return boost::report_errors();
}

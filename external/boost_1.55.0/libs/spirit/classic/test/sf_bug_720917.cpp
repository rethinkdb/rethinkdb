/*=============================================================================
    Copyright (c) 2003 Giovanni Bajo
    Copyrigh (c) 2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_multi_pass.hpp>
#include <iterator>
#include "impl/sstream.hpp"
#include <boost/detail/lightweight_test.hpp>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

// Test for bug #720917 
// http://sf.net/tracker/index.php?func=detail&aid=720917&group_id=28447&atid=393386
//
// Check that it's possible to use multi_pass
// together with standard library as a normal iterator
//

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

////////////////////////////////////////////////
// four types of multi_pass iterators
typedef multi_pass<
    my_functor,
    multi_pass_policies::functor_input,
    multi_pass_policies::first_owner,
    multi_pass_policies::no_check,
    multi_pass_policies::std_deque
> functor_multi_pass_t;

typedef multi_pass<istream_iterator<char> > default_multi_pass_t;
typedef look_ahead<istream_iterator<char>, 6> fixed_multi_pass_t;

typedef multi_pass<
    istream_iterator<char>,
    multi_pass_policies::input_iterator,
    multi_pass_policies::first_owner,
    multi_pass_policies::buf_id_check,
    multi_pass_policies::std_deque
> first_owner_multi_pass_t;


////////////////////////////////////////////////
// the test cases
template <typename IterT>
void construct_string_from(void)
{
    sstream_t ss;
    ss << "test string";

    IterT mpend;
    istream_iterator<char> a(ss);
    IterT mp1(a);

    std::string dummy;
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
    dummy.assign(mp1, mpend);
#else
    copy(mp1, mpend, inserter(dummy, dummy.end()));
#endif
}

template <>
void construct_string_from<functor_multi_pass_t>(void) 
{
    functor_multi_pass_t mpend;
    functor_multi_pass_t mp1 = functor_multi_pass_t(my_functor());

    std::string dummy;
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
    dummy.assign(mp1, mpend);
#else
    copy(mp1, mpend, inserter(dummy, dummy.end()));
#endif
}

////////////////////////////////////////////////
// Definition of the test suite
int
main()
{
    construct_string_from<default_multi_pass_t>();
    construct_string_from<fixed_multi_pass_t>();
    construct_string_from<first_owner_multi_pass_t>();
    construct_string_from<functor_multi_pass_t>();
    return boost::report_errors();
}

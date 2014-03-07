/*=============================================================================
    Copyright (c) 2004 Joao Abecasis
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// This test requires NDEBUG to be undefined,  because it depends on 
// BOOST_SPIRIT_ASSERT throwing an exception.
#ifdef NDEBUG  
#  undef NDEBUG  
#endif  

#include <stdexcept>

#define BOOST_SPIRIT_ASSERT_EXCEPTION ::spirit_exception

struct spirit_exception : std::exception
{
    spirit_exception(char const * msg)
        : message(msg)
    {
    }
    ~spirit_exception() throw() {}

    char const* what() const throw() { return message; }

    char const * message;
};

#include <boost/spirit/include/classic_scanner.hpp>
#include <boost/spirit/home/classic/symbols/impl/tst.ipp>
#include <boost/utility/addressof.hpp>

#include <boost/detail/lightweight_test.hpp>

typedef char char_type;
typedef char const * iterator;

char_type data_[] = "whatever";

iterator begin = data_;
iterator end = data_
    + sizeof(data_)/sizeof(char_type); // Yes, this is an intentional bug ;)

char_type data2_[] = "\0something";
iterator begin2 = data2_;
iterator end2 = data2_ + sizeof(data2_)/sizeof(char_type) - 1;

int main()
{
    typedef BOOST_SPIRIT_CLASSIC_NS::scanner<> scanner;
    typedef BOOST_SPIRIT_CLASSIC_NS::impl::tst<void *, char_type> symbols;

    symbols symbols_;

    try
    {
        // It is not ok to add strings containing the null character.
        symbols_.add(begin, end, (void*) boost::addressof(symbols_));
        BOOST_TEST(0);
    }
    catch (spirit_exception &/*e*/)
    {
    }

    try
    {
        // It is not ok to add strings containing the null character.
        symbols_.add(begin2, end2, (void*) boost::addressof(symbols_));
        BOOST_TEST(0);
    }
    catch (spirit_exception &/*e*/)
    {
    }
    return boost::report_errors();
}

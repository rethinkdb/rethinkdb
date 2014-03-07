/*=============================================================================
    Copyright (c) 2003 Giovanni Bajo
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <cstdio>
#include <iostream>
#include <boost/concept_check.hpp>
#include <boost/spirit/include/classic_file_iterator.hpp>

// This checks for a namespace related problem in VC8
// The problem can be avoided by not using "using namespace std;" in the
// Spirit headers
namespace vc8_bug_1 { struct plus {}; }
namespace vc8_bug_2 { using namespace vc8_bug_1; struct test : plus {}; }


using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace {

static const char* TMP_FILE = "file_iter.tmp";

bool CreateTempFile(void)
{
    FILE* f = fopen(TMP_FILE, "wb");

    if (!f)
        return false;

    for (int i=0;i<256;i++)
    {
        unsigned char ci = (unsigned char)i;

        if (fwrite(&ci,1,1,f) == 0)
        {
            fclose(f);
            return false;
        }
    }

    fclose(f);
    return true;
}

template <typename ITER>
void RunTest(void)
{
    // Check constructor opening a file
    ITER a(TMP_FILE);
    BOOST_TEST(!!a);

    // Assert dereference (twice: derefence
    //  must not move the iterator)
    BOOST_TEST(*a == 0);
    BOOST_TEST(*a == 0);

    // Check random access
    BOOST_TEST(a[123] == 123);

    // Check copy constructor and operator==
    ITER c(a);
    BOOST_TEST(c == a);
    BOOST_TEST(!(c != a));

    // Check assignment operator
    ITER d; d = a;
    BOOST_TEST(d == a);
    BOOST_TEST(!(d != a));

    // Check make_end()
    ITER b(a.make_end());
    BOOST_TEST(!!b);
    BOOST_TEST(a != b);
    BOOST_TEST(a+256 == b);
    BOOST_TEST(a == b-256);

    // Check copy constructor on non-trivial position
    BOOST_TEST(*ITER(a+67) == 67);

    // Check increment
    ++a; ++a; a++; a++;
    BOOST_TEST(*a == 4);
    BOOST_TEST(a == c+4);

    // Check decrement
    --a; --a; a--; a--;
    BOOST_TEST(*a == 0);
    BOOST_TEST(a == c);

    // Check end iterator increment/decrement
    --b; b--;
    BOOST_TEST(*b == 254);
    BOOST_TEST(a+254 == b);
    ++b; b++;
    BOOST_TEST(a+256 == b);

    // Check order
    a += 128;
    BOOST_TEST(c < a);
    BOOST_TEST(a < b);
    BOOST_TEST(a > c);
    BOOST_TEST(b > a);

    // Check assignment
    a = b;
    BOOST_TEST(a == b);
    a = c;
    BOOST_TEST(a == c);

    // Check weak order
    BOOST_TEST(a <= c);
    BOOST_TEST(a >= c);
    BOOST_TEST(a <= b);
    BOOST_TEST(!(a >= b));

    // Check increment through end
    a += 255;
    BOOST_TEST(a != b);
    ++a;
    BOOST_TEST(a == b);
    ++a;
    BOOST_TEST(a != b);
}

///////////////////////////////////////////////////////////////////////////////
}

typedef unsigned char character_t;
typedef file_iterator<character_t,
    fileiter_impl::std_file_iterator<character_t> > iter;
BOOST_CLASS_REQUIRE(iter, boost, RandomAccessIteratorConcept);

#ifdef BOOST_SPIRIT_FILEITERATOR_WINDOWS
    typedef file_iterator<character_t,
        fileiter_impl::mmap_file_iterator<character_t> > iterwin;
    BOOST_CLASS_REQUIRE(iterwin, boost, RandomAccessIteratorConcept);
#endif
#ifdef BOOST_SPIRIT_FILEITERATOR_POSIX
    typedef file_iterator<character_t,
        fileiter_impl::mmap_file_iterator<character_t> > iterposix;
    BOOST_CLASS_REQUIRE(iterposix, boost, RandomAccessIteratorConcept);
#endif

int main(void)
{
    if (!CreateTempFile())
    {
        cerr << "ERROR: Cannot create temporary file file_iter.tmp" << endl;
        return 2;
    }

    cerr << "Testing standard iterator" << endl;
    RunTest<iter>();

#ifdef BOOST_SPIRIT_FILEITERATOR_WINDOWS
    cerr << "Testing Windows iterator" << endl;
    RunTest<iterwin>();
#endif

#ifdef BOOST_SPIRIT_FILEITERATOR_POSIX
    cerr << "Testing POSIX iterator" << endl;
    RunTest<iterposix>();
#endif

    // Check if the file handles were closed correctly
    BOOST_TEST(remove(TMP_FILE) == 0);

    return boost::report_errors();
}

#ifdef BOOST_NO_EXCEPTIONS

namespace boost {
    void throw_exception(std::exception const& e)
    {
        BOOST_EROR("throw_exception");
    }
}

#endif

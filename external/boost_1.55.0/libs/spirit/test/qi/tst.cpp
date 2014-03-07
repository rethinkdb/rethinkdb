/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/home/qi/string/tst.hpp>
#include <boost/spirit/home/qi/string/tst_map.hpp>

#include <string>
#include <cctype>
#include <iostream>

namespace
{
    template <typename TST, typename Char>
    void add(TST& tst, Char const* s, int data)
    {
        Char const* last = s;
        while (*last)
            last++;
        tst.add(s, last, data);
    }

    template <typename TST, typename Char>
    void remove(TST& tst, Char const* s)
    {
        Char const* last = s;
        while (*last)
            last++;
        tst.remove(s, last);
    }

    template <typename TST, typename Char>
    void docheck(TST const& tst, Char const* s, bool expected, int N = 0, int val = -1)
    {
        Char const* first = s;
        Char const* last = s;
        while (*last)
            last++;
        int* r = tst.find(s, last);
        BOOST_TEST((r != 0) == expected);
        if (r != 0)
            BOOST_TEST((s-first) == N);
        if (r)
            BOOST_TEST(*r == val);
    }

    struct printer
    {
        template <typename String, typename Data>
        void operator()(String const& s, Data const& data)
        {
            std::cout << "    " << s << ": " << data << std::endl;
        }
    };

    template <typename TST>
    void print(TST const& tst)
    {
        std::cout << '[' << std::endl;
        tst.for_each(printer());
        std::cout << ']' << std::endl;
    }

    struct no_case_filter
    {
        template <typename Char>
        Char operator()(Char ch) const
        {
            return static_cast<Char>(std::tolower(ch));
        }
    };

    template <typename TST, typename Char>
    void nc_check(TST const& tst, Char const* s, bool expected, int N = 0, int val = -1)
    {
        Char const* first = s;
        Char const* last = s;
        while (*last)
            last++;
        int* r = tst.find(s, last, no_case_filter());
        BOOST_TEST((r != 0) == expected);
        if (r != 0)
            BOOST_TEST((s-first) == N);
        if (r)
            BOOST_TEST(*r == val);
    }
}

template <typename Lookup, typename WideLookup>
void tests()
{
    { // basic tests
        Lookup lookup;

        docheck(lookup, "not-yet-there", false);
        docheck(lookup, "", false);

        add(lookup, "apple", 123);
        docheck(lookup, "apple", true, 5, 123); // full match
        docheck(lookup, "banana", false); // no-match
        docheck(lookup, "applexxx", true, 5, 123); // partial match

        add(lookup, "applepie", 456);
        docheck(lookup, "applepie", true, 8, 456); // full match
        docheck(lookup, "banana", false); // no-match
        docheck(lookup, "applepiexxx", true, 8, 456); // partial match
        docheck(lookup, "apple", true, 5, 123); // full match
        docheck(lookup, "applexxx", true, 5, 123); // partial match
    }

    { // variation of above
        Lookup lookup;

        add(lookup, "applepie", 456);
        add(lookup, "apple", 123);

        docheck(lookup, "applepie", true, 8, 456); // full match
        docheck(lookup, "banana", false); // no-match
        docheck(lookup, "applepiexxx", true, 8, 456); // partial match
        docheck(lookup, "apple", true, 5, 123); // full match
        docheck(lookup, "applexxx", true, 5, 123); // partial match
    }
    { // variation of above
        Lookup lookup;

        add(lookup, "applepie", 456);
        add(lookup, "apple", 123);

        docheck(lookup, "applepie", true, 8, 456); // full match
        docheck(lookup, "banana", false); // no-match
        docheck(lookup, "applepiexxx", true, 8, 456); // partial match
        docheck(lookup, "apple", true, 5, 123); // full match
        docheck(lookup, "applexxx", true, 5, 123); // partial match
    }

    { // narrow char tests
        Lookup lookup;
        add(lookup, "pineapple", 1);
        add(lookup, "orange", 2);
        add(lookup, "banana", 3);
        add(lookup, "applepie", 4);
        add(lookup, "apple", 5);

        docheck(lookup, "pineapple", true, 9, 1);
        docheck(lookup, "orange", true, 6, 2);
        docheck(lookup, "banana", true, 6, 3);
        docheck(lookup, "apple", true, 5, 5);
        docheck(lookup, "pizza", false);
        docheck(lookup, "steak", false);
        docheck(lookup, "applepie", true, 8, 4);
        docheck(lookup, "bananarama", true, 6, 3);
        docheck(lookup, "applet", true, 5, 5);
        docheck(lookup, "applepi", true, 5, 5);
        docheck(lookup, "appl", false);

        docheck(lookup, "pineapplez", true, 9, 1);
        docheck(lookup, "orangez", true, 6, 2);
        docheck(lookup, "bananaz", true, 6, 3);
        docheck(lookup, "applez", true, 5, 5);
        docheck(lookup, "pizzaz", false);
        docheck(lookup, "steakz", false);
        docheck(lookup, "applepiez", true, 8, 4);
        docheck(lookup, "bananaramaz", true, 6, 3);
        docheck(lookup, "appletz", true, 5, 5);
        docheck(lookup, "applepix", true, 5, 5);
    }

    { // wide char tests
        WideLookup lookup;
        add(lookup, L"pineapple", 1);
        add(lookup, L"orange", 2);
        add(lookup, L"banana", 3);
        add(lookup, L"applepie", 4);
        add(lookup, L"apple", 5);

        docheck(lookup, L"pineapple", true, 9, 1);
        docheck(lookup, L"orange", true, 6, 2);
        docheck(lookup, L"banana", true, 6, 3);
        docheck(lookup, L"apple", true, 5, 5);
        docheck(lookup, L"pizza", false);
        docheck(lookup, L"steak", false);
        docheck(lookup, L"applepie", true, 8, 4);
        docheck(lookup, L"bananarama", true, 6, 3);
        docheck(lookup, L"applet", true, 5, 5);
        docheck(lookup, L"applepi", true, 5, 5);
        docheck(lookup, L"appl", false);

        docheck(lookup, L"pineapplez", true, 9, 1);
        docheck(lookup, L"orangez", true, 6, 2);
        docheck(lookup, L"bananaz", true, 6, 3);
        docheck(lookup, L"applez", true, 5, 5);
        docheck(lookup, L"pizzaz", false);
        docheck(lookup, L"steakz", false);
        docheck(lookup, L"applepiez", true, 8, 4);
        docheck(lookup, L"bananaramaz", true, 6, 3);
        docheck(lookup, L"appletz", true, 5, 5);
        docheck(lookup, L"applepix", true, 5, 5);
    }

    { // test remove
        Lookup lookup;
        add(lookup, "pineapple", 1);
        add(lookup, "orange", 2);
        add(lookup, "banana", 3);
        add(lookup, "applepie", 4);
        add(lookup, "apple", 5);

        docheck(lookup, "pineapple", true, 9, 1);
        docheck(lookup, "orange", true, 6, 2);
        docheck(lookup, "banana", true, 6, 3);
        docheck(lookup, "apple", true, 5, 5);
        docheck(lookup, "applepie", true, 8, 4);
        docheck(lookup, "bananarama", true, 6, 3);
        docheck(lookup, "applet", true, 5, 5);
        docheck(lookup, "applepi", true, 5, 5);
        docheck(lookup, "appl", false);

        remove(lookup, "banana");
        docheck(lookup, "pineapple", true, 9, 1);
        docheck(lookup, "orange", true, 6, 2);
        docheck(lookup, "banana", false);
        docheck(lookup, "apple", true, 5, 5);
        docheck(lookup, "applepie", true, 8, 4);
        docheck(lookup, "bananarama", false);
        docheck(lookup, "applet", true, 5, 5);
        docheck(lookup, "applepi", true, 5, 5);
        docheck(lookup, "appl", false);

        remove(lookup, "apple");
        docheck(lookup, "pineapple", true, 9, 1);
        docheck(lookup, "orange", true, 6, 2);
        docheck(lookup, "apple", false);
        docheck(lookup, "applepie", true, 8, 4);
        docheck(lookup, "applet", false);
        docheck(lookup, "applepi", false);
        docheck(lookup, "appl", false);

        remove(lookup, "orange");
        docheck(lookup, "pineapple", true, 9, 1);
        docheck(lookup, "orange", false);
        docheck(lookup, "applepie", true, 8, 4);

        remove(lookup, "pineapple");
        docheck(lookup, "pineapple", false);
        docheck(lookup, "orange", false);
        docheck(lookup, "applepie", true, 8, 4);

        remove(lookup, "applepie");
        docheck(lookup, "applepie", false);
    }

    { // copy/assign/clear test
        Lookup lookupa;
        add(lookupa, "pineapple", 1);
        add(lookupa, "orange", 2);
        add(lookupa, "banana", 3);
        add(lookupa, "applepie", 4);
        add(lookupa, "apple", 5);

        Lookup lookupb(lookupa); // copy ctor
        docheck(lookupb, "pineapple", true, 9, 1);
        docheck(lookupb, "orange", true, 6, 2);
        docheck(lookupb, "banana", true, 6, 3);
        docheck(lookupb, "apple", true, 5, 5);
        docheck(lookupb, "pizza", false);
        docheck(lookupb, "steak", false);
        docheck(lookupb, "applepie", true, 8, 4);
        docheck(lookupb, "bananarama", true, 6, 3);
        docheck(lookupb, "applet", true, 5, 5);
        docheck(lookupb, "applepi", true, 5, 5);
        docheck(lookupb, "appl", false);

        lookupb.clear(); // clear
        docheck(lookupb, "pineapple", false);
        docheck(lookupb, "orange", false);
        docheck(lookupb, "banana", false);
        docheck(lookupb, "apple", false);
        docheck(lookupb, "applepie", false);
        docheck(lookupb, "bananarama", false);
        docheck(lookupb, "applet", false);
        docheck(lookupb, "applepi", false);
        docheck(lookupb, "appl", false);

        lookupb = lookupa; // assign
        docheck(lookupb, "pineapple", true, 9, 1);
        docheck(lookupb, "orange", true, 6, 2);
        docheck(lookupb, "banana", true, 6, 3);
        docheck(lookupb, "apple", true, 5, 5);
        docheck(lookupb, "pizza", false);
        docheck(lookupb, "steak", false);
        docheck(lookupb, "applepie", true, 8, 4);
        docheck(lookupb, "bananarama", true, 6, 3);
        docheck(lookupb, "applet", true, 5, 5);
        docheck(lookupb, "applepi", true, 5, 5);
        docheck(lookupb, "appl", false);
    }

    { // test for_each
        Lookup lookup;
        add(lookup, "pineapple", 1);
        add(lookup, "orange", 2);
        add(lookup, "banana", 3);
        add(lookup, "applepie", 4);
        add(lookup, "apple", 5);

        print(lookup);
    }

    { // case insensitive tests
        Lookup lookup;

        // NOTE: make sure all entries are in lower-case!!!
        add(lookup, "pineapple", 1);
        add(lookup, "orange", 2);
        add(lookup, "banana", 3);
        add(lookup, "applepie", 4);
        add(lookup, "apple", 5);

        nc_check(lookup, "pineapple", true, 9, 1);
        nc_check(lookup, "orange", true, 6, 2);
        nc_check(lookup, "banana", true, 6, 3);
        nc_check(lookup, "apple", true, 5, 5);
        nc_check(lookup, "applepie", true, 8, 4);

        nc_check(lookup, "PINEAPPLE", true, 9, 1);
        nc_check(lookup, "ORANGE", true, 6, 2);
        nc_check(lookup, "BANANA", true, 6, 3);
        nc_check(lookup, "APPLE", true, 5, 5);
        nc_check(lookup, "APPLEPIE", true, 8, 4);

        nc_check(lookup, "pineApple", true, 9, 1);
        nc_check(lookup, "orangE", true, 6, 2);
        nc_check(lookup, "Banana", true, 6, 3);
        nc_check(lookup, "aPPLe", true, 5, 5);
        nc_check(lookup, "ApplePie", true, 8, 4);

        print(lookup);
    }
}

int main()
{
    using boost::spirit::qi::tst;
    using boost::spirit::qi::tst_map;

    tests<tst<char, int>, tst<wchar_t, int> >();
    tests<tst_map<char, int>, tst_map<wchar_t, int> >();

    return boost::report_errors();
}


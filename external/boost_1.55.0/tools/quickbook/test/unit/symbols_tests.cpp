/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c)      2003 Martin Wille
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <iostream>
#include <string>
#include <boost/detail/lightweight_test.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/swap.hpp>
#include "symbols.hpp"

///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////

template <typename IteratorT>
bool
equal(IteratorT p, IteratorT q)
{
    while (*p && *p == *q)
    {
        ++p;
        ++q;
    }
    return *p == *q;
}

template <class SymbolsT, typename CharT>
void
docheck
(
    SymbolsT const &sym,
    CharT    const *candidate,
    bool           hit,
    CharT    const *result,
    int            length
)
{
    parse_info<CharT const*> info = parse(candidate, sym);

#define correctly_matched hit == info.hit
#define correct_match_length unsigned(length) == info.length
#define correct_tail equal(candidate + (hit?1:0)*length, result)

    BOOST_TEST(correctly_matched);

    if (hit)
    {
        BOOST_TEST(correct_match_length);
        BOOST_TEST(correct_tail);
    }
    else
    {
        BOOST_TEST(correct_tail);
    }
}

template <typename T>
struct store_action
{
    store_action(T const &v) : value(v) {}
    void operator()(T &v) const { v = value; }
private:
    T const value;
};

template <typename T>
store_action<T>
store(T const &v)
{
    return v;
}

template <typename T>
struct check_action
{
    check_action(T const &v) : value(v) {}

#define correct_value_stored (v==value)
    void operator()(T const &v) const { BOOST_TEST(correct_value_stored); }
private:
    T const value;
};

template <typename T>
check_action<T>
docheck(T const &v)
{
    return v;
}

static void
default_constructible()
{   // this actually a compile time test
    symbols<int, char, quickbook::tst<int, char> > ns1;
    symbols<int, wchar_t, quickbook::tst<int, wchar_t> > ws1;
    symbols<std::string, char, quickbook::tst<std::string, char> > ns2;
    symbols<std::string, wchar_t, quickbook::tst<std::string, wchar_t> > ws2;

    (void)ns1; (void)ws1; (void)ns2; (void)ws2;
}

typedef symbols<int, char, quickbook::tst<int, char> > nsymbols;
typedef symbols<int, wchar_t, quickbook::tst<int, wchar_t> > wsymbols;

static void
narrow_match_tests()
{
    nsymbols sym;
    sym = "pineapple", "orange", "banana", "applepie", "apple";

    docheck(sym, "pineapple", true, "", 9);
    docheck(sym, "orange", true, "", 6);
    docheck(sym, "banana", true, "", 6);
    docheck(sym, "apple", true, "", 5);
    docheck(sym, "pizza", false, "pizza", -1);
    docheck(sym, "steak", false, "steak", -1);
    docheck(sym, "applepie", true, "", 8);
    docheck(sym, "bananarama", true, "rama", 6);
    docheck(sym, "applet", true, "t", 5);
    docheck(sym, "applepi", true, "pi", 5);
    docheck(sym, "appl", false, "appl", -1);

    docheck(sym, "pineapplez", true, "z", 9);
    docheck(sym, "orangez", true, "z", 6);
    docheck(sym, "bananaz", true, "z", 6);
    docheck(sym, "applez", true, "z", 5);
    docheck(sym, "pizzaz", false, "pizzaz", -1);
    docheck(sym, "steakz", false, "steakz", -1);
    docheck(sym, "applepiez", true, "z", 8);
    docheck(sym, "bananaramaz", true, "ramaz", 6);
    docheck(sym, "appletz", true, "tz", 5);
    docheck(sym, "applepix", true, "pix", 5);
}

static void
narrow_copy_ctor_tests()
{
    nsymbols sym;
    sym = "pineapple", "orange", "banana", "applepie", "apple";

    nsymbols sym2(sym);
    docheck(sym2, "pineapple", true, "", 9);
    docheck(sym2, "pizza", false, "pizza", -1);
    docheck(sym2, "bananarama", true, "rama", 6);
}

static void
narrow_assigment_operator_tests()
{
    nsymbols sym;
    sym = "pineapple", "orange", "banana", "applepie", "apple";

    nsymbols sym2;
    sym2 = sym;

    docheck(sym2, "pineapple", true, "", 9);
    docheck(sym2, "pizza", false, "pizza", -1);
    docheck(sym2, "bananarama", true, "rama", 6);
}

static void
narrow_swap_tests()
{
    nsymbols sym, sym2;
    sym = "pineapple", "orange", "banana", "applepie", "apple";
    sym2 = "potato", "cucumber", "cauliflower", "carrot";

    boost::swap(sym, sym2);

    docheck(sym2, "pineapple", true, "", 9);
    docheck(sym2, "pizza", false, "pizza", -1);
    docheck(sym2, "bananarama", true, "rama", 6);
    docheck(sym, "potatoe", true, "e", 6);
    docheck(sym, "cauliflour", false, "cauliflour", -1);
}

static void
narrow_value_tests()
{   // also tests the add member functions
    nsymbols sym;

    sym = "orange", "banana";
    sym.add("pineapple",1234);
    sym.add("lemon");

    parse("orange", sym[store(12345)]);
    parse("orange", sym[docheck(12345)]);
    parse("pineapple", sym[docheck(1234)]);
    parse("banana", sym[docheck(int())]);
    parse("lemon", sym[docheck(int())]);
}

static void
narrow_free_functions_tests()
{
    nsymbols sym;

#define add_returned_non_null_value (res!=0)
#define add_returned_null (res==0)
#define find_returned_non_null_value (res!=0)
#define find_returned_null (res==0)

    int *res = add(sym,"pineapple");
    BOOST_TEST(add_returned_non_null_value);
    res = add(sym,"pineapple");
    BOOST_TEST(add_returned_null);

    res = find(sym, "pineapple");
    BOOST_TEST(find_returned_non_null_value);
    res = find(sym, "banana");
    BOOST_TEST(find_returned_null);
}

static void
wide_match_tests()
{
    wsymbols sym;
    sym = L"pineapple", L"orange", L"banana", L"applepie", L"apple";

    docheck(sym, L"pineapple", true, L"", 9);
    docheck(sym, L"orange", true, L"", 6);
    docheck(sym, L"banana", true, L"", 6);
    docheck(sym, L"apple", true, L"", 5);
    docheck(sym, L"pizza", false, L"pizza", -1);
    docheck(sym, L"steak", false, L"steak", -1);
    docheck(sym, L"applepie", true, L"", 8);
    docheck(sym, L"bananarama", true, L"rama", 6);
    docheck(sym, L"applet", true, L"t", 5);
    docheck(sym, L"applepi", true, L"pi", 5);
    docheck(sym, L"appl", false, L"appl", -1);

    docheck(sym, L"pineapplez", true, L"z", 9);
    docheck(sym, L"orangez", true, L"z", 6);
    docheck(sym, L"bananaz", true, L"z", 6);
    docheck(sym, L"applez", true, L"z", 5);
    docheck(sym, L"pizzaz", false, L"pizzaz", -1);
    docheck(sym, L"steakz", false, L"steakz", -1);
    docheck(sym, L"applepiez", true, L"z", 8);
    docheck(sym, L"bananaramaz", true, L"ramaz", 6);
    docheck(sym, L"appletz", true, L"tz", 5);
    docheck(sym, L"applepix", true, L"pix", 5);
}

static void
wide_copy_ctor_tests()
{
    wsymbols sym;
    sym = L"pineapple", L"orange", L"banana", L"applepie", L"apple";

    wsymbols sym2(sym);
    docheck(sym2, L"pineapple", true, L"", 9);
    docheck(sym2, L"pizza", false, L"pizza", -1);
    docheck(sym2, L"bananarama", true, L"rama", 6);
}

static void
wide_assigment_operator_tests()
{
    wsymbols sym;
    sym = L"pineapple", L"orange", L"banana", L"applepie", L"apple";

    wsymbols sym2;
    sym2 = sym;

    docheck(sym2, L"pineapple", true, L"", 9);
    docheck(sym2, L"pizza", false, L"pizza", -1);
    docheck(sym2, L"bananarama", true, L"rama", 6);
}

static void
wide_swap_tests()
{
    wsymbols sym, sym2;
    sym = L"pineapple", L"orange", L"banana", L"applepie", L"apple";
    sym2 = L"potato", L"cucumber", L"cauliflower", L"carrot";

    boost::swap(sym, sym2);

    docheck(sym2, L"pineapple", true, L"", 9);
    docheck(sym2, L"pizza", false, L"pizza", -1);
    docheck(sym2, L"bananarama", true, L"rama", 6);
    docheck(sym, L"potatoe", true, L"e", 6);
    docheck(sym, L"cauliflour", false, L"cauliflour", -1);
}

static void
wide_value_tests()
{   // also tests the add member functions
    wsymbols sym;

    sym = L"orange", L"banana";
    sym.add(L"pineapple",1234);
    sym.add(L"lemon");

    parse(L"orange", sym[store(12345)]);
    parse(L"orange", sym[docheck(12345)]);
    parse(L"pineapple", sym[docheck(1234)]);
    parse(L"banana", sym[docheck(int())]);
    parse(L"lemon", sym[docheck(int())]);
}

static void
wide_free_functions_tests()
{
    wsymbols sym;

    int *res = add(sym,L"pineapple");
    BOOST_TEST(add_returned_non_null_value);
    res = add(sym,L"pineapple");
    BOOST_TEST(add_returned_null);

    res = find(sym, L"pineapple");
    BOOST_TEST(find_returned_non_null_value);
    res = find(sym, L"banana");
    BOOST_TEST(find_returned_null);
}

static 
void free_add_find_functions_tests()
{
    nsymbols sym;
    BOOST_TEST(*add(sym, "a", 0) == 0);
    BOOST_TEST(*add(sym, "a2", 1) == 1);
    BOOST_TEST(add(sym, "a2", 2) == 0);
    BOOST_TEST(find(sym, "a2"));
    BOOST_TEST(find(sym, "a"));
}

// The original teneray search tree implementation contained a bug when
// inserting duplicate values. I want this implementation to be as
// close as possible to the original (so they can be easily switched)
// so check that the bug remains the same.

struct check_parse_value
{
    explicit check_parse_value(int value) : value_(value){}
    
    void operator()(int value) const { BOOST_TEST(value == value_); }
    
    int value_;
};

// My version is different to the original, if there's an existing value
// it replaces it with the new one.

static
void duplicate_add_tests()
{
    char const* foo1 = "foo";
    char const* foo2 = foo1 + 3;

    nsymbols sym;
    sym.add(foo1, foo2, 1);
    nsymbols sym2 = sym;
    sym.add(foo1, foo2, 2);
    sym2.add(foo1, foo2, 3);
    
    BOOST_TEST(find(sym, "foo") && *find(sym, "foo") == 2);
    BOOST_TEST(find(sym2, "foo") && *find(sym2, "foo") == 3);

    parse_info<char const*> info;

    info = parse("foo ", sym[check_parse_value(2)]);
    BOOST_TEST(info.hit && info.length == 3);

    info = parse("foo", sym[check_parse_value(2)]);
    BOOST_TEST(info.hit && info.length == 3);

    info = parse("foo ", sym2[check_parse_value(3)]);
    BOOST_TEST(info.hit && info.length == 3);

    info = parse("foo", sym2[check_parse_value(3)]);
    BOOST_TEST(info.hit && info.length == 3);
}

int
main()
{
    default_constructible();
    narrow_match_tests();
    narrow_copy_ctor_tests();
    narrow_assigment_operator_tests();
    narrow_swap_tests();
    narrow_value_tests();
    narrow_free_functions_tests();
    wide_match_tests();
    wide_copy_ctor_tests();
    wide_assigment_operator_tests();
    wide_swap_tests();
    wide_value_tests();
    wide_free_functions_tests();
    free_add_find_functions_tests();
    duplicate_add_tests();

    return boost::report_errors();
}

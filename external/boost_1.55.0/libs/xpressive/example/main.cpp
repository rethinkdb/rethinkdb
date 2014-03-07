///////////////////////////////////////////////////////////////////////////////
// main.hpp
//
//  Copyright 2004 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <boost/xpressive/xpressive.hpp>

using namespace boost::xpressive;

///////////////////////////////////////////////////////////////////////////////
// Displays nested results to std::cout with indenting
//
// Display a tree of nested results
//
// Here is a helper class to demonstrate how you might display a tree of nested results:
struct output_nested_results
{
    int tabs_;

    output_nested_results(int tabs = 0)
      : tabs_(tabs)
    {
    }

    template< typename BidiIterT >
    void operator ()( match_results< BidiIterT > const &what ) const
    {
        // first, do some indenting
        typedef typename std::iterator_traits< BidiIterT >::value_type char_type;
        char_type space_ch = char_type(' ');
        std::fill_n( std::ostream_iterator<char_type>( std::cout ), tabs_ * 4, space_ch );

        // output the match
        std::cout << what[0] << '\n';

        // output any nested matches
        std::for_each(
            what.nested_results().begin(),
            what.nested_results().end(),
            output_nested_results( tabs_ + 1 ) );
    }
};

///////////////////////////////////////////////////////////////////////////////
// See if a whole string matches a regex
//
// This program outputs the following:
//
// hello world!
// hello
// world

void example1()
{
    std::string hello( "hello world!" );

    sregex rex = sregex::compile( "(\\w+) (\\w+)!" );
    smatch what;

    if( regex_match( hello, what, rex ) )
    {
        std::cout << what[0] << '\n'; // whole match
        std::cout << what[1] << '\n'; // first capture
        std::cout << what[2] << '\n'; // second capture
    }
}

///////////////////////////////////////////////////////////////////////////////
// See if a string contains a sub-string that matches a regex
//
// Notice in this example how we use custom mark_tags to make the pattern
// more readable. We can use the mark_tags later to index into the match_results<>.
//
// This program outputs the following:
// 
// 5/30/1973
// 30
// 5
// 1973
// /

void example2()
{
    char const *str = "I was born on 5/30/1973 at 7am.";

    // define some custom mark_tags with names more meaningful than s1, s2, etc.
    mark_tag day(1), month(2), year(3), delim(4);

    // this regex finds a date
    cregex date = (month= repeat<1,2>(_d))           // find the month ...
               >> (delim= (set= '/','-'))            // followed by a delimiter ...
               >> (day=   repeat<1,2>(_d)) >> delim  // and a day followed by the same delimiter ...
               >> (year=  repeat<1,2>(_d >> _d));    // and the year.

    cmatch what;

    if( regex_search( str, what, date ) )
    {
        std::cout << what[0]     << '\n'; // whole match
        std::cout << what[day]   << '\n'; // the day
        std::cout << what[month] << '\n'; // the month
        std::cout << what[year]  << '\n'; // the year
        std::cout << what[delim] << '\n'; // the delimiter
    }
}

///////////////////////////////////////////////////////////////////////////////
// Replace all sub-strings that match a regex
// 
// The following program finds dates in a string and marks them up with pseudo-HTML.
//
// This program outputs the following:
// 
// I was born on <date>5/30/1973</date> at 7am.

void example3()
{
    std::string str( "I was born on 5/30/1973 at 7am." );

    // essentially the same regex as in the previous example, but using a dynamic regex
    sregex date = sregex::compile( "(\\d{1,2})([/-])(\\d{1,2})\\2((?:\\d{2}){1,2})" );

    // As in Perl, $& is a reference to the sub-string that matched the regex
    std::string format( "<date>$&</date>" );

    str = regex_replace( str, date, format );
    std::cout << str << '\n';
}

///////////////////////////////////////////////////////////////////////////////
// Find all the sub-strings that match a regex and step through them one at a time
// 
// The following program finds the words in a wide-character string. It uses wsregex_iterator.
// Notice that dereferencing a wsregex_iterator yields a wsmatch object.
//
// This program outputs the following:
// 
// This
// is
// his
// face

void example4()
{
    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    std::wstring str( L"This is his face." );

    // find a whole word
    wsregex token = +alnum;

    wsregex_iterator cur( str.begin(), str.end(), token );
    wsregex_iterator end;

    for( ; cur != end; ++cur )
    {
        wsmatch const &what = *cur;
        std::wcout << what[0] << L'\n';
    }
    #endif
}

///////////////////////////////////////////////////////////////////////////////
// Split a string into tokens that each match a regex
//
// The following program finds race times in a string and displays first the minutes
// and then the seconds. It uses regex_token_iterator<>.
//
// This program outputs the following:
//
// 4
// 40
// 3
// 35
// 2
// 32

void example5()
{
    std::string str( "Eric: 4:40, Karl: 3:35, Francesca: 2:32" );

    // find a race time
    sregex time = sregex::compile( "(\\d):(\\d\\d)" );

    // for each match, the token iterator should first take the value of
    // the first marked sub-expression followed by the value of the second
    // marked sub-expression
    int const subs[] = { 1, 2 };

    sregex_token_iterator cur( str.begin(), str.end(), time, subs );
    sregex_token_iterator end;

    for( ; cur != end; ++cur )
    {
        std::cout << *cur << '\n';
    }
}

///////////////////////////////////////////////////////////////////////////////
// Split a string using a regex as a delimiter
//
// The following program takes some text that has been marked up with html and strips
// out the mark-up. It uses a regex that matches an HTML tag and a regex_token_iterator<>
// that returns the parts of the string that do not match the regex.
//
// This program outputs the following:
//
// {Now }{is the time }{for all good men}{ to come to the aid of their}{ country.}

void example6()
{
    std::string str( "Now <bold>is the time <i>for all good men</i> to come to the aid of their</bold> country." );

    // find an HTML tag
    sregex html = '<' >> optional('/') >> +_w >> '>';

    // the -1 below directs the token iterator to display the parts of
    // the string that did NOT match the regular expression.
    sregex_token_iterator cur( str.begin(), str.end(), html, -1 );
    sregex_token_iterator end;

    for( ; cur != end; ++cur )
    {
        std::cout << '{' << *cur << '}';
    }
    std::cout << '\n';
}

///////////////////////////////////////////////////////////////////////////////
// main
int main()
{
    std::cout << "\n\nExample 1:\n\n";
    example1();

    std::cout << "\n\nExample 2:\n\n";
    example2();

    std::cout << "\n\nExample 3:\n\n";
    example3();

    std::cout << "\n\nExample 4:\n\n";
    example4();

    std::cout << "\n\nExample 5:\n\n";
    example5();

    std::cout << "\n\nExample 6:\n\n";
    example6();

    std::cout << "\n\n" << std::flush;

    return 0;
}

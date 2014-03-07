/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>
#include <boost/spirit/include/classic_if.hpp>
#include <boost/spirit/include/classic_for.hpp>
#include <boost/spirit/include/phoenix1.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
//
//  Please check it out ipv4.cpp sample first!
//  << See ipv4.cpp sample for details >>
//
//  This is a variation of the ipv4.cpp sample. The original ipv4.cpp code
//  compiles to 36k on MSVC7.1. Not bad! Yet, we want to shave a little bit
//  more. Is it possible? Yes! This time, we'll use subrules and just store
//  the rules in a plain old struct. We are parsing at the char level anyway,
//  so we know what type of rule we'll need: a plain rule<>. The result: we
//  shaved off another 20k. Now the code compiles to 16k on MSVC7.1.
//
//  Could we have done better? Yes, but only if only we had typeof! << See
//  the techniques section of the User's guide >> ... Someday... :-)
//
///////////////////////////////////////////////////////////////////////////////
using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;
using namespace phoenix;

struct ipv4_prefix_data
{
    char prefix_len, n0, n1, n2, n3;

    ipv4_prefix_data()
        : prefix_len(0),n0(0),n1(0),n2(0),n3(0) {}
};

struct ipv4_data
{
    char packet_len, header_len;
    std::string header;
    std::vector<ipv4_prefix_data> prefixes;

    ipv4_data()
        : packet_len(0),header_len(0){}

};

struct ipv4
{
    ipv4(ipv4_data& data)
        : data(data)
    {
        start =
        (
            packet =
                '\xff'
                >> anychar_p[var(data.packet_len) = arg1]
                >> payload
            ,

            payload =
                anychar_p[var(data.header_len) = arg1]
                >>  for_p(var(i) = 0, var(i) < var(data.header_len), ++var(i))
                    [
                        anychar_p[var(data.header) += arg1]
                    ]
                >> *ipv4_prefix
             ,

            ipv4_prefix =
                anychar_p
                [
                    var(temp.prefix_len) = arg1,
                    var(temp.n0) = 0,
                    var(temp.n1) = 0,
                    var(temp.n2) = 0,
                    var(temp.n3) = 0
                ]

                >>  if_p(var(temp.prefix_len) > 0x00)
                    [
                        anychar_p[var(temp.n0) = arg1]
                        >>  if_p(var(temp.prefix_len) > 0x08)
                            [
                                anychar_p[var(temp.n1) = arg1]
                                >>  if_p(var(temp.prefix_len) > 0x10)
                                    [
                                        anychar_p[var(temp.n2) = arg1]
                                        >>  if_p(var(temp.prefix_len) > 0x18)
                                            [
                                                anychar_p[var(temp.n3) = arg1]
                                            ]
                                    ]
                            ]
                    ]
                    [
                        push_back_a(data.prefixes, temp)
                    ]
        );
    }

    int i;
    ipv4_prefix_data temp;

    rule<> start;
    subrule<0> packet;
    subrule<1> payload;
    subrule<2> ipv4_prefix;
    ipv4_data& data;
};

////////////////////////////////////////////////////////////////////////////
//
//  Main program
//
////////////////////////////////////////////////////////////////////////////
int
as_byte(char n)
{
    if (n < 0)
        return n + 256;
    return n;
}

void
print_prefix(ipv4_prefix_data const& prefix)
{
    cout << "prefix length = " << as_byte(prefix.prefix_len) << endl;
    cout << "n0 = " << as_byte(prefix.n0) << endl;
    cout << "n1 = " << as_byte(prefix.n1) << endl;
    cout << "n2 = " << as_byte(prefix.n2) << endl;
    cout << "n3 = " << as_byte(prefix.n3) << endl;
}

void
parse_ipv4(char const* str, unsigned len)
{
    ipv4_data data;
    ipv4 g(data);
    parse_info<> info = parse(str, str+len, g.start);

    if (info.full)
    {
        cout << "-------------------------\n";
        cout << "Parsing succeeded\n";

        cout << "packet length = " << as_byte(data.packet_len) << endl;
        cout << "header length = " << as_byte(data.header_len) << endl;
        cout << "header = " << data.header << endl;

        for_each(data.prefixes.begin(), data.prefixes.end(), print_prefix);
        cout << "-------------------------\n";
    }
    else
    {
        cout << "Parsing failed\n";
        cout << "stopped at:";
        for (char const* s = info.stop; s != str+len; ++s)
            cout << static_cast<int>(*s) << endl;
    }
}

// Test inputs:

// The string in the header is "empty", the prefix list is empty.
char const i1[8] =
{
    0xff,0x08,0x05,
    'e','m','p','t','y'
};

// The string in the header is "default route", the prefix list
// has just one element, 0.0.0.0/0.
char const i2[17] =
{
    0xff,0x11,0x0d,
    'd','e','f','a','u','l','t',' ',
    'r','o','u','t','e',
    0x00
};

// The string in the header is "private address space", the prefix list
// has the elements 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16.
char const i3[32] =
{
    0xff,0x20,0x15,
    'p','r','i','v','a','t','e',' ',
    'a','d','d','r','e','s','s',' ',
    's','p','a','c','e',
    0x08,0x0a,
    0x0c,0xac,0x10,
    0x10,0xc0,0xa8
};

int
main()
{
    parse_ipv4(i1, sizeof(i1));
    parse_ipv4(i2, sizeof(i2));
    parse_ipv4(i3, sizeof(i3));
    return 0;
}


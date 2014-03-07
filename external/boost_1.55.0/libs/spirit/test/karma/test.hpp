//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_TEST_FEB_23_2007_1221PM)
#define BOOST_SPIRIT_KARMA_TEST_FEB_23_2007_1221PM

#include <cstring>
#include <string>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <typeinfo>

#include <boost/foreach.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_what.hpp>

namespace spirit_test
{
    ///////////////////////////////////////////////////////////////////////////
    struct display_type
    {
        template<typename T>
        void operator()(T const &) const
        {
            std::cout << typeid(T).name() << std::endl;
        }

        template<typename T>
        static void print() 
        {
            std::cout << typeid(T).name() << std::endl;
        }
    };

    display_type const display = {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    struct output_iterator
    {
        typedef std::basic_string<Char> string_type;
        typedef std::back_insert_iterator<string_type> type;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename T>
    void print_if_failed(char const* func, bool result
      , std::basic_string<Char> const& generated, T const& expected)
    {
        if (!result)
            std::cerr << "in " << func << ": result is false" << std::endl;
        else if (generated != expected)
            std::cerr << "in " << func << ": generated \""
                << std::string(generated.begin(), generated.end())
                << "\"" << std::endl;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename T>
    void print_binary_if_failed(char const* func, bool result
      , std::basic_string<Char> const& generated, T const& expected)
    {
        if (!result)
            std::cerr << "in " << func << ": result is false" << std::endl;
        else if (generated.size() != expected.size() ||
                 std::memcmp(generated.c_str(), expected.c_str(), generated.size()))
        {
            std::cerr << "in " << func << ": generated \"";
            BOOST_FOREACH(int c, generated)
                std::cerr << "\\x" << std::hex << std::setfill('0') << std::setw(2) << c;
            std::cerr << "\"" << std::endl;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator>
    inline bool test(Char const *expected, Generator const& g)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate(outit, g);

        print_if_failed("test", result, generated, expected);
        return result && generated == expected;
    }

    template <typename Char, typename Generator>
    inline bool test(std::basic_string<Char> const& expected, Generator const& g)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate(outit, g);

        print_if_failed("test", result, generated, expected);
        return result && generated == expected;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator, typename Attribute>
    inline bool test(Char const *expected, Generator const& g, 
        Attribute const &attrib)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate(outit, g, attrib);

        print_if_failed("test", result, generated, expected);
        return result && generated == expected;
    }

    template <typename Char, typename Generator, typename Attribute>
    inline bool test(std::basic_string<Char> const& expected, Generator const& g, 
        Attribute const &attrib)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate(outit, g, attrib);

        print_if_failed("test", result, generated, expected);
        return result && generated == expected;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator, typename Delimiter>
    inline bool test_delimited(Char const *expected, Generator const& g, 
        Delimiter const& d)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate_delimited(outit, g, d);

        print_if_failed("test_delimited", result, generated, expected);
        return result && generated == expected;
    }

    template <typename Char, typename Generator, typename Delimiter>
    inline bool test_delimited(std::basic_string<Char> const& expected, 
        Generator const& g, Delimiter const& d)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate_delimited(outit, g, d);

        print_if_failed("test_delimited", result, generated, expected);
        return result && generated == expected;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Char, typename Generator, typename Attribute,
        typename Delimiter>
    inline bool test_delimited(Char const *expected, Generator const& g, 
        Attribute const &attrib, Delimiter const& d)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate_delimited(outit, g, d, attrib);

        print_if_failed("test_delimited", result, generated, expected);
        return result && generated == expected;
    }

    template <typename Char, typename Generator, typename Attribute,
        typename Delimiter>
    inline bool test_delimited(std::basic_string<Char> const& expected, 
        Generator const& g, Attribute const &attrib, Delimiter const& d)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<Char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate_delimited(outit, g, d, attrib);

        print_if_failed("test_delimited", result, generated, expected);
        return result && generated == expected;
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Generator>
    inline bool 
    binary_test(char const *expected, std::size_t size, 
        Generator const& g)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate(outit, g);

        print_binary_if_failed("binary_test", result, generated
          , std::string(expected, size));
        return result && !std::memcmp(generated.c_str(), expected, size);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Generator, typename Attribute>
    inline bool 
    binary_test(char const *expected, std::size_t size, 
        Generator const& g, Attribute const &attrib)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate(outit, g, attrib);

        print_binary_if_failed("binary_test", result, generated
          , std::string(expected, size));
        return result && !std::memcmp(generated.c_str(), expected, size);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Generator, typename Delimiter>
    inline bool 
    binary_test_delimited(char const *expected, std::size_t size, 
        Generator const& g, Delimiter const& d)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate_delimited(outit, g, d);

        print_binary_if_failed("binary_test_delimited", result, generated
          , std::string(expected, size));
        return result && !std::memcmp(generated.c_str(), expected, size);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Generator, typename Attribute, typename Delimiter>
    inline bool 
    binary_test_delimited(char const *expected, std::size_t size, 
        Generator const& g, Attribute const &attrib, Delimiter const& d)
    {
        namespace karma = boost::spirit::karma;
        typedef std::basic_string<char> string_type;

        // we don't care about the result of the "what" function.
        // we only care that all generators have it:
        karma::what(g);

        string_type generated;
        std::back_insert_iterator<string_type> outit(generated);
        bool result = karma::generate_delimited(outit, g, d, attrib);

        print_binary_if_failed("binary_test_delimited", result, generated
          , std::string(expected, size));
        return result && !std::memcmp(generated.c_str(), expected, size);
    }

}   // namespace spirit_test

#endif // !BOOST_SPIRIT_KARMA_TEST_FEB_23_2007_1221PM

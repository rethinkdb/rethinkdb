/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   named_scope_format_parser.cpp
 * \author Andrey Semashev
 * \date   14.11.2012
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <boost/move/core.hpp>
#include <boost/move/utility.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/utility/formatting_ostream.hpp>
#include <boost/log/detail/header.hpp>

namespace karma = boost::spirit::karma;

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace expressions {

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

template< typename CharT >
class named_scope_formatter
{
    BOOST_COPYABLE_AND_MOVABLE_ALT(named_scope_formatter)

public:
    typedef void result_type;

    typedef CharT char_type;
    typedef std::basic_string< char_type > string_type;
    typedef basic_formatting_ostream< char_type > stream_type;
    typedef attributes::named_scope::value_type::value_type value_type;

    struct literal
    {
        typedef void result_type;

        explicit literal(string_type& lit) { m_literal.swap(lit); }

        result_type operator() (stream_type& strm, value_type const&) const
        {
            strm << m_literal;
        }

    private:
        string_type m_literal;
    };

    struct scope_name
    {
        typedef void result_type;

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            strm << value.scope_name;
        }
    };

    struct file_name
    {
        typedef void result_type;

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            strm << value.file_name;
        }
    };

    struct line_number
    {
        typedef void result_type;

        result_type operator() (stream_type& strm, value_type const& value) const
        {
            strm.flush();
            typedef typename stream_type::streambuf_type streambuf_type;
            string_type& str = *static_cast< streambuf_type* >(strm.rdbuf())->storage();

            char_type buf[std::numeric_limits< unsigned int >::digits10 + 2];
            char_type* p = buf;

            typedef karma::uint_generator< unsigned int, 10 > uint_gen;
            karma::generate(p, uint_gen(), value.line);
            str.append(buf, p);
        }
    };

private:
    typedef boost::log::aux::light_function< void (stream_type&, value_type const&) > formatter_type;
    typedef std::vector< formatter_type > formatters;

private:
    formatters m_formatters;

public:
    BOOST_DEFAULTED_FUNCTION(named_scope_formatter(), {})
    named_scope_formatter(named_scope_formatter const& that) : m_formatters(that.m_formatters) {}
    named_scope_formatter(BOOST_RV_REF(named_scope_formatter) that) { m_formatters.swap(that.m_formatters); }

    named_scope_formatter& operator= (named_scope_formatter that)
    {
        this->swap(that);
        return *this;
    }

    result_type operator() (stream_type& strm, value_type const& value) const
    {
        for (typename formatters::const_iterator it = m_formatters.begin(), end = m_formatters.end(); strm.good() && it != end; ++it)
        {
            (*it)(strm, value);
        }
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template< typename FunT >
    void add_formatter(FunT&& fun)
    {
        m_formatters.emplace_back(boost::forward< FunT >(fun));
    }
#else
    template< typename FunT >
    void add_formatter(FunT const& fun)
    {
        m_formatters.push_back(formatter_type(fun));
    }
#endif

    void swap(named_scope_formatter& that)
    {
        m_formatters.swap(that.m_formatters);
    }
};

//! Parses the named scope format string and constructs the formatter function
template< typename CharT >
BOOST_FORCEINLINE boost::log::aux::light_function< void (basic_formatting_ostream< CharT >&, attributes::named_scope::value_type::value_type const&) >
do_parse_named_scope_format(const CharT* begin, const CharT* end)
{
    typedef CharT char_type;
    typedef boost::log::aux::light_function< void (basic_formatting_ostream< char_type >&, attributes::named_scope::value_type::value_type const&) > result_type;
    typedef named_scope_formatter< char_type > formatter_type;
    formatter_type fmt;

    std::basic_string< char_type > literal;

    while (begin != end)
    {
        const char_type* p = std::find(begin, end, static_cast< char_type >('%'));
        literal.append(begin, p);

        if ((end - p) >= 2)
        {
            switch (p[1])
            {
            case '%':
                literal.push_back(static_cast< char_type >('%'));
                break;

            case 'n':
                if (!literal.empty())
                    fmt.add_formatter(typename formatter_type::literal(literal));
                fmt.add_formatter(typename formatter_type::scope_name());
                break;

            case 'f':
                if (!literal.empty())
                    fmt.add_formatter(typename formatter_type::literal(literal));
                fmt.add_formatter(typename formatter_type::file_name());
                break;

            case 'l':
                if (!literal.empty())
                    fmt.add_formatter(typename formatter_type::literal(literal));
                fmt.add_formatter(typename formatter_type::line_number());
                break;

            default:
                literal.append(p, p + 2);
                break;
            }

            begin = p + 2;
        }
        else
        {
            if (p != end)
                literal.push_back(static_cast< char_type >('%')); // a single '%' character at the end of the string
            begin = end;
        }
    }

    if (!literal.empty())
        fmt.add_formatter(typename formatter_type::literal(literal));

    return result_type(boost::move(fmt));
}

} // namespace


#ifdef BOOST_LOG_USE_CHAR

//! Parses the named scope format string and constructs the formatter function
BOOST_LOG_API boost::log::aux::light_function< void (basic_formatting_ostream< char >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(const char* begin, const char* end)
{
    return do_parse_named_scope_format(begin, end);
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

//! Parses the named scope format string and constructs the formatter function
BOOST_LOG_API boost::log::aux::light_function< void (basic_formatting_ostream< wchar_t >&, attributes::named_scope::value_type::value_type const&) >
parse_named_scope_format(const wchar_t* begin, const wchar_t* end)
{
    return do_parse_named_scope_format(begin, end);
}

#endif // BOOST_LOG_USE_WCHAR_T

} // namespace aux

} // namespace expressions

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

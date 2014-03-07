/*==============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2010-2011 Bryce Lelbach

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file BOOST_LICENSE_1_0.rst or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_SPIRIT_UTREE_EXAMPLE_ERROR_HANDLER_HPP)
#define BOOST_SPIRIT_UTREE_EXAMPLE_ERROR_HANDLER_HPP

#include <string>
#include <sstream>

#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>

namespace sexpr
{

using boost::spirit::info;
 
template <typename Out>
struct print_info
{
    typedef boost::spirit::utf8_string string;

    print_info(Out& out) : out(out), first(true) {}

    void element(string const& tag, string const& value, int) const
    {
        if (!first) {
            out << ' ';
            first = false;
        }        

        if (value == "")
            out << tag;
        else
            out << "\"" << value << '"';
    }

    Out& out;
    mutable bool first;
};

struct expected_component : std::exception
{
    std::string msg;

    expected_component(std::string const& source, std::size_t line
                     , info const& w)
    {
        using boost::spirit::basic_info_walker;

        std::ostringstream oss;
        oss << "(exception \"" << source << "\" ";
      
        if (line == -1)
          oss << -1;
        else
          oss << line;

        oss << " '(expected_component (";

        print_info<std::ostringstream> pr(oss);
        basic_info_walker<print_info<std::ostringstream> >
        walker(pr, w.tag, 0);

        boost::apply_visitor(walker, w.value);

        oss << ")))";

        msg = oss.str();
    }

    virtual ~expected_component() throw() {}

    virtual char const* what() const throw()
    {
        return msg.c_str();
    }
};

template <typename Iterator>
struct error_handler
{
    template <typename, typename, typename, typename>
    struct result
    {
        typedef void type;
    };

    std::string source;

    error_handler(std::string const& source_ = "<string>") : source(source_) {}

    void operator()(Iterator first, Iterator last, Iterator err_pos
                  , info const& what) const
    {
        using boost::spirit::get_line;
        Iterator eol = err_pos;
        std::size_t line = get_line(err_pos);
        throw expected_component(source, line, what);
    }
};

} // sexpr

#endif // BOOST_SPIRIT_UTREE_EXAMPLE_ERROR_HANDLER_HPP


/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONJURE_ERROR_HANDLER_HPP)
#define BOOST_SPIRIT_CONJURE_ERROR_HANDLER_HPP

#include <iostream>
#include <string>
#include <vector>

namespace client
{
    ///////////////////////////////////////////////////////////////////////////////
    //  The error handler
    ///////////////////////////////////////////////////////////////////////////////
    template <typename BaseIterator, typename Iterator>
    struct error_handler
    {
        template <typename, typename, typename>
        struct result { typedef void type; };

        error_handler(BaseIterator first, BaseIterator last)
          : first(first), last(last) {}

        template <typename Message, typename What>
        void operator()(
            Message const& message,
            What const& what,
            Iterator err_pos) const
        {
            // retrieve underlying iterator from current token, err_pos points
            // to the last validly matched token, so we use its end iterator 
            // as the error position
            BaseIterator err_pos_base = err_pos->matched().end();
            std::cout << message << what << std::endl;
            if (err_pos_base != BaseIterator())
                dump_error_line(err_pos_base);
        }

        void dump_error_line(BaseIterator err_pos_base) const
        {
            int line;
            BaseIterator line_start = get_pos(err_pos_base, line);
            if (err_pos_base != last)
            {
                std::cout << " line " << line << ':' << std::endl;
                std::cout << get_line(line_start) << std::endl;
                for (; line_start != err_pos_base; ++line_start)
                    std::cout << ' ';
                std::cout << '^' << std::endl;
            }
            else
            {
                std::cout << "Unexpected end of file.\n";
            }
            
        }

        BaseIterator get_pos(BaseIterator err_pos, int& line) const
        {
            line = 1;
            BaseIterator i = first;
            BaseIterator line_start = first;
            while (i != err_pos)
            {
                bool eol = false;
                if (i != err_pos && *i == '\r') // CR
                {
                    eol = true;
                    line_start = ++i;
                }
                if (i != err_pos && *i == '\n') // LF
                {
                    eol = true;
                    line_start = ++i;
                }
                if (eol)
                    ++line;
                else
                    ++i;
            }
            return line_start;
        }

        std::string get_line(BaseIterator err_pos) const
        {
            BaseIterator i = err_pos;
            // position i to the next EOL
            while (i != last && (*i != '\r' && *i != '\n'))
                ++i;
            return std::string(err_pos, i);
        }

        BaseIterator first;
        BaseIterator last;
        std::vector<Iterator> iters;
    };
}

#endif


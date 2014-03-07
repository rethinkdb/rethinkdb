/*=============================================================================
    Copyright (c) 2005 2006 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_QUICKBOOK_POST_PROCESS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_POST_PROCESS_HPP

#include <string>
#include <stdexcept>

namespace quickbook
{
    std::string post_process(
        std::string const& in
      , int indent = -1
      , int linewidth = -1);

    struct post_process_failure : public std::runtime_error
    {
    public:
        explicit post_process_failure(std::string const& error)
            : std::runtime_error(error) {}
    };
}

#endif // BOOST_SPIRIT_QUICKBOOK_POST_PROCESS_HPP


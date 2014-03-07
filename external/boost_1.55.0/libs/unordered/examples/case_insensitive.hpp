
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This file implements a locale aware case insenstive equality predicate and
// hash function. Unfortunately it still falls short of full
// internationalization as it only deals with a single character at a time
// (some languages have tricky cases where the characters in an upper case
// string don't have a one-to-one correspondence with the lower case version of
// the text, eg. )

#if !defined(BOOST_HASH_EXAMPLES_CASE_INSENSITIVE_HEADER)
#define BOOST_HASH_EXAMPLES_CASE_INSENSITIVE_HEADER

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>

namespace hash_examples
{
    struct iequal_to
        : std::binary_function<std::string, std::string, bool>
    {
        iequal_to() {}
        explicit iequal_to(std::locale const& l) : locale_(l) {}

        template <typename String1, typename String2>
        bool operator()(String1 const& x1, String2 const& x2) const
        {
            return boost::algorithm::iequals(x1, x2, locale_);
        }
    private:
        std::locale locale_;
    };

    struct ihash
        : std::unary_function<std::string, std::size_t>
    {
        ihash() {}
        explicit ihash(std::locale const& l) : locale_(l) {}

        template <typename String>
        std::size_t operator()(String const& x) const
        {
            std::size_t seed = 0;

            for(typename String::const_iterator it = x.begin();
                it != x.end(); ++it)
            {
                boost::hash_combine(seed, std::toupper(*it, locale_));
            }

            return seed;
        }
    private:
        std::locale locale_;
    };
}

#endif

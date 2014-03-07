// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_PTREE_UTILS_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_PTREE_UTILS_HPP_INCLUDED

#include <boost/limits.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/mpl/and.hpp>
#include <string>
#include <algorithm>
#include <locale>

namespace boost { namespace property_tree { namespace detail
{

    template<class T>
    struct less_nocase
    {
        typedef typename T::value_type Ch;
        std::locale m_locale;
        inline bool operator()(Ch c1, Ch c2) const
        {
            return std::toupper(c1, m_locale) < std::toupper(c2, m_locale);
        }
        inline bool operator()(const T &t1, const T &t2) const
        {
            return std::lexicographical_compare(t1.begin(), t1.end(),
                                                t2.begin(), t2.end(), *this);
        }
    };

    template <typename Ch>
    struct is_character : public boost::false_type {};
    template <>
    struct is_character<char> : public boost::true_type {};
    template <>
    struct is_character<wchar_t> : public boost::true_type {};


    BOOST_MPL_HAS_XXX_TRAIT_DEF(internal_type)
    BOOST_MPL_HAS_XXX_TRAIT_DEF(external_type)
    template <typename T>
    struct is_translator : public boost::mpl::and_<
        has_internal_type<T>, has_external_type<T> > {};



    // Naively convert narrow string to another character type
    template<class Ch>
    std::basic_string<Ch> widen(const char *text)
    {
        std::basic_string<Ch> result;
        while (*text)
        {
            result += Ch(*text);
            ++text;
        }
        return result;
    }

    // Naively convert string to narrow character type
    template<class Ch>
    std::string narrow(const Ch *text)
    {
        std::string result;
        while (*text)
        {
            if (*text < 0 || *text > (std::numeric_limits<char>::max)())
                result += '*';
            else
                result += char(*text);
            ++text;
        }
        return result;
    }

    // Remove trailing and leading spaces
    template<class Ch>
    std::basic_string<Ch> trim(const std::basic_string<Ch> &s,
                               const std::locale &loc = std::locale())
    {
        typename std::basic_string<Ch>::const_iterator first = s.begin();
        typename std::basic_string<Ch>::const_iterator end = s.end();
        while (first != end && std::isspace(*first, loc))
            ++first;
        if (first == end)
            return std::basic_string<Ch>();
        typename std::basic_string<Ch>::const_iterator last = end;
        do --last; while (std::isspace(*last, loc));
        if (first != s.begin() || last + 1 != end)
            return std::basic_string<Ch>(first, last + 1);
        else
            return s;
    }

} } }

#endif

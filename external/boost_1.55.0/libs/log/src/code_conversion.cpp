/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   code_conversion.cpp
 * \author Andrey Semashev
 * \date   08.11.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#include <locale>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <boost/log/exceptions.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/detail/header.hpp>

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_LOG_ANONYMOUS_NAMESPACE {

    //! The function performs character conversion with the specified facet
    template< typename LocalCharT >
    inline std::codecvt_base::result convert(
        std::codecvt< LocalCharT, char, std::mbstate_t > const& fac,
        std::mbstate_t& state,
        const char*& pSrcBegin,
        const char* pSrcEnd,
        LocalCharT*& pDstBegin,
        LocalCharT* pDstEnd)
    {
        return fac.in(state, pSrcBegin, pSrcEnd, pSrcBegin, pDstBegin, pDstEnd, pDstBegin);
    }

    //! The function performs character conversion with the specified facet
    template< typename LocalCharT >
    inline std::codecvt_base::result convert(
        std::codecvt< LocalCharT, char, std::mbstate_t > const& fac,
        std::mbstate_t& state,
        const LocalCharT*& pSrcBegin,
        const LocalCharT* pSrcEnd,
        char*& pDstBegin,
        char* pDstEnd)
    {
        return fac.out(state, pSrcBegin, pSrcEnd, pSrcBegin, pDstBegin, pDstEnd, pDstBegin);
    }

} // namespace

template< typename SourceCharT, typename TargetCharT, typename FacetT >
inline void code_convert(const SourceCharT* begin, const SourceCharT* end, std::basic_string< TargetCharT >& converted, FacetT const& fac)
{
    typedef typename FacetT::state_type state_type;
    TargetCharT converted_buffer[256];

    state_type state = state_type();
    while (begin != end)
    {
        TargetCharT* dest = converted_buffer;
        std::codecvt_base::result res = convert(
            fac,
            state,
            begin,
            end,
            dest,
            dest + sizeof(converted_buffer) / sizeof(*converted_buffer));

        switch (res)
        {
        case std::codecvt_base::ok:
            // All characters were successfully converted
            // NOTE: MSVC 11 also returns ok when the source buffer was only partially consumed, so we also check that the begin pointer has reached the end.
            converted.append(converted_buffer, dest);
            break;

        case std::codecvt_base::partial:
            // Some characters were converted, some were not
            if (dest != converted_buffer)
            {
                // Some conversion took place, so it seems like
                // the destination buffer might not have been long enough
                converted.append(converted_buffer, dest);

                // ...and go on for the next part
                break;
            }
            else
            {
                // Nothing was converted, looks like the tail of the
                // source buffer contains only part of the last character.
                // Leave it as it is.
                return;
            }

        case std::codecvt_base::noconv:
            // Not possible, unless both character types are actually equivalent
            converted.append(reinterpret_cast< const TargetCharT* >(begin), reinterpret_cast< const TargetCharT* >(end));
            return;

        default: // std::codecvt_base::error
            BOOST_LOG_THROW_DESCR(conversion_error, "Could not convert character encoding");
        }
    }
}

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert(const wchar_t* str1, std::size_t len, std::string& str2, std::locale const& loc)
{
    code_convert(str1, str1 + len, str2, std::use_facet< std::codecvt< wchar_t, char, std::mbstate_t > >(loc));
}

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert(const char* str1, std::size_t len, std::wstring& str2, std::locale const& loc)
{
    code_convert(str1, str1 + len, str2, std::use_facet< std::codecvt< wchar_t, char, std::mbstate_t > >(loc));
}

#if !defined(BOOST_NO_CXX11_CHAR16_T)

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert(const char16_t* str1, std::size_t len, std::string& str2, std::locale const& loc)
{
    code_convert(str1, str1 + len, str2, std::use_facet< std::codecvt< char16_t, char, std::mbstate_t > >(loc));
}

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert(const char* str1, std::size_t len, std::u16string& str2, std::locale const& loc)
{
    code_convert(str1, str1 + len, str2, std::use_facet< std::codecvt< char16_t, char, std::mbstate_t > >(loc));
}

#endif

#if !defined(BOOST_NO_CXX11_CHAR32_T)

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert(const char32_t* str1, std::size_t len, std::string& str2, std::locale const& loc)
{
    code_convert(str1, str1 + len, str2, std::use_facet< std::codecvt< char32_t, char, std::mbstate_t > >(loc));
}

//! The function converts one string to the character type of another
BOOST_LOG_API void code_convert(const char* str1, std::size_t len, std::u32string& str2, std::locale const& loc)
{
    code_convert(str1, str1 + len, str2, std::use_facet< std::codecvt< char32_t, char, std::mbstate_t > >(loc));
}

#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attachable_sstream_buf.hpp
 * \author Andrey Semashev
 * \date   29.07.2007
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_
#define BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_

#include <memory>
#include <string>
#include <streambuf>
#include <boost/assert.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

//! A streambuf that puts the formatted data to an external string
template<
    typename CharT,
    typename TraitsT = std::char_traits< CharT >,
    typename AllocatorT = std::allocator< CharT >
>
class basic_ostringstreambuf :
    public std::basic_streambuf< CharT, TraitsT >
{
    //! Self type
    typedef basic_ostringstreambuf< CharT, TraitsT, AllocatorT > this_type;
    //! Base type
    typedef std::basic_streambuf< CharT, TraitsT > base_type;

    //! Buffer size
    enum { buffer_size = 16 };

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Traits type
    typedef typename base_type::traits_type traits_type;
    //! String type
    typedef std::basic_string< char_type, traits_type, AllocatorT > string_type;
    //! Int type
    typedef typename base_type::int_type int_type;

private:
    //! A reference to the string that will be filled
    string_type* m_Storage;
    //! A buffer used to temporarily store output
    char_type m_Buffer[buffer_size];

public:
    //! Constructor
    explicit basic_ostringstreambuf() : m_Storage(0)
    {
        base_type::setp(m_Buffer, m_Buffer + (sizeof(m_Buffer) / sizeof(*m_Buffer)));
    }
    //! Constructor
    explicit basic_ostringstreambuf(string_type& storage) : m_Storage(boost::addressof(storage))
    {
        base_type::setp(m_Buffer, m_Buffer + (sizeof(m_Buffer) / sizeof(*m_Buffer)));
    }

    //! Clears the buffer to the initial state
    void clear()
    {
        register char_type* pBase = this->pbase();
        register char_type* pPtr = this->pptr();
        if (pBase != pPtr)
            this->pbump(static_cast< int >(pBase - pPtr));
    }

    //! Detaches the buffer from the string
    void detach()
    {
        if (m_Storage)
        {
            this_type::sync();
            m_Storage = 0;
        }
    }

    //! Attaches the buffer to another string
    void attach(string_type& storage)
    {
        detach();
        m_Storage = boost::addressof(storage);
    }

    //! Returns a pointer to the attached string
    string_type* storage() const { return m_Storage; }

protected:
    //! Puts all buffered data to the string
    int sync()
    {
        BOOST_ASSERT(m_Storage != 0);
        register char_type* pBase = this->pbase();
        register char_type* pPtr = this->pptr();
        if (pBase != pPtr)
        {
            m_Storage->append(pBase, pPtr);
            this->pbump(static_cast< int >(pBase - pPtr));
        }
        return 0;
    }
    //! Puts an unbuffered character to the string
    int_type overflow(int_type c)
    {
        BOOST_ASSERT(m_Storage != 0);
        basic_ostringstreambuf::sync();
        if (!traits_type::eq_int_type(c, traits_type::eof()))
        {
            m_Storage->push_back(traits_type::to_char_type(c));
            return c;
        }
        else
            return traits_type::not_eof(c);
    }
    //! Puts a character sequence to the string
    std::streamsize xsputn(const char_type* s, std::streamsize n)
    {
        BOOST_ASSERT(m_Storage != 0);
        basic_ostringstreambuf::sync();
        typedef typename string_type::size_type string_size_type;
        register const string_size_type max_storage_left =
            m_Storage->max_size() - m_Storage->size();
        if (static_cast< string_size_type >(n) < max_storage_left)
        {
            m_Storage->append(s, static_cast< string_size_type >(n));
            return n;
        }
        else
        {
            m_Storage->append(s, max_storage_left);
            return static_cast< std::streamsize >(max_storage_left);
        }
    }

    //! Copy constructor (closed)
    BOOST_DELETED_FUNCTION(basic_ostringstreambuf(basic_ostringstreambuf const& that))
    //! Assignment (closed)
    BOOST_DELETED_FUNCTION(basic_ostringstreambuf& operator= (basic_ostringstreambuf const& that))
};

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATTACHABLE_SSTREAM_BUF_HPP_INCLUDED_

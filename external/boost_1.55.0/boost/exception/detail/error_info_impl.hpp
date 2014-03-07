//Copyright (c) 2006-2010 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_CE6983AC753411DDA764247956D89593
#define UUID_CE6983AC753411DDA764247956D89593
#if (__GNUC__*100+__GNUC_MINOR__>301) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma GCC system_header
#endif
#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(push,1)
#endif

#include <string>

namespace
boost
    {
    namespace
    exception_detail
        {
        class
        error_info_base
            {
            public:

            virtual std::string name_value_string() const = 0;

            protected:

            virtual
            ~error_info_base() throw()
                {
                }
            };
        }

    template <class Tag,class T>
    class
    error_info:
        public exception_detail::error_info_base
        {
        public:

        typedef T value_type;

        error_info( value_type const & value );
        ~error_info() throw();

        value_type const &
        value() const
            {
            return value_;
            }

        value_type &
        value()
            {
            return value_;
            }

        private:

        std::string name_value_string() const;

        value_type value_;
        };
    }

#if defined(_MSC_VER) && !defined(BOOST_EXCEPTION_ENABLE_WARNINGS)
#pragma warning(pop)
#endif
#endif

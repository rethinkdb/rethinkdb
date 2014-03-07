//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_BC765EB4CA2A11DCBDC5828355D89593
#define UUID_BC765EB4CA2A11DCBDC5828355D89593

#include <boost/exception/exception.hpp>
#include <exception>

namespace
boost
    {
    namespace
    exception_test
        {
        struct
        derives_boost_exception:
            public boost::exception,
            public std::exception
            {
            explicit derives_boost_exception( int x );
            virtual ~derives_boost_exception() throw();
            int x_;
            };

        struct
        derives_boost_exception_virtually:
            public virtual boost::exception,
            public std::exception
            {
            explicit derives_boost_exception_virtually( int x );
            virtual ~derives_boost_exception_virtually() throw();
            int x_;
            };

        struct
        derives_std_exception:
            public std::exception
            {
            explicit derives_std_exception( int x );
            virtual ~derives_std_exception() throw();
            int x_;
            };

        template <class>
        void throw_test_exception( int );

        template <>
        void throw_test_exception<derives_boost_exception>( int );

        template <>
        void throw_test_exception<derives_boost_exception_virtually>( int );

        template <>
        void throw_test_exception<derives_std_exception>( int );
        }
    }

#endif

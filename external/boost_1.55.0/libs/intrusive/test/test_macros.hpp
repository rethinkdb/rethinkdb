/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_TEST_TEST_MACROS_HPP
#define BOOST_INTRUSIVE_TEST_TEST_MACROS_HPP

#define TEST_INTRUSIVE_SEQUENCE( INTVALUES, ITERATOR )\
{  \
   BOOST_TEST (std::equal(&INTVALUES[0], &INTVALUES[0] + sizeof(INTVALUES)/sizeof(INTVALUES[0]), ITERATOR) ); \
}

#define TEST_INTRUSIVE_SEQUENCE_EXPECTED( EXPECTEDVECTOR, ITERATOR )\
{  \
   BOOST_TEST (std::equal(EXPECTEDVECTOR.begin(), EXPECTEDVECTOR.end(), ITERATOR) ); \
}

#endif

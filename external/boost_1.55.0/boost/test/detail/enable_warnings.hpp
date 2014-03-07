//  (C) Copyright Gennadiy Rozental 2004-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision: 54633 $
//
//  Description : enable previosly suppressed warnings
// ***************************************************************************

#ifdef BOOST_MSVC
# pragma warning(default: 4511) // copy constructor can't not be generated
# pragma warning(default: 4512) // assignment operator can't not be generated
# pragma warning(default: 4100) // unreferenced formal parameter 
# pragma warning(default: 4996) // <symbol> was declared deprecated 
# pragma warning(default: 4355) // 'this' : used in base member initializer list
# pragma warning(default: 4706) // assignment within conditional expression
# pragma warning(default: 4251) // class 'A<T>' needs to have dll-interface to be used by clients of class 'B'
# pragma warning(default: 4127) // conditional expression is constant
# pragma warning(default: 4290) // C++ exception specification ignored except to ...
# pragma warning(default: 4180) // qualifier applied to function type has no meaning; ignored
# pragma warning(default: 4275) // non dll-interface class ... used as base for dll-interface class ...
# pragma warning(default: 4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
# pragma warning(default: 4511) // 'class' : copy constructor could not be generated
# pragma warning(pop)
#endif

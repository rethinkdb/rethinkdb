//  memory.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_HANDLES_HPP
#define BOOST_DETAIL_WINAPI_HANDLES_HPP

#include <boost/detail/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost
{
namespace detail
{
namespace winapi
{
#if defined( BOOST_USE_WINDOWS_H )
    using ::CloseHandle;
    using ::DuplicateHandle;
#else
extern "C" { 
    __declspec(dllimport) int __stdcall 
        CloseHandle(void*);
    __declspec(dllimport) int __stdcall 
        DuplicateHandle(void*,void*,void*,void**,unsigned long,int,unsigned long);
}

#endif
}
}
}

#endif // BOOST_DETAIL_WINAPI_HANDLES_HPP

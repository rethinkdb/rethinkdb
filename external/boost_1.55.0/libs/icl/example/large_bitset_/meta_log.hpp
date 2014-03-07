/*-----------------------------------------------------------------------------+    
Author: Joachim Faulhaber
Copyright (c) 2009-2009: Joachim Faulhaber
+------------------------------------------------------------------------------+
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENCE.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
+-----------------------------------------------------------------------------*/

namespace mini // minimal implementations for example projects
{
// A meta implementation of an the logarithm function on integrals
template <size_t Argument, size_t Base=2>
struct log_{ enum { value = 1 + log_<Argument/Base, Base>::value }; };

template <size_t Base>struct log_<1, Base>{ enum { value = 0 }; };
template <size_t Base>struct log_<0, Base>{ enum { value = 0 }; };

template <size_t Argument>
struct log2_{ enum { value = log_<Argument, 2>::value }; };

template <size_t Argument>
struct power2_{ enum { value = 1 << Argument }; };

} // namespace mini


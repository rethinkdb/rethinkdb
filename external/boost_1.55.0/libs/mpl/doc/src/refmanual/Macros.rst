
Being a *template* metaprogramming framework, the MPL concentrates on 
getting one thing done well and leaves most of the clearly 
preprocessor-related tasks to the corresponding specialized 
libraries [PRE]_, [Ve03]_. But whether we like it or not, macros play
an important role on today's C++ metaprogramming, and some of 
the useful MPL-level functionality cannot be implemented 
without leaking its preprocessor-dependent implementation 
nature into the library's public interface.
 

.. [PRE] Vesa Karvonen, Paul Mensonides, 
   `The Boost Preprocessor Metaprogramming library`__

__ http://www.boost.org/libs/preprocessor/doc/index.html

.. [Ve03] Vesa Karvonen, `The Order Programming Language`, 2003.


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. Data Types/Miscellaneous//void_ |100

void\_
======

Synopsis
--------

.. parsed-literal::
    
    struct void\_
    {
        typedef void\_ type;
    };

    template< typename T > struct is_void;


Description
-----------

``void_`` is a generic type placeholder representing "nothing". 

.. In many cases, returning ``void_`` from a metafunction to signal 
   an absence of the requested data leads to a simpler user code than 
   having a separate metafunction specifically for the purpose of 
   performing the corresponding check. 

Header
------

.. parsed-literal::
    
    #include <boost/mpl/void.hpp>


See also
--------

|Data Types|, |pair|, |empty_base|, |bool_|, |int_|, |integral_c|


.. copyright:: Copyright ©  2001-2009 Aleksey Gurtovoy and David Abrahams
   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

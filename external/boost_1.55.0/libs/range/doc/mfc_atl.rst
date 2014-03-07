
++++++++++++++++++++++++++++++++
 |Boost| Range MFC/ATL Extension
++++++++++++++++++++++++++++++++

.. |Boost| image:: http://www.boost.org/libs/ptr_container/doc/boost.png



:Author:        Shunsuke Sogame
:Contact:       mb2act@yahoo.co.jp
:date:          26th of May 2006
:copyright:     Shunsuke Sogame 2005-2006. Use, modification and distribution is subject to the Boost Software License, Version 1.0 (see LICENSE_1_0.txt__).

__ http://www.boost.org/LICENSE_1_0.txt



========
Overview
========

Boost.Range MFC/ATL Extension provides `Boost.Range`_ support for MFC/ATL collection and string types.


.. parsed-literal::

        CTypedPtrArray<CPtrArray, CList<CString> \*> myArray;
        ...
        BOOST_FOREACH (CList<CString> \*theList, myArray)
        {
            BOOST_FOREACH (CString& str, \*theList)
            {
                boost::to_upper(str);
                std::sort(boost::begin(str), boost::end(str));
                ...
            }
        }



* `Requirements`_
* `MFC Ranges`_
* `ATL Ranges`_
* `const Ranges`_
* `References`_



============
Requirements
============

- `Boost C++ Libraries Version 1.34.0`__ or later (no compilation required)
- Visual C++ 7.1 or Visual C++ 8.0

__ Boost_



==========
MFC Ranges
==========

If the ``<boost/range/mfc.hpp>`` is included before or after `Boost.Range`_ headers,
the MFC collections and strings become models of Range.
The table below lists the Traversal Category and ``range_reference`` of MFC ranges.


============================= ================== =======================================
``Range``                     Traversal Category ``range_reference<Range>::type``
============================= ================== =======================================
``CArray<T,A>``               Random Access      ``T&``
----------------------------- ------------------ ---------------------------------------
``CList<T,A>``                Bidirectional      ``T&``
----------------------------- ------------------ ---------------------------------------
``CMap<K,AK,M,AM>``           Forward            ``Range::CPair&``
----------------------------- ------------------ ---------------------------------------
``CTypedPtrArray<B,T*>``      Random Access      ``T* const``
----------------------------- ------------------ ---------------------------------------
``CTypedPtrList<B,T*>``       Bidirectional      ``T* const``
----------------------------- ------------------ ---------------------------------------
``CTypedPtrMap<B,T*,V*>``     Forward            ``std::pair<T*,V*> const``
----------------------------- ------------------ ---------------------------------------
``CByteArray``                Random Access      ``BYTE&``
----------------------------- ------------------ ---------------------------------------
``CDWordArray``               Random Access      ``DWORD&``
----------------------------- ------------------ ---------------------------------------
``CObArray``                  Random Access      ``CObject* &``
----------------------------- ------------------ ---------------------------------------
``CPtrArray``                 Random Access      ``void* &``
----------------------------- ------------------ ---------------------------------------
``CStringArray``              Random Access      ``CString&``
----------------------------- ------------------ ---------------------------------------
``CUIntArray``                Random Access      ``UINT&``
----------------------------- ------------------ ---------------------------------------
``CWordArray``                Random Access      ``WORD&``
----------------------------- ------------------ ---------------------------------------
``CObList``                   Bidirectional      ``CObject* &``
----------------------------- ------------------ ---------------------------------------
``CPtrList``                  Bidirectional      ``void* &``
----------------------------- ------------------ ---------------------------------------
``CStringList``               Bidirectional      ``CString&``
----------------------------- ------------------ ---------------------------------------
``CMapPtrToWord``             Forward            ``std::pair<void*,WORD> const``
----------------------------- ------------------ ---------------------------------------
``CMapPtrToPtr``              Forward            ``std::pair<void*,void*> const``
----------------------------- ------------------ ---------------------------------------
``CMapStringToOb``            Forward            ``std::pair<String,CObject*> const``
----------------------------- ------------------ ---------------------------------------
``CMapStringToString``        Forward            ``Range::CPair&``
----------------------------- ------------------ ---------------------------------------
``CMapWordToOb``              Forward            ``std::pair<WORD,CObject*> const``
----------------------------- ------------------ ---------------------------------------
``CMapWordToPtr``             Forward            ``std::pair<WORD,void*> const``
============================= ================== =======================================


Other `Boost.Range`_ metafunctions are defined by the following.
Let ``Range`` be any type listed above and ``ReF`` be the same as ``range_reference<Range>::type``.
``range_value<Range>::type`` is the same as ``remove_reference<remove_const<Ref>::type>::type``,
``range_difference<Range>::type`` is the same as ``std::ptrdiff_t``, and
``range_pointer<Range>::type`` is the same as ``add_pointer<remove_reference<Ref>::type>::type``.
As for ``const Range``, see `const Ranges`_.



==========
ATL Ranges
==========

If the ``<boost/range/atl.hpp>`` is included before or after `Boost.Range`_ headers,
the ATL collections and strings become models of Range.
The table below lists the Traversal Category and ``range_reference`` of ATL ranges.


============================= ================== =======================================
``Range``                     Traversal Category ``range_reference<Range>::type``
============================= ================== =======================================
``CAtlArray<E,ET>``           Random Access      ``E&``
----------------------------- ------------------ ---------------------------------------
``CAutoPtrArray<E>``          Random Access      ``E&``
----------------------------- ------------------ ---------------------------------------
``CInterfaceArray<I,pi>``     Random Access      ``CComQIPtr<I,pi>&``
----------------------------- ------------------ ---------------------------------------
``CAtlList<E,ET>``            Bidirectional      ``E&``
----------------------------- ------------------ ---------------------------------------
``CAutoPtrList<E>``           Bidirectional      ``E&``
----------------------------- ------------------ ---------------------------------------
``CHeapPtrList<E,A>``         Bidirectional      ``E&``
----------------------------- ------------------ ---------------------------------------
``CInterfaceList<I,pi>``      Bidirectional      ``CComQIPtr<I,pi>&``
----------------------------- ------------------ ---------------------------------------
``CAtlMap<K,V,KT,VT>``        Forward            ``Range::CPair&``
----------------------------- ------------------ ---------------------------------------
``CRBTree<K,V,KT,VT>``        Bidirectional      ``Range::CPair&``
----------------------------- ------------------ ---------------------------------------
``CRBMap<K,V,KT,VT>``         Bidirectional      ``Range::CPair&``
----------------------------- ------------------ ---------------------------------------
``CRBMultiMap<K,V,KT,VT>``    Bidirectional      ``Range::CPair&``
----------------------------- ------------------ ---------------------------------------
``CSimpleStringT<B,b>``       Random Access      ``B&``
----------------------------- ------------------ ---------------------------------------
``CStringT<B,ST>``            Random Access      ``B&``
----------------------------- ------------------ ---------------------------------------
``CFixedStringT<S,n>``        Random Access      ``range_reference<S>::type``
----------------------------- ------------------ ---------------------------------------
``CStringT<B,ST>``            Random Access      ``B&``
----------------------------- ------------------ ---------------------------------------
``CComBSTR``                  Random Access      ``OLECHAR&``
----------------------------- ------------------ ---------------------------------------
``CSimpleArray<T,TE>``        Random Access      ``T&``
============================= ================== =======================================


Other `Boost.Range`_ metafunctions are defined by the following.
Let ``Range`` be any type listed above and ``ReF`` be the same as ``range_reference<Range>::type``.
``range_value<Range>::type`` is the same as ``remove_reference<Ref>::type``,
``range_difference<Range>::type`` is the same as ``std::ptrdiff_t``, and
``range_pointer<Range>::type`` is the same as ``add_pointer<remove_reference<Ref>::type>::type``.
As for ``const Range``, see `const Ranges`_.



============
const Ranges
============

``range_reference<const Range>::type`` is defined by the following algorithm.
Let ``Range`` be any type listed above and ``ReF`` be the same as ``range_reference<Range>::type``.


.. parsed-literal::

    if (Range is CObArray || Range is CObList)
        return CObject const \* &
    else if (Range is CPtrArray || Range is CPtrList)
        return void const \* &
    else if (there is a type X such that X& is the same as ReF)
        return X const &
    else if (there is a type X such that X* const is the same as ReF)
        return X const \* const
    else
        return ReF


Other `Boost.Range`_ metafunctions are defined by the following.
``range_value<const Range>::type`` is the same as ``range_value<Range>::type``,
``range_difference<const Range>::type`` is the same as ``std::ptrdiff_t``, and
``range_pointer<const Range>::type`` is the same as ``add_pointer<remove_reference<range_reference<const Range>::type>::type>::type``.



==========
References
==========
- `Boost.Range`_
- `MFC Collections`__
- `ATL Collection Classes`__

__ http://msdn2.microsoft.com/en-us/library/942860sh.aspx
__ http://msdn2.microsoft.com/en-US/library/15e672bd.aspx



.. _Boost C++ Libraries: http://www.boost.org/
.. _Boost: `Boost C++ Libraries`_
.. _Boost.Range: ../index.html
.. _forward: range.html#forward_range
.. _bidirectional: range.html#forward_range
.. _random access: range.html#random_access_range


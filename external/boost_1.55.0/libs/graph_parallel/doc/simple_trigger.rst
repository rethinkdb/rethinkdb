.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

============================
|Logo| Simple Triggers
============================

.. contents::

Introduction
------------

Triggers in the `process group`_ interface are used to asynchronously
receive and process messages destined for distributed data
structures. The trigger interface is relatively versatile, permitting
one to attach any function object to handle requests. The
``simple_trigger`` function simplifies a common case for triggers:
attaching a trigger that invokes a specific member function of the
distributed data structure.

Where Defined
-------------

Header ``<boost/graph/parallel/simple_trigger.hpp>``

Reference
---------

  ::

    template<typename ProcessGroup, typename Class, typename T>
      void 
      simple_trigger(ProcessGroup& pg, int tag, Class* self, 
                     void (Class::*pmf)(int source, int tag, const T& data, 
                                        trigger_receive_context context))

    template<typename ProcessGroup, typename Class, typename T, typename Result>
      void 
      simple_trigger(ProcessGroup& pg, int tag, Class* self, 
                     Result (Class::*pmf)(int source, int tag, const T& data, 
                                          trigger_receive_context context))

The ``simple_trigger`` function registers a trigger that invokes the
given member function (``pmf``) on the object ``self`` whenever a
message is received. If the member function has a return value, then
the trigger has a reply, and can only be used via out-of-band sends
that expect a reply. Otherwise, the member function returns ``void``,
and the function is registered as a normal trigger.


-----------------------------------------------------------------------------

Copyright (C) 2007 Douglas Gregor

Copyright (C) 2007 Matthias Troyer

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _process group: process_group.html

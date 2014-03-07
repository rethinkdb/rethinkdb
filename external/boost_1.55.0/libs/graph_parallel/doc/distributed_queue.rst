.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

================================
|Logo| Distributed queue adaptor
================================

::

  template<typename ProcessGroup, typename Buffer>
  class distributed_queue
  {
  public:
    typedef ProcessGroup                     process_group_type;
    typedef Buffer                           buffer_type;
    typedef typename buffer_type::value_type value_type;
    typedef typename buffer_type::size_type  size_type;

    explicit 
    distributed_queue(const ProcessGroup& process_group = ProcessGroup(), 
                      const Buffer& buffer = Buffer(),
                      bool polling = false);

    distributed_queue(const ProcessGroup& process_group, bool polling);

    void push(const value_type& x);
    void pop(); 
    value_type& top();
    const value_type& top() const;
    bool empty() const;
    size_type size() const;
  };

  template<typename ProcessGroup, typename Buffer>
  inline distributed_queue<ProcessGroup, Buffer>
  make_distributed_queue(const ProcessGroup& process_group, const Buffer& buffer,
                         bool polling = false);
  
Class template ``distributed_queue`` implements a distributed queue
across a process group. The distributed queue is an adaptor over an
existing (local) queue, which must model the Buffer_ concept. Each
process stores a distinct copy of the local queue, from which it draws
or removes elements via the ``pop`` and ``top`` members.

The value type of the local queue must be a model of the 
`Global Descriptor`_ concept. The ``push`` operation of the
distributed queue passes (via a message) the value to its owning
processor. Thus, the elements within a particular local queue are
guaranteed to have the process owning that local queue as an owner.

Synchronization of distributed queues occurs in the ``empty`` and
``size`` functions, which will only return "empty" values (true or 0,
respectively) when the entire distributed queue is empty. If the local
queue is empty but the distributed queue is not, the operation will
block until either condition changes. When the ``size`` function of a
nonempty queue returns, it returns the size of the local queue. These
semantics were selected so that sequential code that processes
elements in the queue via the following idiom can be parallelized via
introduction of a distributed queue:

::

  distributed_queue<...> Q;
  Q.push(x);
  while (!Q.empty()) {
    // do something, that may push a value onto Q
  }

In the parallel version, the initial ``push`` operation will place
the value ``x`` onto its owner's queue. All processes will
synchronize at the call to empty, and only the process owning ``x``
will be allowed to execute the loop (``Q.empty()`` returns
false). This iteration may in turn push values onto other remote
queues, so when that process finishes execution of the loop body
and all processes synchronize again in ``empty``, more processes
may have nonempty local queues to execute. Once all local queues
are empty, ``Q.empty()`` returns ``false`` for all processes.

The distributed queue can receive messages at two different times:
during synchronization and when polling ``empty``. Messages are
always received during synchronization, to ensure that accurate
local queue sizes can be determines. However, whether ``empty``
should poll for messages is specified as an option to the
constructor. Polling may be desired when the order in which
elements in the queue are processed is not important, because it
permits fewer synchronization steps and less communication
overhead. However, when more strict ordering guarantees are
required, polling may be semantically incorrect. By disabling
polling, one ensures that parallel execution using the idiom above
will not process an element at a later "level" before an earlier
"level". 

The distributed queue nearly models the Buffer_
concept. However, the ``push`` routine does not necessarily
increase the result of ``size()`` by one (although the size of the
global queue does increase by one).

Member Functions
----------------

::

  explicit 
  distributed_queue(const ProcessGroup& process_group = ProcessGroup(), 
                    const Buffer& buffer = Buffer(),
                    bool polling = false);

Build a new distributed queue that communicates over the given
``process_group``, whose local queue is initialized via ``buffer`` and
which may or may not poll for messages.

-----------------------------------------------------------------------------

::

  distributed_queue(const ProcessGroup& process_group, bool polling);

Build a new distributed queue that communicates over the given 
``process_group``, whose local queue is default-initalized and which
may or may not poll for messages.

-----------------------------------------------------------------------------

::

  void push(const value_type& x);

Push an element onto the distributed queue.

The element will be sent to its owner process to be added to that
process's local queue. If polling is enabled for this queue and
the owner process is the current process, the value will be
immediately pushed onto the local queue.

Complexity: O(1) messages of size O(``sizeof(value_type)``) will be
transmitted.


-----------------------------------------------------------------------------

::

  void pop();

Pop an element off the local queue. The queue must not be ``empty()``.

-----------------------------------------------------------------------------

::

  value_type& top();
  const value_type& top();

Returns the top element in the local queue. The queue must not be
``empty()``.

-----------------------------------------------------------------------------

::

  bool empty() const;

Determines if the queue is empty.

When the local queue is nonempty, returns true. If the local queue is
empty, synchronizes with all other processes in the process group
until either (1) the local queue is nonempty (returns true) (2) the
entire distributed queue is empty (returns false).

-----------------------------------------------------------------------------

::

  size_type size() const;


Determines the size of the local queue.

The behavior of this routine is equivalent to the behavior of
``empty``, except that when ``empty`` returns true this
function returns the size of the local queue and when ``empty``
returns false this function returns zero.

Free Functions
--------------

::

  template<typename ProcessGroup, typename Buffer>
  inline distributed_queue<ProcessGroup, Buffer>
  make_distributed_queue(const ProcessGroup& process_group, const Buffer& buffer,
                         bool polling = false);

Constructs a distributed queue.

-----------------------------------------------------------------------------

Copyright (C) 2004, 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Global descriptor: GlobalDescriptor.html
.. _Buffer: http://www.boost.org/libs/graph/doc/Buffer.html

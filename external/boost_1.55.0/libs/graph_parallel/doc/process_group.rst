.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

==================================
|Logo| Parallel BGL Process Groups
==================================

.. contents::

Introduction
------------

Process groups are an abstraction of a set of communicating processes
that coordinate to solve the same problem. Process groups contain
facilities for identifying the processes within that group, sending
and receiving messages between the processes in that group, and
performing collective communications involving all processes in the
group simultaneously. 

Communication model
-------------------

Process groups are based on an extended version of the Bulk
Synchronous Parallel (BSP) model of computation. Parallel computations
in the BSP model are organized into *supersteps*, each of which
consists of a computation phase followed by a communication
phase. During the computation phase, all processes in the process
group work exclusively on local data, and there is no inter-process
communication. During the communication phase, all of the processes
exchange message with each other. Messages sent in the communication
phase of a superstep will be received in the next superstep.

The boundary between supersteps in the Parallel BGL corresponds to the
``synchronize`` operation. Whenever a process has completed its local
computation phase and sent all of the messages required for that
superstep, it invokes the ``synchronize`` operation on the process
group. Once all processes in the process group have entered
``synchronize``, they exchange messages and then continue with the
next superstep. 

The Parallel BGL loosens the BSP model significantly, to provide a
more natural programming model that also provides some performance
benefits over the strict BSP model. The primary extension is the
ability to receive messages sent within the same superstep
"asynchronously", either to free up message buffers or to respond to
an immediate request for information. For particularly unstructured
computations, the ability to send a message and get an immediate reply
can simplify many computations that would otherwise need to be split
into two separate supersteps. Additionally, the Parallel BGL augments
the BSP model with support for multiple distributed data structures,
each of which are provided with a different communication space but
whose messages will all be synchronized concurrently. 

Distributed data structures
~~~~~~~~~~~~~~~~~~~~~~~~~~~

A typical computation with the Parallel BGL involves several
distributed data structures working in concern. For example, a simple
breadth-first search involves the distributed graph data structure
containing the graph itself, a distributed queue that manages the
traversal through the graph, and a distributed property map that
tracks which vertices have already been visited as part of the
search. 

The Parallel BGL manages these distributed data structures by allowing
each of the data structures to attach themselves to the process group
itself. When a distributed data structure attaches to the process
group, it receives its own copy of the process group that allows the
distributed data structure to communicate without colliding with the
communications from other distributed data structures. When the
process group is synchronized, all of the distributed data structures
attached to that process group are automatically synchronized, so that
all of the distributed data structures in a computation remain
synchronized. 

A distributed data structure attaches itself to the process group by
creating a copy of the process group and passing an
``attach_distributed_object`` flag to the process group
constructor. So long as this copy of the process group persists, the
distributed data structure is attached the process group. For this
reason, most distributed data structures keep a copy of the process
group as member data, constructing the member with
``attach_distributed_object``, e.g.,

::

  template<typename ProcessGroup>
  struct distributed_data_structure 
  {
    explicit distributed_data_structure(const ProcessGroup& pg)
      : process_group(pg, boost::parallel::attach_distributed_object())
    { }

  private:
    ProcessGroup process_group;
  };


Asynchronous receives
~~~~~~~~~~~~~~~~~~~~~

Distributed data structures in the Parallel BGL can "asynchronously"
receive and process messages before the end of a BSP
superstep. Messages can be received any time that a process is inside
the process group operations, and the scheduling of message receives
is entirely managed by the process group. 

Distributed data structures receive messages through
"triggers". Triggers are function objects responsible for processing a
received message. Each trigger is registered with the ``trigger``
method of the process group using a specific message
tag (an integer) and the type of data that is expected to be
contained within that message. Whenever a message with that tag
becomes available, the progress group will call the trigger with the
source of the message, the message tag, the data contained in the
message, and the "context" of the message.

The majority of triggers have no return value, although it is common
that the triggers send messages back to the source process. In certain
cases where the trigger's purpose is to immediately reply with a
value, the trigger should be registered with the
``trigger_with_reply`` method and should return the value that will be
sent back to the caller. The ``trigger_with_reply`` facility is only
useful in conjunction with out-of-band messaging, discussed next.

Out-of-band messaging
~~~~~~~~~~~~~~~~~~~~~

The majority of messages sent by the Parallel BGL are sent through the
normal send operations, to be received in the next superstep or, in
some cases, received "early" by a trigger. These messages are not
time-sensitive, so they will be delivered whenever the process group
processes them. 

Some messages, however, require immediate responses. For example, if a
process needs to determine the current value associated with a vertex
owned by another process, the first process must send a request to the
second process and block while waiting for a response. For such
messages, the Parallel BGL's process groups provide an out-of-band
messaging mechanism. Out-of-band messages are transmitted immediately,
with a much higher priority than other messages. The sending of
out-of-band messages can be coupled with a receive operation that
waits until the remote process has received the message and sent its
reply. For example, in the following code the process sends a message
containing the string ``name`` to process ``owner`` with tag
``msg_get_descriptor_by_name`` via an out-of-band message. The
receiver of that message will immediately deliver the message via a
trigger, that returns the resulting value--a
``vertex_descriptor``--that will be passed back to the process that
initiated the communication. The full communication happens
immediately, within the current superstep.

::
 
  std::string name;
  vertex_descriptor descriptor;
  send_oob_with_reply(process_group, owner, msg_get_descriptor_by_name,
                      name, descriptor);

Reference
---------

The Parallel BGL process groups specify an interface that can be
implemented by various communication subsystems. In this reference
section, we use the placeholder type ``ProcessGroup`` to stand in for
the various process group implementations that exist. There is only
one implementation of the process group interface at this time:

  - `MPI BSP process group`_

::

  enum trigger_receive_context {
    trc_none,
    trc_in_synchronization,
    trc_early_receive,
    trc_out_of_band
  };
  
  class ProcessGroup 
  {
    // Process group constructors
    ProcessGroup();
    ProcessGroup(const ProcessGroup&, boost::parallel::attach_distributed_object);

    // Triggers
    template<typename Type, typename Handler>
      void trigger(int tag, const Handler& handler);

    template<typename Type, typename Handler>
      void trigger_with_reply(int tag, const Handler& handler);

    trigger_receive_context trigger_context() const;

    // Helper operations
    void poll();
    ProcessGroup base() const;
  };

  // Process query
  int process_id(const ProcessGroup&);
  int num_processes(const ProcessGroup&);

  // Message transmission
  template<typename T>
    void send(const ProcessGroup& pg, int dest, int tag, const T& value);

  template<typename T>
    void receive(const ProcessGroup& pg, int source, int tag, T& value);

  optional<std::pair<int, int> > probe(const ProcessGroup& pg);

  // Synchronization
  void synchronize(const ProcessGroup& pg);

  // Out-of-band communication
  template<typename T>
    void send_oob(const ProcessGroup& pg, int dest, int tag, const T& value);

  template<typename T, typename U>
    void 
    send_oob_with_reply(const ProcessGroup& pg, int dest, int
                        tag, const T& send_value, U& receive_value);

  template<typename T>
    void receive_oob(const ProcessGroup& pg, int source, int tag, T& value);


Process group constructors
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    ProcessGroup();

Constructs a new process group with a different communication space
from any other process group.

-----------------------------------------------------------------------------

::

    ProcessGroup(const ProcessGroup& pg, boost::parallel::attach_distributed_object);

Attaches a new distributed data structure to the process group
``pg``. The resulting process group can be used for communication
within that new distributed data structure. When the newly-constructed
process group is eventually destroyed, the distributed data structure
is detached from the process group.

Triggers
~~~~~~~~

::

    template<typename Type, typename Handler>
      void trigger(int tag, const Handler& handler);

Registers a trigger with the given process group. The trigger will
watch for messages with the given ``tag``. When such a message is
available, it will be received into a value of type ``Type``, and the
function object ``handler`` will be invoked with four parameters: 

source
  The rank of the source process (an ``int``)

tag
  The tag used to send the message (also an ``int``)

data:
  The data transmitted with the message. The data will have the type
  specified when the trigger was registered.

context:
  The context in which the trigger is executed. This will be a value of
  type ``trigger_receive_context``, which stages whether the trigger
  is being executed during synchronization, asynchronously in response
  to an "early" receive (often to free up communication buffers), or
  in response to an "out-of-band" message.

Triggers can only be registered by process groups that result from
attaching a distributed data structure. A trigger can be invoked in
response to either a normal send operation or an out-of-band send
operation. There is also a `simple trigger interface`_ for defining
triggers in common cases.

-----------------------------------------------------------------------------

::

    template<typename Type, typename Handler>
      void trigger_with_reply(int tag, const Handler& handler);

Like the ``trigger`` method, registers a trigger with the given
process group. The trigger will watch for messages with the given
``tag``. When such a message is available, it will be received into a
value of type ``Type`` and ``handler`` will be invoked, just as with a
normal trigger. However, a trigger registered with
``trigger_with_reply`` must return a value, which will be immediately
sent back to the process that initiated the send resulting in this
trigger. Thus, ``trigger_with_reply`` should only be used for messages
that need immediate responses. These triggers can only be invoked via
the out-of-band sends that wait for the reply, via
``send_oob_with_reply``. There is also a `simple trigger interface`_
for defining triggers in common cases.

-----------------------------------------------------------------------------

::

    trigger_receive_context trigger_context() const;

Retrieves the current context of the process group with respect to the
invocation of triggers. When ``trc_none``, the process group is not
currently invoking any triggers. Otherwise, this value describes in
what context the currently executing trigger is being invoked.


Helper operations
~~~~~~~~~~~~~~~~~

::

  void poll();

Permits the process group to receive any incomining messages,
processing them via triggers. If you have a long-running computation
that does not invoke any of the process group's communication
routines, you should call ``poll`` occasionally to along incoming
messages to be processed. 

-----------------------------------------------------------------------------

::

    ProcessGroup base() const;

Retrieves the "base" process group for this process group, which is a
copy of the underlying process group that does not reference any
specific distributed data structure.

Process query
~~~~~~~~~~~~~

::

  int process_id(const ProcessGroup& pg);

Retrieves the ID (or "rank") of the calling process within the process
group. Process IDs are values in the range [0, ``num_processes(pg)``)
that uniquely identify the process. Process IDs can be used to
initiate communication with another process.

-----------------------------------------------------------------------------

::

  int num_processes(const ProcessGroup& pg);

Returns the number of processes within the process group.


Message transmission
~~~~~~~~~~~~~~~~~~~~

::

  template<typename T>
    void send(const ProcessGroup& pg, int dest, int tag, const T& value);

Sends a message with the given ``tag`` and carrying the given
``value`` to the process with ID ``dest`` in the given process
group. All message sends are non-blocking, meaning that this send
operation will not block while waiting for the communication to
complete. There is no guarantee when the message will be received,
except that it will become available to the destination process by the
end of the superstep, in the collective call to ``synchronize``.

Any type of value can be transmitted via ``send``, so long as it
provides the appropriate functionality to be serialized with the
Boost.Serialization library.

-----------------------------------------------------------------------------

::

  template<typename T>
    void receive(const ProcessGroup& pg, int source, int tag, T& value);

Receives a message with the given ``tag`` sent from the process
``source``, updating ``value`` with the payload of the message. This
receive operation can only receive messages sent within the previous
superstep via the ``send`` operation. If no such message is available
at the time ``receive`` is called, the program is ill-formed.

-----------------------------------------------------------------------------

::

  optional<std::pair<int, int> > probe(const ProcessGroup& pg);

Determines whether a message is available. The probe operation checks
for any messages that were sent in the previous superstep but have not
yet been received. If such a message exists, ``probe`` returns a
(source, tag) pair describing the message. Otherwise, ``probe`` will
return an empty ``boost::optional``. 

A typical use of ``probe`` is to continually probe for messages at the
beginning of the superstep, receiving and processing those messages
until no messages remain.


Synchronization
~~~~~~~~~~~~~~~

::

  void synchronize(const ProcessGroup& pg);

The ``synchronize`` function is a collective operation that must be
invoked by all of the processes within the process group. A call to
``synchronize`` marks the end of a superstep in the parallel
computation. All messages sent before the end of the superstep will be
received into message buffers, and can be processed by the program in
the next superstep. None of the processes will leave the
``synchronize`` function until all of the processes have entered the
function and exchanged messages, so that all processes are always on
the same superstep.

Out-of-band communication
~~~~~~~~~~~~~~~~~~~~~~~~~

::

  template<typename T>
    void send_oob(const ProcessGroup& pg, int dest, int tag, const T& value);

Sends and out-of-band message. This out-of-band send operation acts
like the normal ``send`` operation, except that out-of-band messages
are delivered immediately through a high-priority channel. 

-----------------------------------------------------------------------------

::

  template<typename T, typename U>
    void 
    send_oob_with_reply(const ProcessGroup& pg, int dest, int
                        tag, const T& send_value, U& receive_value);

Sends an out-of-band message and waits for a reply. The
``send_oob_with_reply`` function can only be invoked with message tags
that correspond to triggers registered with
``trigger_with_reply``. This operation will send the message
immediately (through the high-priority, out-of-band channel), then
wait until the remote process sends a reply. The data from the reply
is stored into ``receive_value``. 

-----------------------------------------------------------------------------

::

  template<typename T>
    void receive_oob(const ProcessGroup& pg, int source, int tag, T& value);

Receives an out-of-band message with the given ``source`` and
``tag``. As with the normal ``receive`` operation, it is an error to
call ``receive_oob`` if no message matching the source and tag is
available. This routine is used only rarely; for most circumstances,
use ``send_oob_with_reply`` to perform an immediate send with a
reply. 

-----------------------------------------------------------------------------

Copyright (C) 2007 Douglas Gregor

Copyright (C) 2007 Matthias Troyer

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _MPI BSP process group: mpi_bsp_process_group.html
.. _Simple trigger interface: simple_trigger.html

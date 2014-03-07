.. Copyright (C) 2004-2009 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

============================
|Logo| MPI BSP Process Group
============================

.. contents::

Introduction
------------

The MPI ``mpi_process_group`` is an implementation of the `process
group`_ interface using the Message Passing Interface (MPI). It is the
primary process group used in the Parallel BGL at this time.

Where Defined
-------------

Header ``<boost/graph/distributed/mpi_process_group.hpp>``

Reference
---------

::

  namespace boost { namespace graph { namespace distributed {

  class mpi_process_group 
  {
  public:
    typedef boost::mpi::communicator communicator_type;
    
    // Process group constructors
    mpi_process_group(communicator_type comm = communicator_type());
    mpi_process_group(std::size_t num_headers, std::size_t buffer_size, 
                      communicator_type comm = communicator_type());
    
    mpi_process_group();
    mpi_process_group(const mpi_process_group&, boost::parallel::attach_distributed_object);

    // Triggers
    template<typename Type, typename Handler>
      void trigger(int tag, const Handler& handler);

    template<typename Type, typename Handler>
      void trigger_with_reply(int tag, const Handler& handler);

    trigger_receive_context trigger_context() const;

    // Helper operations
    void poll();
    mpi_process_group base() const;
  };

  // Process query
  int process_id(const mpi_process_group&);
  int num_processes(const mpi_process_group&);

  // Message transmission
  template<typename T>
    void send(const mpi_process_group& pg, int dest, int tag, const T& value);

  template<typename T>
    void receive(const mpi_process_group& pg, int source, int tag, T& value);

  optional<std::pair<int, int> > probe(const mpi_process_group& pg);

  // Synchronization
  void synchronize(const mpi_process_group& pg);

  // Out-of-band communication
  template<typename T>
    void send_oob(const mpi_process_group& pg, int dest, int tag, const T& value);

  template<typename T, typename U>
    void 
    send_oob_with_reply(const mpi_process_group& pg, int dest, int
                        tag, const T& send_value, U& receive_value);

  template<typename T>
    void receive_oob(const mpi_process_group& pg, int source, int tag, T& value);

  } } }

Since the ``mpi_process_group`` is an implementation of the `process
group`_ interface, we omit the description of most of the functions in
the prototype. Two constructors need special mentioning:

::
  
      mpi_process_group(communicator_type comm = communicator_type());

The constructor can take an optional MPI communicator. As default a communicator
constructed from MPI_COMM_WORLD is used.

::
  
    mpi_process_group(std::size_t num_headers, std::size_t buffer_size, 
                      communicator_type comm = communicator_type());


For performance fine tuning the maximum number of headers in a message batch
(num_headers) and the maximum combined size of batched messages (buffer_size) 
can be specified. The maximum message size of a batch is 
16*num_headers+buffer_size. Sensible default values have been found by optimizing
a typical application on a cluster with Ethernet network, and are num_header=64k
and buffer_size=1MB, for a total maximum batches message size of 2MB.



-----------------------------------------------------------------------------

Copyright (C) 2007 Douglas Gregor

Copyright (C) 2007 Matthias Troyer

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _process group: process_group.html

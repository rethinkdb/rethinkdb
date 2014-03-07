.. Copyright (C) 2004-2008 The Trustees of Indiana University.
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
   http://www.boost.org/LICENSE_1_0.txt)

===============================
|Logo| Distributed Property Map
===============================

A distributed property map adaptor is a property map whose stored
values are distributed across multiple non-overlapping memory spaces
on different processes. Values local to the current process are stored
within a local property map and may be immediately accessed via
``get`` and ``put``. Values stored on remote processes may also be
accessed via ``get`` and ``put``, but the behavior differs slightly:

 - ``put`` operations update a local ghost cell and send a "put"
   message to the process that owns the value. The owner is free to
   update its own "official" value or may ignore the put request.

 - ``get`` operations returns the contents of the local ghost
   cell. If no ghost cell is available, one is created using a
   (customizable) default value. 

Using distributed property maps requires a bit more care than using
local, sequential property maps. While the syntax and semantics are
similar, distributed property maps may contain out-of-date
information that can only be guaranteed to be synchronized by
calling the ``synchronize`` function in all processes.

To address the issue of out-of-date values, distributed property
maps support multiple `consistency models`_ and may be supplied with a
`reduction operation`_.

Distributed property maps meet the requirements of the `Readable
Property Map`_ and, potentially, the `Writable Property Map`_ and
`Read/Write Property Map`_ concepts. Distributed property maps do
*not*, however, meet the requirements of the `Lvalue Property Map`_
concept, because elements residing in another process are not
directly addressible. There are several forms of distributed property
maps:

  - `Distributed property map adaptor`_
  - `Distributed iterator property map`_
  - `Distributed safe iterator property map`_
  - `Local property map`_

------------------
Consistency models
------------------

Distributed property maps offer many consistency models, which affect
how the values read from and written to remote keys relate to the
"official" value for that key stored in the owning process. The
consistency model of a distributed property map can be set with the
member function ``set_consistency_model`` to a bitwise-OR of the
flags in the ``boost::parallel::consistency_model`` enumeration. The
individual flags are:

  - ``cm_forward``: The default consistency model, which propagates
    values forward from ``put`` operations on remote processors to
    the owner of the value being changed. 
  
  - ``cm_backward``: After all values have been forwarded or flushed
    to the owning processes, each process receives updates values for
    each of its ghost cells. After synchronization, the values in
    ghost cells are guaranteed to match the values stored on the
    owning processor.

  - ``cm_bidirectional``: A combination of both ``cm_forward`` and
    ``cm_backward``. 

  - ``cm_flush``: At the beginning of synchronization, all of the
    values stored locally in ghost cells are sent to their owning
    processors. 

  - ``cm_reset``: Executes a ``reset()`` operation after
    synchronization, setting the values in each ghost cell to their
    default value.

  - ``cm_clear``: Executes a ``clear()`` operation after
    synchronizing, eliminating all ghost cells.


There are several common combinations of flags that result in
interesting consistency models. Some of these combinations are:

  - ``cm_forward``: By itself, the forward consistency model enables
    algorithms such as `Dijkstra's shortest paths`_ and
    `Breadth-First Search`_ to operate correctly.

  - ``cm_flush & cm_reset``: All updates values are queued locally,
    then flushed during the synchronization step. Once the flush has
    occurred, the ghost cells are restored to their default
    values. This consistency model is used by the PageRank_
    implementation to locally accumulate rank for each node.


-------------------
Reduction operation
-------------------

The reduction operation maintains consistency by determining how
multiple writes to a property map are resolved and what the property
map should do if unknown values are requested. More specifically, a
reduction operation is used in two cases:

  1. When a value is needed for a remote key but no value is
     immediately available, the reduction operation provides a
     suitable default. For instance, a distributed property map
     storing distances may have a reduction operation that returns
     an infinite value as the default, whereas a distributed
     property map for vertex colors may return white as the
     default.

  2. When a value is received from a remote process, the process
     owning the key associated with that value must determine which
     value---the locally stored value, the value received from a
     remote process, or some combination of the two---will be
     stored as the "official" value in the property map. The
     reduction operation transforms the local and remote values
     into the "official" value to be stored.

The reduction operation of a distributed property map can be set with
the ``set_reduce`` method of ``distributed_property_map``. The reduce
operation is a function object with two signatures. The first
signature takes a (remote) key and returns a default value for it,
whereas the second signatures takes a key and two values (local first,
then remote) and will return the combined value that will be stored in
the local property map.  Reduction operations must also contain a
static constant ``non_default_resolver", which states whether the
reduction operation's default value actually acts like a default
value. It should be ``true`` when the default is meaningful (e.g.,
infinity for a distance) and ``false`` when the default should not be
used. 

The following reduction operation is used by the distributed PageRank
algorithm. The default rank for a remote node is 0. Rank is
accumulated locally, and then the reduction operation combines local
and remote values by adding them. Combined with a consistency model
that flushes all values to the owner and then resets the values
locally in each step, the resulting property map will compute partial 
sums on each processor and then accumulate the results on the owning
processor. The PageRank reduction operation is defined as follows.
 
::

  template<typename T>
  struct rank_accumulate_reducer {
    static const bool non_default_resolver = true;

    // The default rank of an unknown node 
    template<typename K>
    T operator()(const K&) const { return T(0); }

    template<typename K>
    T operator()(const K&, const T& x, const T& y) const { return x + y; }
  };


--------------------------------
Distributed property map adaptor
--------------------------------

The distributed property map adaptor creates a distributed property
map from a local property map, a `process group`_ over which
distribution should occur, and a `global descriptor`_ type that
indexes the distributed property map. 
  

Synopsis
~~~~~~~~

::

  template<typename ProcessGroup, typename LocalPropertyMap, typename Key,
           typename GhostCellS = gc_mapS>
  class distributed_property_map
  {
  public:
    typedef ... ghost_regions_type;
 
    distributed_property_map();

    distributed_property_map(const ProcessGroup& pg, 
                             const LocalPropertyMap& pm);

    template<typename Reduce>
    distributed_property_map(const ProcessGroup& pg, 
                             const LocalPropertyMap& pm,
                             const Reduce& reduce);

    template<typename Reduce> void set_reduce(const Reduce& reduce);
    void set_consistency_model(int model);

    void flush();
    void reset();
    void clear();
  };

  reference get(distributed_property_map pm, const key_type& key);

  void
  put(distributed_property_map pm, const key_type& key, const value_type& value);
  local_put(distributed_property_map pm, const key_type& key, const value_type& value);

  void request(distributed_property_map pm, const key_type& key);

  void synchronize(distributed_property_map& pm);

  template<typename Key, typename ProcessGroup, typename LocalPropertyMap>
  distributed_property_map<ProcessGroup, LocalPropertyMap, Key>
  make_distributed_property_map(const ProcessGroup& pg, LocalPropertyMap pmap);

  template<typename Key, typename ProcessGroup, typename LocalPropertyMap,
           typename Reduce>
  distributed_property_map<ProcessGroup, LocalPropertyMap, Key>
  make_distributed_property_map(const ProcessGroup& pg, LocalPropertyMap pmap,
                                Reduce reduce);

Template parameters
~~~~~~~~~~~~~~~~~~~

**ProcessGroup**:
  The type of the process group over which the
  property map is distributed and is also the medium for
  communication.


**LocalPropertyMap**:
  The type of the property map that will store values
  for keys local to this processor. The ``value_type`` of this
  property map will become the ``value_type`` of the distributed
  property map. The distributed property map models the same property
  map concepts as the ``LocalPropertyMap``, with one exception: a
  distributed property map cannot be an `Lvalue Property Map`_
  (because remote values are not addressable), and is therefore
  limited to `Read/Write Property Map`_.


**Key**:
  The ``key_type`` of the distributed property map, which
  must model the `Global Descriptor`_ concept. The process ID type of
  the ``Key`` parameter must match the process ID type of the
  ``ProcessGroup``, and the local descriptor type of the ``Key`` must
  be convertible to the ``key_type`` of the ``LocalPropertyMap``.


**GhostCellS**:
  A selector type that indicates how ghost cells should be stored in
  the distributed property map. There are either two or three
  options, depending on your compiler:

    - ``boost::parallel::gc_mapS`` (default): Uses an STL ``map`` to
      store the ghost cells for each process. 
    
    - ``boost::parallel::gc_vector_mapS``: Uses a sorted STL
      ``vector`` to store the ghost cells for each process. This
      option works well when there are likely to be few insertions
      into the ghost cells; for instance, if the only ghost cells used
      are for neighboring vertices, the property map can be
      initialized with cells for each neighboring vertex, providing
      faster lookups than a ``map`` and using less space.

    - ``boost::parallel::gc_hash_mapS``: Uses the GCC ``hash_map`` to
      store ghost cells. This option may improve performance over
      ``map`` for large problems sizes, where the set of ghost cells
      cannot be predetermined.


Member functions
~~~~~~~~~~~~~~~~

::

  distributed_property_map();

Default-construct a distributed property map. The property map is in
an invalid state, and may only be used if it is reassigned to a valid
property map. 

------------------------------------------------------------------------------

::

    distributed_property_map(const ProcessGroup& pg, 
                             const LocalPropertyMap& pm);

    template<typename Reduce>
    distributed_property_map(const ProcessGroup& pg, 
                             const LocalPropertyMap& pm,
                             const Reduce& reduce);

Construct a property map from a process group and a local property
map. If a ``reduce`` operation is not supplied, a default of
``basic_reduce<value_type>`` will be used. 

------------------------------------------------------------------------------

::

  template<typename Reduce> void set_reduce(const Reduce& reduce);

Replace the current reduction operation with the new operation
``reduce``.

------------------------------------------------------------------------------

::

  void set_consistency_model(int model);

Sets the consistency model of the distributed property map, which will
take effect on the next synchronization step. See the section
`Consistency models`_ for a description of the effect of various
consistency model flags.

------------------------------------------------------------------------------

::
  
  void flush();

Emits a message sending the contents of all local ghost cells to the
owners of those cells. 

------------------------------------------------------------------------------

::
  
  void reset();

Replaces the values stored in each of the ghost cells with the default
value generated by the reduction operation. 

------------------------------------------------------------------------------

::
  
  void clear();

Removes all ghost cells from the property map.


Free functions
~~~~~~~~~~~~~~

::

  reference get(distributed_property_map pm, const key_type& key);

Retrieves the element in ``pm`` associated with the given ``key``. If
the key refers to data stored locally, returns the actual value
associated with the key. If the key refers to nonlocal data, returns
the value of the ghost cell. If no ghost cell exists, the behavior
depends on the current reduction operation: if a reduction operation
has been set and has ``non_default_resolver`` set ``true``, then a
ghost cell will be created according to the default value provided by
the reduction operation. Otherwise, the call to ``get`` will abort
because no value exists for this remote cell. To avoid this problem,
either set a reduction operation that generates default values,
``request()`` the value and then perform a synchronization step, or
``put`` a value into the cell before reading it.

------------------------------------------------------------------------------

::

  void
  put(distributed_property_map pm, const key_type& key, const value_type& value);

Places the given ``value`` associated with ``key`` into property map
``pm``. If the key refers to data stored locally, the value is
immediately updates. If the key refers to data stored in a remote
process, updates (or creates) a local ghost cell containing this
value for the key and sends the new value to the owning process. Note
that the owning process may reject this value based on the reduction
operation, but this will not be detected until the next
synchronization step.

------------------------------------------------------------------------------

::

  void
  local_put(distributed_property_map pm, const key_type& key, const value_type& value);

Equivalent to ``put(pm, key, value)``, except that no message is sent
to the owning process when the value is changed for a nonlocal key.

------------------------------------------------------------------------------

::

  void synchronize(distributed_property_map& pm);

Synchronize the values stored in the distributed property maps. Each
process much execute ``synchronize`` at the same time, after which
the ghost cells in every process will reflect the actual value stored
in the owning process.

------------------------------------------------------------------------------

::

 void request(distributed_property_map pm, const key_type& key);

Request that the element "key" be available after the next
synchronization step. For a non-local key, this means establishing a
ghost cell and requesting.

------------------------------------------------------------------------------

::

  template<typename Key, typename ProcessGroup, typename LocalPropertyMap>
  distributed_property_map<ProcessGroup, LocalPropertyMap, Key>
  make_distributed_property_map(const ProcessGroup& pg, LocalPropertyMap pmap);

  template<typename Key, typename ProcessGroup, typename LocalPropertyMap,
           typename Reduce>
  distributed_property_map<ProcessGroup, LocalPropertyMap, Key>
  make_distributed_property_map(const ProcessGroup& pg, LocalPropertyMap pmap,
                                Reduce reduce);

Create a distributed property map over process group ``pg`` and local
property map ``pmap``. A default reduction operation will be generated
if it is not provided.

---------------------------------
Distributed iterator property map
---------------------------------

The distributed iterator property map adaptor permits the creation of
distributed property maps from random access iterators using the same
syntax as non-distributed iterator property maps. The specialization
is based on a `local property map`_, which contains the
indices for local descriptors and is typically returned to describe
the vertex indices of a distributed graph.

Synopsis
~~~~~~~~

::

  template<typename RandomAccessIterator, typename ProcessGroup,
           typename GlobalKey, typename LocalMap, typename ValueType,
           typename Reference>
  class iterator_property_map<RandomAccessIterator, 
                              local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                              ValueType, Reference>
  {
  public:
    typedef local_property_map<ProcessGroup, GlobalKey, LocalMap> index_map_type;

    iterator_property_map();
    iterator_property_map(RandomAccessIterator iter, const index_map_type& id);
  };

  reference get(iterator_property_map pm, const key_type& key);
  void put(iterator_property_map pm, const key_type& key, const value_type& value);

  template<typename RandomAccessIterator, typename ProcessGroup,
           typename GlobalKey, typename LocalMap>
  iterator_property_map<RandomAccessIterator, 
                        local_property_map<ProcessGroup, GlobalKey, LocalMap> >
  make_iterator_property_map(RandomAccessIterator iter, 
                             local_property_map<ProcessGroup, GlobalKey, LocalMap> id);


Member functions
~~~~~~~~~~~~~~~~

::

    iterator_property_map();

Default-constructs a distributed iterator property map. The property
map is in an invalid state, and must be reassigned before it may be
used. 

------------------------------------------------------------------------------

::

    iterator_property_map(RandomAccessIterator iter, const index_map_type& id);

Constructs a distributed iterator property map using the property map
``id`` to map global descriptors to local indices. The random access
iterator sequence ``[iter, iter + n)`` must be a valid range, where
``[0, n)`` is the range of local indices. 

Free functions
~~~~~~~~~~~~~~

::

  reference get(iterator_property_map pm, const key_type& key);

Returns the value associated with the given ``key`` from the
distributed property map.

------------------------------------------------------------------------------

::
 
  void put(iterator_property_map pm, const key_type& key, const value_type& value);

Associates the value with the given key in the distributed property map.

------------------------------------------------------------------------------

::

  template<typename RandomAccessIterator, typename ProcessGroup,
           typename GlobalKey, typename LocalMap, typename ValueType,
           typename Reference>
  iterator_property_map<RandomAccessIterator, 
                        local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                           ValueType, Reference>
  make_iterator_property_map(RandomAccessIterator iter, 
                             local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                                ValueType, Reference> id);

Creates a distributed iterator property map using the given iterator
``iter`` and local index property map ``id``.

--------------------------------------
Distributed safe iterator property map
--------------------------------------

The distributed safe iterator property map adaptor permits the
creation of distributed property maps from random access iterators
using the same syntax as non-distributed safe iterator property
maps. The specialization is based on a `local property map`_, which
contains the indices for local descriptors and is typically returned
to describe the vertex indices of a distributed graph. Safe iterator
property maps check the indices of accesses to ensure that they are
not out-of-bounds before attempting to access an value.

Synopsis
~~~~~~~~

::

  template<typename RandomAccessIterator, typename ProcessGroup,
           typename GlobalKey, typename LocalMap, typename ValueType,
           typename Reference>
  class safe_iterator_property_map<RandomAccessIterator, 
                                   local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                   ValueType, Reference>
  {
  public:
    typedef local_property_map<ProcessGroup, GlobalKey, LocalMap> index_map_type;

    safe_iterator_property_map();
    safe_iterator_property_map(RandomAccessIterator iter, std::size_t n, 
                               const index_map_type& id);
  };

  reference get(safe_iterator_property_map pm, const key_type& key);
  void put(safe_iterator_property_map pm, const key_type& key, const value_type& value);

  template<typename RandomAccessIterator, typename ProcessGroup,
           typename GlobalKey, typename LocalMap, typename ValueType,
           typename Reference>
  safe_iterator_property_map<RandomAccessIterator, 
                             local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                                ValueType, Reference>
  make_safe_iterator_property_map(RandomAccessIterator iter, 
                                  std::size_t n,
                                  local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                                     ValueType, Reference> id);

Member functions
~~~~~~~~~~~~~~~~

::

    safe_iterator_property_map();

Default-constructs a distributed safe iterator property map. The property
map is in an invalid state, and must be reassigned before it may be
used. 

------------------------------------------------------------------------------

::

    safe_iterator_property_map(RandomAccessIterator iter, std::size_t n,
                               const index_map_type& id);

Constructs a distributed safe iterator property map using the property map
``id`` to map global descriptors to local indices. The random access
iterator sequence ``[iter, iter + n)``. 

Free functions
~~~~~~~~~~~~~~

::

  reference get(safe_iterator_property_map pm, const key_type& key);

Returns the value associated with the given ``key`` from the
distributed property map.

------------------------------------------------------------------------------

::
 
  void put(safe_iterator_property_map pm, const key_type& key, const value_type& value);

Associates the value with the given key in the distributed property map.

------------------------------------------------------------------------------

::

  template<typename RandomAccessIterator, typename ProcessGroup,
           typename GlobalKey, typename LocalMap, typename ValueType,
           typename Reference>
  safe_iterator_property_map<RandomAccessIterator, 
                             local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                                ValueType, Reference>
  make_safe_iterator_property_map(RandomAccessIterator iter, 
                                  std::size_t n,
                                  local_property_map<ProcessGroup, GlobalKey, LocalMap>,
                                                     ValueType, Reference> id);

Creates a distributed safe iterator property map using the given iterator
``iter`` and local index property map ``id``. The indices in ``id`` must

------------------
Local property map
------------------

A property map adaptor that accesses an underlying property map whose
key type is the local part of the ``Key`` type for the local subset
of keys. Local property maps are typically used by distributed graph
types for vertex index properties.

Synopsis
~~~~~~~~

::

  template<typename ProcessGroup, typename GlobalKey, typename LocalMap>
    class local_property_map
    {
    public:
    typedef typename property_traits<LocalMap>::value_type value_type;
    typedef GlobalKey                                      key_type;
    typedef typename property_traits<LocalMap>::reference  reference;
    typedef typename property_traits<LocalMap>::category   category;

    explicit 
    local_property_map(const ProcessGroup& process_group = ProcessGroup(),
                       const LocalMap& local_map = LocalMap());

    reference operator[](const key_type& key);
  };

  reference get(const local_property_map& pm, key_type key);
  void put(local_property_map pm, const key_type& key, const value_type& value);

Template parameters
~~~~~~~~~~~~~~~~~~~

:ProcessGroup: the type of the process group over which the global
  keys are distributed.

:GlobalKey: The ``key_type`` of the local property map, which
  must model the `Global Descriptor`_ concept. The process ID type of
  the ``GlobalKey`` parameter must match the process ID type of the
  ``ProcessGroup``, and the local descriptor type of the ``GlobalKey``
  must be convertible to the ``key_type`` of the ``LocalMap``.

:LocalMap: the type of the property map that will store values
  for keys local to this processor. The ``value_type`` of this
  property map will become the ``value_type`` of the local
  property map. The local property map models the same property
  map concepts as the ``LocalMap``.

Member functions
~~~~~~~~~~~~~~~~

::

  explicit 
  local_property_map(const ProcessGroup& process_group = ProcessGroup(),
                     const LocalMap& local_map = LocalMap());

Constructs a local property map whose keys are distributed across the
given process group and which accesses the given local map.

------------------------------------------------------------------------------

::

  reference operator[](const key_type& key);

Access the value associated with the given key, which must be local
to this process.

Free functions
~~~~~~~~~~~~~~

::

  reference get(const local_property_map& pm, key_type key);

Return the value associated with the given key, which must be local
to this process.

------------------------------------------------------------------------------

::

  void put(local_property_map pm, const key_type& key, const value_type& value);

Set the value associated with the given key, which must be local to
this process.

-----------------------------------------------------------------------------

Copyright (C) 2004, 2005 The Trustees of Indiana University.

Authors: Douglas Gregor and Andrew Lumsdaine

.. |Logo| image:: pbgl-logo.png
            :align: middle
            :alt: Parallel BGL
            :target: http://www.osl.iu.edu/research/pbgl

.. _Readable Property Map: http://www.boost.org/libs/property_map/ReadablePropertyMap.html
.. _Writable Property Map: http://www.boost.org/libs/property_map/WritablePropertyMap.html
.. _Read/Write Property Map: http://www.boost.org/libs/property_map/ReadWritePropertyMap.html
.. _Lvalue Property Map: http://www.boost.org/libs/property_map/LvaluePropertyMap.html
.. _Process Group: process_group.html
.. _Global Descriptor: GlobalDescriptor.html
.. _Dijkstra's shortest paths: dijkstra_shortest_paths.html
.. _Breadth-First Search: breadth_first_search.html
.. _PageRank: page_rank.html


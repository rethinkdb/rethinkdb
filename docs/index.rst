.. RethinkDB documentation master file, created by
.. sphinx-quickstart on Thu Jan 13 01:07:31 2011.
.. You can adapt this file completely to your liking, but it should at least
.. contain the root `toctree` directive.
.. If you find yourself editing this and would like to preview it use
.. http://www.tele3.cz/jbar/rest/rest.html

============
Introduction
============

RethinkDB is an industrial strength, high performance, durable data
store. It supports a wide variety of operations on a set of key/value
pairs, and is exposed to the network via a Memcached protocol. The
engine is designed to be robust in a wide range of scenarios and
operates efficiently on various hardware configurations, workloads,
and dataset sizes. It is capable of operating in highly concurrent,
high throughput, low latency environments, and in many cases can
perform more than a million operations per second at sub-millisecond
latency on commodity hardware.

The following is a sample list of scenarios in which RethinkDB can
maintain efficient operation:

- Various dataset sizes, including datasets that fit into main memory,
  large datasets with *active* datasets that fit into main memory, and
  datasets that require disk access for every operation.
- Workloads with very large, very small, and mixed read/write
  ratios.
- A wide range of key and value sizes.
- Thousands of concurrent network connections.
- Pipelined as well as blocking operation requests.

In addition to supporting legacy hardware, RethinkDB takes full
advantage of modern architectures, including multicore servers,
solid-state drives, NUMA architectures, and fast Ethernet. It ships
with tools that help easily perform data migration and recovery, run
data consistency checks, do performance tuning, and more.

This manual describes the features exposed by RethinkDB in significant
detail. For additional questions, please contact RethinkDB support_.

.. contents::

=================
Quick start guide
=================

Install RethinkDB from http://rethinkdb.com/zWef3q/download/.

Start it [#trial-binary]_::

  $ rethinkdb

Connect to it via telnet and start typing memcached commands::

  $ telnet localhost 11211
  get foo
  END

===============
Getting started
===============

-------------
Prerequisites
-------------

RethinkDB works on modern 64-bit distributions of Linux. We support the following distributions:

- Ubuntu 10.04.1 x86_64
- Ubuntu 10.10 x86_64
- Red Hat Enterprise Linux 5 x86_64
- CentOS 5 x86_64

------------
Installation
------------

``````
Ubuntu
``````

Download the latest package of RethinkDB for Ubuntu from http://rethinkdb.com/zWef3q/download/.

Navigate to the directory the package was downloaded to and install RethinkDB and its dependencies::

  # Get dependencies for RethinkDB
  sudo apt-get install libaio1

  # Install RethinkDB
  dpkg -i rethinkdb_0.1.2-1_amd64.deb 

`````````````````
RHEL 5 / CentOS 5
`````````````````

Download the latest package of RethinkDB for RHEL 5 / CentOS 5 from http://rethinkdb.com/zWef3q/download/.

Navigate to the directory the package was downloaded to and install RethinkDB and its dependencies::

  # Install RethinkDB
  rpm -i rethinkdb-0.1.2-1.x86_64.rpm

~~~~~~~~~~~
Limitations
~~~~~~~~~~~

RHEL 5 and CentOS 5 kernels are missing certain system calls; this may affect performance in highly concurrent environments. 

In these environments, server-side software will not scale to a large number of concurrent connections. In database environments this normally does not affect real-world performance, but may affect the results of some artificial benchmarks.

------------------
Running the server
------------------

Once RethinkDB is installed, start the server::

  $ rethinkdb

This is equivalent to running RethinkDB with the ``serve`` command::

  $ rethinkdb serve

This command will look for a database file named ``rethinkdb_data`` in
the current directory, create it if it's missing, and start the server
on port ``11211``. Alternatively, specify the database file and
the port explicitly::

  $ rethinkdb -f mydb.file -p 8080

To test that the server is operating correctly, we can ``telnet`` into the
appropriate port and type Memcached commands directly. In the
following telnet session we set a value for a key, get it back, and
quit the connection::

  $ telnet localhost 11211
  Trying ::1...
  Trying 127.0.0.1...
  Connected to localhost.
  Escape character is '^]'.
  set foo 0 0 3
  bar
  STORED
  get foo
  VALUE foo 0 3
  bar
  END
  quit
  Connection closed by foreign host.

To stop the server, type ``CTRL + C``.

-----------------
Language bindings
-----------------

RethinkDB is binary compatible with the Memcached protocol, and can be
used as a drop in replacement for an existing solution without any
changes to the application. Client libraries that support the
Memcached protocol will also work with RethinkDB. The following page
contains a list of client libraries for various languages:
http://code.google.com/p/memcached/wiki/Clients.

Note that many existing clients have not implemented full support for the Memcached
protocol. You may encounter subtle issues with clients that aren't
in mainstream use.
  
For example, if you're using Python with the `pylibmc` library, you can set and get keys in the following way::

  >>> import pylibmc
  >>> conn = pylibmc.Client(["localhost:11211"])
  >>> conn.set("some_key", "some_value")
  True
  >>> conn.get("some_key")
  'some_value'

---------------
Additional help
---------------

To get additional help on specific usage of RethinkDB, use the built-in
``help`` command. For example, to learn more about the ``serve`` command::

  $ rethinkdb help serve

To get a full list of commands available within RethinkDB::

  $ rethinkdb help

Alternatively, you can get help from the RethinkDB man page that comes
with the installation::

  $ man rethinkdb

If you have additional questions, please contact RethinkDB support_.

========  
Features
========  

------------------
Memcached protocol
------------------

RethinkDB implements the Memcached protocol as described on the following
page:
http://code.sixapart.com/svn/memcached/trunk/server/doc/protocol.txt. All
specified commands should work as expected, and clients that work with
Memcached implementations should continue working with RethinkDB without modification. The
following is a list of known discrepancies with the Memcached
protocol:

- Currently, only the text protocol is supported.
- `Range queries`_  are supported, with `undefined boundary extension`_.
- Connections over UDP are not supported.
- Delete queues are not supported.
- The ``flush_all`` command is not supported.
- The ``stat`` command returns different statistics than specified in
  the protocol. Some of the statistics that do not make sense in the
  context of a persistent engine are removed, and new statistics are
  added.
- Value size limit is increased to 10MB from 1MB specified by Memcached.

-----------
Performance
-----------

RethinkDB has a number of features intended to increase
performance. Common performance problems encountered with database systems involve disk I/O bottlenecks
(number of possible operations per second, throughput, latency, etc.),
CPU lock contention, and network bottlenecks. The following features
are designed to mitigate performance problems associated with hardware
bottlenecks.

````
Disk
````

~~~~~~~~
Striping
~~~~~~~~

Modern RAID controllers implement efficient striping across disks by
synchronizing rotational disk spindles. Unfortunately, in the case of
solid-state drives, no synchronization is possible. Because these
drives often have varying latency, the entire array is limited to the speed of the slowest-operating drive at any given time.
This significantly increases latency on write operations. RethinkDB implements disk striping that
gets around this problem by writing to each disk independently. In
order to take advantage of this feature you can partition a RethinkDB
database across multiple files (located on one or many disks), and
RethinkDB will take care of striping and latency issues
automatically::

  $ rethinkdb -f file1.db -f file2.db

If the files ``file1.db`` and ``file2.db`` are located on different
disks, the I/O performance will double without needing to use a RAID
controller and without sacrificing latency.

Note that this feature does not implement mirroring and parity guarantees
implemented by advanced RAID controllers. The intention is not to
entirely replace RAID, but to support an alternative partitioning
method which can be very useful in certain situations.

~~~~~~~~~~~~~~
Active extents
~~~~~~~~~~~~~~

Rotational disks are fundamentally sequential machines—they have a
single head that can read from, and write to a single location at a
time. Many solid-state storage devices are fundamentally parallel—they
have multiple flash memory chips and improve in performance if software distributes writes to multiple disk locations concurrently.

RethinkDB divides disk space into blocks of space called *extents*.
Specify the number of concurrent extents by starting the server with the following flag::

  $ rethinkdb --active-data-extents 4

For storage systems based on rotational drives,
the value of ``active-data-extents`` should be set to ``1``. On
write-heavy workloads, many solid-state drives will perform more
efficiently if this value is between ``2`` and ``16``.

`````````
Multicore
`````````

RethinkDB has full support for machines with multiple CPUs and for
CPUs with multiple cores. By default, the server takes advantage of
all available cores on a machine. The number of cores the server
should use can be specified explicitly::

  $ rethinkdb --cores 8

This will limit the server to using eight cores. It is OK to
over-provision cores (passing a larger number than the machine has),
which may or may not affect performance in a real-world scenario.

``````
Memory
``````

The amount of available main memory can drastically affect performance
of a database system because main memory is used to cache data and delays the need to go to disk, which is orders of magnitude slower.
By default, RethinkDB will use as much memory as necessary (and as the
system has available) to operate efficiently. However, this number can
be specified explicitly::

  $ rethinkdb --max-cache-size 8192

The cache size is specified in megabytes—the above command limits
the cache size to 8GB.

----------
Durability
----------

``````````````
Flush interval
``````````````

For increased performance, RethinkDB delays flushing data to disk in
order to batch updates and write them to disk more efficiently. The
amount of time between flushes can be controlled explicitly (in milliseconds)::

  $ rethinkdb --flush-timer 1000

This tells the server to flush data to disk every second. A longer
flush timer allows the server to batch writes more effectively and
increase performance. A shorter flush timer flushes the data more
often, but ensures that less data can be lost in the event of a power
failure.

``````````````````
Unsaved data limit
``````````````````

In environments that operate under extremely high load, the network
component is often significantly faster than the disk, which means
commands arrive at a faster rate than the storage system can
satisfy. In these situations RethinkDB implements throughput
throttling—if the disk gets saturated, RethinkDB slows down its
responses to commands to give the disk time to catch up.

To maintain high performance, RethinkDB often allows the commands to
proceed despite the fact that the disk cannot catch up. This allows
the changes to batch in memory and get flushed to disk later. In
cases of power failure, this means large amounts of data can be
lost. RethinkDB allows controlling precisely how much data is allowed to be
cached in RAM without flushing to disk (in megabytes)::

  $ rethinkdb --unsaved-data-limit 1024

This allows RethinkDB to cache up to one gigabyte of unsaved data in RAM. In
the event of a power failure, no more than one gigabyte of data will be
lost. Adjust this limit to set the durability and performance trade-off to an acceptable level.

`````````````
Response mode
`````````````

By default, RethinkDB responds to write commands before they get
committed to disk. This significantly decreases the latency and allows
for increased throughput, but leaves the  possibility of data loss in the
event of power failure. It is possible to ensure no data loss in the
event of a power failure by telling the server not to acknowledge
writes until they are safely committed to disk::

  $ rethinkdb --wait-for-flush y

Note that to minimize latency, if ``wait-for-flush`` is turned on, the
`flush interval`_ should be set to a low value (or zero) to ensure
low latency.

---------------------------
Data migration and recovery
---------------------------

RethinkDB provides tools for migrating into different solutions by
exporting its data to the open Memcached format. The following command extracts the
contents of a RethinkDB database::

  $ rethinkdb extract -f file.db -o memcached.out

This command extracts the data from the database file ``file.db`` into
a file named ``memcached.out``. The contents of ``memcached.out`` will
be standard Memcached insertion commands which can be piped into a
different server that supports the Memcached protocol, or
programmatically converted to other formats. For example, if we have a
different server that supports a Memcached interface (including
RethinkDB) running on a port ``8080`` we can fill it with the contents
of the exported file with the following Unix command::

  $ cat memcached.out | nc localhost 8080 -q 0

The ``extract`` command works even in cases when the data has been
corrupted and  server cannot open the database file. In this
case, ``extract`` will try to recover as much data as possible and
ignore the corrupted parts of the database file.

========================
Replication and failover
========================

RethinkDB version 2.0 supports replication between two servers: a
"master" and a "slave".

-----------
Basic setup
-----------

RethinkDB replication can be set up as follows:

1. Install RethinkDB version 2.x on two machines. Choose one machine to
act as the "master" and one machine to act as the "slave".

2. If you intend to turn an existing non-replicated RethinkDB 2.x
database into a replicated database, the database files must be present
on the master machine. If you intend to turn an existing RethinkDB 1.x
database into a replicated database, you must first turn it into a
RethinkDB 2.x database, which is described elsewhere. If you want to
start a fresh database, use ``rethinkdb create`` to create a new empty
database on the master machine.

3. Create a new empty database on the slave machine using ``rethinkdb
create``. The database creation parameters (number of slices, block
size, etc.) can be different on the master and the slave.

4. On the master machine, run ``rethinkdb serve --master <port>``, using
the ``-f`` flag to specify the database files you prepared in step 2.
You should see a message like of ``Waiting for initial slave to connect
on port <port>...`` in the master's log.

5. On the slave machine, run ``rethinkdb serve --slave-of
<master>:<port>``. You should see a message indicating successful
connection to the master in the slave's log. You should see a message
indicating that the slave has connected in the master's log.

6. At this point, you can perform reads and writes on the master using
the normal memcached-compatible interface. If the master's database
contained any data before the master was started up, the slave will copy
that data; after it finishes copying that data, the slave will also
allow you to perform reads, but not writes. Any writes that you perform
on the master will be replicated to the slave.

In general, the master and the slave will always report the same value
for each key, unless a change has recently been made on one of them and
has not yet been transferred to the other. There are some exceptions to
this rule; the main exception is that no guarantees are made about keys
with expiration times.

--------------
Crash recovery
--------------

If the slave crashes, restart it using the same parameters as before. It
will automatically reconnect to the master and catch up with any changes
that occurred while the slave was down.

If the master crashes, the slave will detect that the master is no
longer active and will allow you to perform writes. Restart the master
using the same parameters as before; the slave will automatically
reconnect to the master and the master will catch up with any changes
that were made on the slave while the master was down. Once the master
has caught up, the slave will stop accepting writes and the master will
start accepting reads and writes.

-----------------
Disaster recovery
-----------------

If your slave-machine is struck by lightning, destroyed in an
explosion, or has a hard-drive crash: Buy a new server to act as the new
slave machine. Create a new fresh database on the slave machine. Run
``rethinkdb serve --slave-of <master>:<port>`` on that machine. It will
automatically re-copy the data from the master.

If your master-machine is destroyed, shut down the slave (using SIGINT
or by sending ``rethinkdb shutdown`` over telnet) and run ``rethinkdb
serve --master <port>`` on the slave machine using the same set of files
that you ran ``rethinkdb serve --slave-of ...`` with. Now the slave
machine will act as a master, and you can start up a new slave using the
procedure described above. Note that once you run ``rethinkdb serve
--master`` on the slave's data files, they will be irreversibly
converted into master-files, and you will have to perform the same
reversal again if you want that particular machine to be the
slave-machine.

----------------------
Database restructuring
----------------------

You can always convert a nonreplicated database into a replication
master or vice versa; just start it with or without the ``--master``
flag, and it will behave correctly. You can use the same technique to
convert a slave database into a nonreplicated database or a replication
master, but you won't be able to change it back into a slave again if
you do that.

-----------
CAS queries
-----------

``gets`` queries count as writes for the purposes of replication.

-------------------------------
The dangers of expiration times
-------------------------------

Please don't mix expiration times with replication. If you insert keys
with expiration times into a replicated database, the behavior is
undefined; the keys may have different values on the master and the
slave.

--------------
Divergent data
--------------

Sometimes the data on the master and the slave can diverge. This can
happen if the master crashes, and some writes to the master are recorded
to disk without being sent to the slave. It can also happen if the slave
and master lose contact with each other but clients stay in touch with
both of them. (Divergence isn't the same as when the master or the slave
goes down; when the master or the slave goes down, then it will
automatically catch back up with the other one.)

When the master and slave get back in contact after having diverged, the
following procedure is used to merge the data:

* If a key was changed on neither the master nor the slave since they
  diverged, then it keeps that value.

* If a key was changed on the slave since the slave and master diverged,
  then it takes the value it was given on the slave.

* If a key was changed on the master but not on the slave, then it may
  have either the value it was assigned on the master or the value that
  it had before the divergence.

-----------------------------
Elastic Load Balancer support
-----------------------------

You may want to use Amazon's Elastic Load Balancer (ELB) in conjunction
with RethinkDB, to automatically direct queries to whichever of the
master and the slave is able to accept writes. If you add
``--run-behind-elb <port>`` to the slave's command line, then it will
accept connections on the given ``<port>`` if and only if it is willing
to accept writes. It will drop the connection as soon as it accepts it.
The only purpose of this is to signal to the ELB that it is willing to
accept writes.

----------------
Failover scripts
----------------

If you need more complicated behavior when the slave loses contact with
the master, you can specify a failover script. Add ``--failover-script
<script>`` to the slave's command line. When the slave makes contact
with the master, it will execute the given script with the argument
``up``. When it loses contact, it will execute the script with the
argument ``down``. You can use this to trigger a custom response when
the master fails.

``````````````````````````````````````````````````
Using failover scripts to do "IP address stealing"
``````````````````````````````````````````````````

RethinkDB can load balance by manipulation of IP addresses. In this
scheme the slave, on failover will "steal" the fallen master's IP
address thus invisibly redirecting new connections to itself. The master
machine must be run with 2 IP addresses, one for user connections and
one for replication connections. This way the slave can steal the
master's user facing address but not the replication address thus
allowing it to reconnect when the master becomes available. Virtual IPs
can be setup on Linux like so:::

  user@master$ ifconfig eth0:1 192.168.0.2 up

And taken back down with::

  user@master$ ifconfig eth0:1 down

The slave side script which will facilitate this is:::

  #!/bin/bash
  if [ "$1" = "down" ]
  then
  ifconfig eth0:1 user_facing_ip_addr up
  fi
  if [ "$1" = "up" ]
  then
  ifconfig eth0:1 down
  fi

----------------------
The slave "goes rogue"
----------------------

If the slave loses and then regains contact with the master five times
in five minutes, it will assume that something is wrong with the master
machine and it will stop trying to reconnect to the master. It will
continue to accept writes. You will see a message in the slave's log
explaining that it has "gone rogue". When you fix whatever was causing
the master to behave so erratically, send the command ``rethinkdb
failover-reset`` to the slave over telnet to make it reconnect to the
master.

You can prevent the slave from going rogue by passing the ``--no-rogue``
flag on the slave's command line.

-------------------
Backfill operations
-------------------

When the slave connects to the master for the first time, or when the
slave and master are reunited after one of them goes down, they must
catch up to changes that have been made in their absence. This process
is called "backfilling". Specifically, backfilling occurs in the
following situations:

* When the slave connects to the master immediately after the slave was
  started or restarted, the master backfills to the slave.

* When the slave reconnects to the master after the master went down
  while the slave stayed up, the slave backfills to the master and then
  the master backfills to the slave. (The purpose of the second backfill
  is to resolve any divergence in the data.)

Due to an unfortunate limitation of RethinkDB's internal architecture, a
backfill operation cannot be interrupted, not even if the receiver of
the backfill disconnects during the backfill. If you try to shut down a
server while it is backfilling to another server, it will print ``Waiting
for operations to complete...`` and then stay in that state until the
backfill completes, which may take a long time. Unfortunately, there
isn't much you can do about this.

--------------------------
Relocating to new machines
--------------------------

Moving a slave to a new machine is easy. Shut down the old slave.
Optionally, copy the old slave's data files to the new machine. (If you
copy the data files, the slave will start up faster, but it's not
strictly necessary.) Start a new slave on the new machine.

Moving a master to a new machine is slightly harder. Shut down the old
master. Copy the old master's data files to the new machine. Start the
new master. Send the command ``rethinkdb new-master <host> <port>`` over
telnet to the slave. It will reconnect to the new master and transfer
any changes that occurred on the slave while you were relocating the
master. (Alternatively, you can just restart the slave with a different
value for the ``--slave-of`` parameter.)

------------------------
Hazards to watch out for
------------------------

If you run RethinkDB even once in non-slave-mode on a set of slave data
files, those data files will be irreversibly changed, and you won't be
able to use them in slave-mode ever again!

Don't use multiple slaves with the same master. If you try to connect a
second slave while a slave is already connected, RethinkDB will kick the
connection off. If you disconnect the slave and then connect another
one, RethinkDB will not complain, but if you later reconnect the
original slave, RethinkDB may become confused and the behavior is
undefined in this case. Your data may be corrupted.

-----------
Other notes
-----------

When you run ``rethinkdb serve --slave-of <master>:<port>``, the
database files must either be empty or must have come from a previous
run of ``rethinkdb serve --slave-of`` with the same master. If this is
not true, RethinkDB will display an error message and then crash.
RethinkDB identifies the "same master" using an identifier in the
master's data files, so the slave will work OK if you migrate the
master, including its data files, to a new machine.

Be careful if you shut down the slave, shut down the master, and then
start back up the master. The master will refuse to start back up again;
it will complain that it is waiting for a slave to connect. Fortunately,
this problem is easy to fix. If you start the slave up after starting
the master up, then the database will automatically fix the problem.
Alternatively, you can send ``rethinkdb dont-wait-for-slave`` to the
master over telnet, which will put the master back in the state it was
in before it was shut down.

=================  
Advanced features
=================  

--------------------
Advanced disk layout
--------------------

RethinkDB allows for tuning of the internal layout of the database
file. Depending on the underlying storage system, this may result in a
significant boost in performance.

````````````````````
Block device support
````````````````````

RethinkDB can bypass the file system and run directly on the block
device. In order for server to use a block device, the device
first needs to be formatted::

  $ rethinkdb create -f /dev/sdb

The database can be sharded across multiple devices::

  $ rethinkdb create -f /dev/sdb -f /dev/sdc

If an existing database was previously created on the device, the server will output an
error message. The block device can be reformatted by using the
``force`` argument::

  $ rethinkdb create -f /dev/sdb -f /dev/sdc --force

Once one or more block devices have been formatted, the database
server can be started as usual::

  $ rethinkdb -f /dev/sdb -f /dev/sdc

``````````
Block size
``````````

By default, RethinkDB uses a 4KB block size. In some cases larger
block sizes (8KB to 64KB) can yield higher performance. When the
database is created, the block size can be specified explicitly as
follows (in bytes)::

  $ rethinkdb create --block-size 8192 -f file.db

```````````
Extent size
```````````

Data blocks are grouped into ``extents``. Large extents often allow
for more efficient disk usage but may lower the performance of the
garbage collector. An extent size can be specified explicitly during
database creation as follows (in bytes)::

  $ rethinkdb create --extent-size 1048576 -f file.db

The above command formats the database with a 1MB extent
size. Normally, extents should be able to hold anywhere from 256 to
8192 blocks.

``````
Slices
``````

RethinkDB automatically partitions the database into independent
slices, which allows for efficient use of multiple disks and multicore
CPUs. The number of slices can be specified explicitly during database
creation time as follows::

  $ rethinkdb create --slices 256 -f file.db

-----------------
Garbage collector
-----------------

RethinkDB ships with a concurrent, incremental on-disk garbage
collector. Because the server uses a log-structured approach to
storage, the database file can fill with unused blocks that need to be
garbage collected. The garbage collector kicks in when there are too
many unused blocks in a file, and turns off when the number of unused
blocks reaches an acceptable level.

The window for garbage collector operation can be specified explicitly
on startup as follows::

  $ rethinkdb --gc-range 0.6-0.8

The above argument configures the garbage collector to kick in when
80% of the file contains unused blocks, and to stop
collecting when less than 60% of the file contains unused blocks.

An aggressive garbage collection setting will keep a larger proportion
of the disk available for live data, but may decrease performance of
the system because of higher load on the disk.

------------------
Consistency checks
------------------

RethinkDB allows verifying that a given database is consistent and has
not been corrupted. The corruption checks can be invoked as follows::

  $ rethinkdb fsck -f file.db

If the database file is corrupted, the command above will report an
error explaining the source of corruption.

----------------------
Advanced data recovery
----------------------

The recovery tool described in the `data migration and recovery`_ section
exposes options to recover data in situations where the tool cannot be run automatically because of substantial metadata corruption.
In such cases, block size, extent size, and slice numbers can be
specified explicitly to allow the tool to proceed::

  $ rethinkdb extract -f file.db --force-block-size 4096      \
                                 --force-extent-size 1048576  \
                                 --force-slice-count 256

-----------------------------
Memcached protocol extensions
-----------------------------

.. _undefined boundary extension:

`````````````````````````````````````````
Undefined range boundary in range queries
`````````````````````````````````````````

In the `rget specification`_ there's no provision for the support of undefined left/right
boundaries, which could potentially allow to stream all the database key-value pairs in the
increasing order. Since this feature may still be valuable in some scenarios, the following
extension to the ``rget`` command is implemented:

  To specify that the boundary is undefined, use the key name ``null`` (case insensitive) and
  openness flag of ``-1``.

Some examples of valid requests:

- ``rget null foo -1 1 100``

  Get at most 100 key-value pairs in ascending order starting from the smallest key in the database
  ending with the key ``foo``, and not including it.

- ``rget bar null 0 -1 343``

  Get at most 343 key-value pairs in ascending order starting from the key ``bar``.

- ``rget NULL nuLL -1 -1 9000``
  
  Get at most 9000 key-value pairs in ascending order starting from the smallest key in the database.

===============
Troubleshooting
===============

-----------------------------
"Too many open files" problem
-----------------------------

RethinkDB can consume a large number of open file handles, for example when the
number of socket connections is high. If you get a "Too many open files" error,
that means that the operating system limit on the number of open file handles
has been reached.

On most distributions of Linux you can find out the total limit for open file
handles in the system using ``sysctl``::

  $ sysctl fs.file-max
  fs.file-max = 764412

You can set this by running the following command under a root account or a
user account with sufficient privileges::

  $ sysctl fs.file-max=1592260
  fs.file-max = 1592260

You can also change the per-process limit temporarily (in the current shell
session), by using the ``ulimit`` command::

  $ ulimit -n 2048

Set the limit to an appropriate number (``2048`` in the example), that is higher
than the number of simultaneous connections to RethinkDB that you plan to have.

It is also possible to set per-user open file handles limits by editing
``/etc/security/limits.conf`` and setting the soft and hard limit values for
``nofile`` for the user or group which you use to run the RethinkDB under::

  rethinkdb soft nofile 2048
  rethinkdb hard nofile 8192

=======
Support
=======

Please report all issues to ``support@rethinkdb.com``. When reporting
an issue, please try to include the following pieces of information:

- A description of the environment you're running in (operating
  system, kernel version, hardware, etc).
- A description of the problem, how it came about, and how it can be
  reproduced.
- The RethinkDB log file. By default, log messages are written to standard
  output. In a production environment you may want to point them to a
  file on disk for easy collection using ``--log-file`` argument.
- If the problem involves a crash, please include the core dump file
  associated with the error. Core dumps are usually named ``core``
  and are placed into the directory where the server was run. If you do
  not see a core dump file, you may need to enable core dumps by
  running the ``ulimit -c unlimited`` command.

.. [#trial-binary] If you're using the trial version of the server, the executable will be named ``rethinkdb-trial``, so be sure to adjust the command-lines accordingly.

.. _`Range queries`: http://memcachedb.googlecode.com/svn/trunk/doc/rget.txt
.. _`rget specification`: http://memcachedb.googlecode.com/svn/trunk/doc/rget.txt

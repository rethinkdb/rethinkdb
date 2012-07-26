Guide to the `rethinkdb/test/` directory
---------------------------------------

RethinkDB has two types of tests: unit tests and integration tests. Unit tests
live in `rethinkdb/src/unittest/` and are outside the scope of this document.

RethinkDB tests are run nightly. The infrastructure for running nightly tests
lives in `rethinkdb/scripts/nightly-test/` and `rethinkdb/scripts/test-host/`
and is outside the scope of this document. The list of tests to be run nightly
lives here in `rethinkdb/test/full_test/`; see the "Nightly test lists" section
for more information.

Workloads
---------

A workload is a command that runs queries against a RethinkDB server at some
host and ports. Workloads are specified as a command which, when executed in
`bash` with environment variables `HOST`, `HTTP_PORT`, and `MC_PORT` set
appropriately, will open some connections to the database and run queries.
`HTTP_PORT` is a RethinkDB administrative HTTP server; the workload mustn't
change the server's configuration. `MC_PORT` is a memcached server. `PORT` is an
alias for `MC_PORT`. There are two types of workloads: "continuous" workloads
are workloads that run until they receive `SIGINT`, but "discrete" workloads
will stop on their own. A workload is considered to have succeeded if it returns
0.

Here are some examples:
* A trivial discrete workload: 
    `true`
* A discrete workload using the stress client:
    `rethinkdb/bench/stress-client/stress -s $HOST:$MC_PORT`
* A continuous workload using the stress client:
    `rethinkdb/bench/stress-client/stress -s $HOST:$MC_PORT --duration infinity`

In the `rethinkdb/test/workloads/` directory, there are many workload scripts
written in Python. For example:
* A discrete workload that tests append and prepend operations:
    `rethinkdb/test/workloads/append_prepend.py $HOST:$MC_PORT`
* A continuous workload that sends a variety of operations:
    `rethinkdb/test/workloads/serial_mix.py $HOST:$MC_PORT --duration forever`

A workload may create files in its current directory. The main purpose of this
is when several workloads are meant to be run in series. The first one can put
some data in the database and then record what it wrote to a file; the second
one can open the file and verify that the data is still present in the database.
`rethinkdb/test/workloads/serial_mix.py $HOST:$MC_PORT --save data_file` and
`rethinkdb/test/workloads/serial_mix.py $HOST:$MC_PORT --load data_file` are
such a pair of workloads. Sometimes this construct is called a "split workload"
or "two-phase workload". Workloads that can be run an arbitrary number of times
against the same server, saving data in between, are called "n-phase workloads".

Scenarios
---------

A scenario is a script that starts up one or more instances of RethinkDB and
runs a workload or workloads against them. The scenarios live in
`rethinkdb/test/scenarios/`.

Here are some examples:
* `scenarios/static_cluster.py <DISCRETE WORKLOAD>` starts a cluster of three
    RethinkDB instances, runs the workload against one of the instances in the
    cluster, and then shuts down the cluster.
* `restart.py <DISCRETE WORKLOAD 1> <DISCRETE WORKLOAD 2>` starts a RethinkDB
    instance; runs the first workload; shuts down and restarts the RethinkDB
    instance; runs the second workload; and then shuts down the RethinkDB
    instance. It's usually used with a split workload, to test that the data is
    correctly persisted to disk when RethinkDB is restarted.
* `more_or_less_secondaries.py 1+1 --workload-during <CONTINUOUS WORKLOAD>` starts
    a RethinkDB cluster; starts the workload; increases the number of
    secondaries in the cluster; stops the workload; and stops the cluster. The
    idea is to make sure that RethinkDB doesn't crash if the number of
    secondaries is changed while queries are being sent.
* `more_or_less_secondaries.py 1+1 --workload-before <DISCRETE WORKLOAD 1> --workload-after <DISCRETE WORKLOAD 2>` is
    like above except that instead of running one continuous workload the whole
    time, it runs two separate discrete workloads; one before adding the new
    secondary and one after. It can be used to make sure that data is not
    corrupted when a secondary is added to the cluster.

Like workloads, scenarios are allowed to leave temporary files--such as the
server's database files--in the local directory. These are important for
debugging purposes, but sometimes they will get confused by left-over temporary
files from a previous run. To avoid that, and to keep the local directory clean,
I recommend running tests something like this:

    ~/rethinkdb/test$ (rm -rf scratch; mkdir scratch; cd scratch; ../scenarios/<SCENARIO> <ARGUMENTS...>)

For example, to run the `static_cluster.py` scenario with the
`append_prepend.py` workload, you could type:

    ~/rethinkdb/test$ (rm -rf scratch; mkdir scratch; cd scratch; ../scenarios/static_cluster.py '../workloads/append_prepend.py $HOST:$MC_PORT')

Interface tests
---------------

An interface test is a test that starts a server to test some part of its
interface, but its primary purpose is not to run queries against the server.
Interface tests live in `rethinkdb/test/interface/`. They usually take no
parameters on the command line.

Some examples:
* `log.py` starts a server and fetches log entries from it over the HTTP
    interface.
* `progress.py` causes a backfill to happen and makes sure that the progress of
    the backfill can be monitored through the HTTP interface

Like scenario tests, interface tests are allowed to leave junk in the local
directory.

Support modules
---------------

The `rethinkdb/test/common/` directory contains a number of Python modules to
assist in writing tests. Here are the important ones:

`driver.py` contains facilities for starting a cluster of RethinkDB servers. It
automatically takes care of starting the processes, connecting them to each
other, and shutting them down. It also has facilities for simulating netsplits
using the `resunder.py` daemon, which is in the same directory.

`http_admin.py` is a Python wrapper around RethinkDB's HTTP interface.

The other tests, such as the ones in `scenarios/`, use `driver.py` to start
RethinkDB instances and use `http_admin.py` to create namespaces, increase or
decrease replication factors, and so on.

Nightly test lists
------------------

The nightly test infrastructure scans the directory `rethinkdb/test/full_test/`
to figure out what tests to run. It executes each `.test` file in that directory
as a Python script, with the `do_test()` function defined; it traps calls to
`do_test()` and builds the test list that way. If you create a new scenario,
workload, or interface test, please add it to the appropriate `.test` file so
that it gets run every night.

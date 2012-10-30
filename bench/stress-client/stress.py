# Copyright 2010-2012 RethinkDB, all rights reserved.
import ctypes, os
from ctypes import POINTER

"""The "stress" module provides a Python interface to the stress client."""

# Load the shared library that has the guts of the stress client

my_dir = os.path.split(__file__)[0]
libstress_path = os.path.abspath(os.path.join(my_dir, "libstress.so"))
assert os.path.exists(libstress_path), 'libstress.so was not found at path %r' % libstress_path
libstress = ctypes.cdll.LoadLibrary(libstress_path)

def declare_struct(name):
    globals()["libstress_%s" % name] = type(name, (ctypes.Structure,), {})

def declare_fun(fname, ret, *args):
    fun = libstress[fname]
    fun.restype = ret
    fun.argtypes = args
    globals()["libstress_%s" % fname] = fun

# These declarations must be kept in sync with python_interface.h

declare_struct("protocol_t")
declare_fun("protocol_create", POINTER(libstress_protocol_t), ctypes.c_char_p)
declare_fun("protocol_destroy", None, POINTER(libstress_protocol_t))
declare_struct("op_generator_t")
declare_fun("op_generator_destroy", None, POINTER(libstress_op_generator_t))
declare_fun("op_generator_lock", None, POINTER(libstress_op_generator_t))
declare_fun("op_generator_poll", None, POINTER(libstress_op_generator_t), POINTER(ctypes.c_int), POINTER(ctypes.c_float), POINTER(ctypes.c_int), POINTER(ctypes.c_float))
declare_fun("op_generator_reset", None, POINTER(libstress_op_generator_t))
declare_fun("op_generator_unlock", None, POINTER(libstress_op_generator_t))
declare_struct("client_t")
declare_fun("client_create", POINTER(libstress_client_t))
declare_fun("client_destroy", None, POINTER(libstress_client_t))
declare_fun("client_add_op", None, POINTER(libstress_client_t), ctypes.c_int, POINTER(libstress_op_generator_t))
declare_fun("client_start", None, POINTER(libstress_client_t))
declare_fun("client_stop", None, POINTER(libstress_client_t))
declare_struct("seed_key_generator_t")
declare_fun("seed_key_generator_create", POINTER(libstress_seed_key_generator_t), ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_int)
declare_fun("seed_key_generator_destroy", None, POINTER(libstress_seed_key_generator_t))
declare_struct("value_watcher_t")
declare_struct("existence_watcher_t")
declare_fun("existence_watcher_as_value_watcher", POINTER(libstress_value_watcher_t), POINTER(libstress_existence_watcher_t))
declare_struct("existence_tracker_t")
declare_struct("value_tracker_t")
declare_fun("value_tracker_as_existence_tracker", POINTER(libstress_existence_tracker_t), POINTER(libstress_value_tracker_t))
declare_struct("seed_chooser_t")
declare_fun("seed_chooser_destroy", None, POINTER(libstress_seed_chooser_t))
declare_fun("op_generator_create_read", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_seed_key_generator_t), POINTER(libstress_seed_chooser_t), POINTER(libstress_protocol_t), ctypes.c_int, ctypes.c_int)
declare_fun("op_generator_create_insert", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_seed_key_generator_t), POINTER(libstress_seed_chooser_t), POINTER(libstress_value_watcher_t), POINTER(libstress_protocol_t), ctypes.c_int, ctypes.c_int)
declare_fun("op_generator_create_update", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_seed_key_generator_t), POINTER(libstress_seed_chooser_t), POINTER(libstress_value_watcher_t), POINTER(libstress_protocol_t), ctypes.c_int, ctypes.c_int)
declare_fun("op_generator_create_delete", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_seed_key_generator_t), POINTER(libstress_seed_chooser_t), POINTER(libstress_value_watcher_t), POINTER(libstress_protocol_t))
declare_fun("op_generator_create_append_prepend", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_seed_key_generator_t), POINTER(libstress_seed_chooser_t), POINTER(libstress_value_watcher_t), POINTER(libstress_protocol_t), ctypes.c_int, ctypes.c_int, ctypes.c_int)
declare_fun("op_generator_create_percentage_range_read", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_protocol_t), ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_char_p)
declare_fun("op_generator_create_calibrated_range_read", POINTER(libstress_op_generator_t), ctypes.c_int, POINTER(libstress_existence_tracker_t), POINTER(libstress_protocol_t), ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_char_p)
declare_struct("consecutive_seed_model_t")
declare_fun("consecutive_seed_model_create", POINTER(libstress_consecutive_seed_model_t))
declare_fun("consecutive_seed_model_destroy", None, POINTER(libstress_consecutive_seed_model_t))
declare_fun("consecutive_seed_model_as_existence_watcher", POINTER(libstress_existence_watcher_t), POINTER(libstress_consecutive_seed_model_t))
declare_fun("consecutive_seed_model_as_existence_tracker", POINTER(libstress_existence_tracker_t), POINTER(libstress_consecutive_seed_model_t))
declare_fun("consecutive_seed_model_make_insert_chooser", POINTER(libstress_seed_chooser_t), POINTER(libstress_consecutive_seed_model_t))
declare_fun("consecutive_seed_model_make_delete_chooser", POINTER(libstress_seed_chooser_t), POINTER(libstress_consecutive_seed_model_t))
declare_fun("consecutive_seed_model_make_live_chooser", POINTER(libstress_seed_chooser_t), POINTER(libstress_consecutive_seed_model_t), ctypes.c_char_p, ctypes.c_int)
declare_struct("fuzzy_model_t")
declare_fun("fuzzy_model_create", POINTER(libstress_fuzzy_model_t), ctypes.c_int)
declare_fun("fuzzy_model_destroy", None, POINTER(libstress_fuzzy_model_t))
declare_fun("fuzzy_model_make_random_chooser", POINTER(libstress_seed_chooser_t), POINTER(libstress_fuzzy_model_t), ctypes.c_char_p, ctypes.c_int)
declare_fun("py_initialize_mysql_table", None, ctypes.c_char_p, ctypes.c_int, ctypes.c_int)

# Where the functions below accept a distribution, they expect either a single
# number or a tuple of (min, max).

def distr_get(distr, i):
    if isinstance(distr, (int, float)):
        return distr
    elif isinstance(distr, (list, tuple)) and len(distr)==2 and \
            isinstance(distr[0], (int, float)) and isinstance(distr[1], (int, float)):
        return distr[i]
    else:
        raise ValueError("Distribution should be either a number or a 2-element tuple. "
            "Instead got %r." % distr)

def distr_min(distr):
    return distr_get(distr, 0)

def distr_max(distr):
    return distr_get(distr, 1)

# Connection corresponds to protocol_t in the C++ stress client.

class Connection(object):
    """Represents a connection to a server being benchmarked. The constructor
    expects a string in the format that the stress client uses."""

    def __init__(self, server_str):
        assert isinstance(server_str, str)
        self._connection = libstress_protocol_create(server_str)
        self.client = None

    def __del__(self):
        if hasattr(self, "_connection"):
            libstress_protocol_destroy(self._connection)

# OpGenerator corresponds to op_generator_t in the C++ stress client.

class OpGenerator(object):
    """Base class for all benchmarkable operations. If you have an OpGenerator
    object called "opg", you can extract stats from it as follows:
    >>> opg.lock()
    >>> stats = opg.poll()
    >>> opg.reset()   # Optional; resets the stat counters back to zero
    >>> opg.unlock()
    """

    def __init__(self, _opg):
        assert isinstance(_opg, POINTER(libstress_op_generator_t))
        self._opg = _opg
        self.client = None
        self.locked = False

    def on_add(self, client):
        assert self.client is None, "Can't add the same op to two clients."
        self.client = client

    def lock(self):
        assert not self.locked
        self.locked = True
        libstress_op_generator_lock(self._opg)

    def poll(self):
        """Returns a dictionary containing three keys:
        "queries": The number of queries performed since creation or since
            the last call to reset().
        "worst_latency": The latency of the highest-latency operation
            performed since creation or the last call to reset().
        "latency_samples": A list with a random sampling of operation latencies;
            you can compute the average or various percentiles from this
            information.
        """
        assert self.locked or not self.client or not self.client.running, \
            "It's not safe to poll() an Op attached to a running client without lock()ing it first."

        queries = ctypes.c_int()
        worst_latency = ctypes.c_float()
        samples_count = ctypes.c_int(100)
        samples = (ctypes.c_float * samples_count.value)()

        libstress_op_generator_poll(self._opg,
            ctypes.byref(queries),
            ctypes.byref(worst_latency),
            ctypes.byref(samples_count),
            samples,
            )

        return {
            "queries": queries.value,
            "worst_latency": worst_latency.value,
            "latency_samples": [samples[i] for i in xrange(samples_count.value)],
            }

    def reset(self):
        assert self.locked or not self.client or not self.client.running, \
            "It's not safe to reset() an OpGenerator attached to a running client without lock()ing it first."
        libstress_op_generator_reset(self._opg)

    def unlock(self):
        assert self.locked
        self.locked = False
        libstress_op_unlock(self._opg)

    def __del__(self):
        if hasattr(self, "locked"):
            assert not self.locked, "OpGenerator.unlock() was never called."
        if hasattr(self, "_opg"):
            libstress.op_destroy(self._opg)

# Client corresponds to client_t in the C++ stress client.

class Client(object):
    """A Client is a single sequential thread of execution that sends queries to the
    server. No two operations on one client can be run concurrently. This means that
    each Connection, Op, and ConsecutiveSeedModel can only be associated with one
    client, or else there will be bad cross-thread accesses. Create a client, call
    add_op() on it repeatedly, then call start(). When you're done, call stop()."""

    def __init__(self):
        self._client = libstress_client_create()
        self.ops = []
        self.running = False

    def add_op(self, freq, opg):
        assert isinstance(freq, int)
        assert freq >= 0
        assert isinstance(opg, OpGenerator)
        opg.on_add(self)
        self.ops.append(opg)
        libstress_client_add_op(self._client, freq, opg._opg)

    def start(self):
        assert not self.running
        self.running = True
        libstress_client_start(self._client)

    def stop(self):
        assert self.running
        self.running = False
        libstress_client_stop(self._client)

    def __del__(self):
        if hasattr(self, "running"):
            assert not self.running, "Client.stop() was never called."
        if hasattr(self, "_client"):
            libstress_client_destroy(self._client)

# SeedKeyGenerator corresponds to seed_key_generator_t in the C++ stress client.

class SeedKeyGenerator(object):
    """SeedKeyGenerator determines the mapping from seeds to keys. Most Ops expect
    a SeedKeyGenerator."""

    def __init__(self, shard=None, prefix="", keysize=(8,16)):
        if shard is None:
            shard_id = shard_count = -1
        else:
            shard_id, shard_count = shard
            assert shard_id >= 0
            assert shard_id < shard_count
        self._skgen = libstress_seed_key_generator_create(shard_id, shard_count, prefix, distr_min(keysize), distr_max(keysize))

    def __del__(self):
        if hasattr(self, "_skg"):
            libstress_seed_key_generator_destroy(self._skgen)

# ValueWatcher and ExistenceWatcher correspond to value_watcher_t and existence_watcher_t.
# They are abstract mixin classes.

class ValueWatcher(object):

    def __init__(self, _vw):
        assert isinstance(_vw, POINTER(libstress_value_watcher_t))
        self._value_watcher = _vw

class ExistenceWatcher(ValueWatcher):

    def __init__(self, _ew):
        assert isinstance(_ew, POINTER(libstress_existence_watcher_t))
        self._existence_watcher = _ew
        ValueWatcher.__init__(self, libstress_existence_watcher_as_value_watcher(_ew))

# ValueTracker and ExistenceTracker correspond to value_tracker_t and existence_tracker_t.
# They are abstract mixin classes.

class ExistenceTracker(object):

    def __init__(self, _et):
        assert isinstance(_et, POINTER(libstress_existence_tracker_t))
        self._existence_tracker = _et

class ValueTracker(object):

    def __init__(self, _vt):
        assert isinstance(_vt, POINTER(libstress_value_tracker_t))
        self._value_tracker = _vt
        ExistenceTracker.__init__(self, libstress_value_tracker_as_existence_tracker(_vt))

# SeedChooser corresponds to seed_chooser_t in the C++ stress client.

class SeedChooser(object):
    """A SeedChooser is an abstract class; each concrete subclass of SeedChooser represents
    a different way of choosing which keys to operate on. To get a SeedChooser, construct a
    ConsecutiveSeedModel or FuzzyModel and then call one of its *_chooser() methods."""

    def __init__(self, _sch):
        assert isinstance(_sch, POINTER(libstress_seed_chooser_t))
        self._sch = _sch

    def __del__(self):
        if hasattr(self, "_sch"):
            libstress_seed_chooser_destroy(self._sch)

# SingleConnectionOpGenerator is a common superlcass for ops that have a
# seed_key_generator_t, a seed_chooser_t, a protocol_t, and optionally a
# value_watcher_t.

class SingleConnectionOpGenerator(OpGenerator):

    def __init__(self, conn, _opg):
        assert isinstance(conn, Connection)
        self.connection = conn
        OpGenerator.__init__(self, _opg)

    def on_add(self, client):
        assert self.connection.client is None or self.connection.client is client, \
            "Can't use the same connection with ops that are associated with two " + \
            "different clients."
        if self.connection.client is None: self.connection.client = client
        OpGenerator.on_add(self, client)

class SimpleOpGenerator(SingleConnectionOpGenerator):

    def __init__(self, skgen, sch, vw, conn, _opg):
        assert isinstance(skgen, SeedKeyGenerator)
        self.seed_key_generator = skgen
        assert isinstance(sch, SeedChooser)
        self.seed_chooser = sch
        assert vw is None or isinstance(vw, ValueWatcher)
        self.value_watcher = vw
        SingleConnectionOpGenerator.__init__(self, conn, _opg)

class ReadOpGenerator(SimpleOpGenerator):
    """ReadOpGenerator is an operation that performs reads against the database.
    The parameters to its constructor are:
    1. A SeedKeyGenerator, which determines how it interprets the seeds it gets
    2. A SeedChooser, which determines which keys it will look up
    3. A Connection, which is where it will submit its queries to
    4. The batch factor distribution, as a number or a tuple of (low, high)
    Please note that add_op() does not account for batch_factor when it comes to
    the frequency of a ReadOpGenerator! You might want to divide frequency by
    the average batch_factor to get a more reasonable distribution of
    operations.
    """

    def __init__(self, nops, skgen, sch, conn, batch_factor = (1,16)):
        SimpleOpGenerator.__init__(self, skgen, sch, None, conn,
            libstress_op_generator_create_read(
                nops, skgen._skgen, sch._sch, conn._connection,
                distr_min(batch_factor), distr_max(batch_factor)))

class InsertOpGenerator(SimpleOpGenerator):
    """InsertOpGenerator performs insertions into the database. Its constructor
    has these parameters:
    1. A SeedKeyGenerator, which determines how it interprets the seeds it gets
    2. A SeedChooser, which determines which keys it will insert
    3. A ValueWatcher, which it reports its changes to; if you use it with a
       ConsecutiveSeedModel this should be the ConsecutiveSeedModel, or else it
       can be None.
    4. The connection over which to send the query, as a Connection
    5. A value size distribution, as a number or a tuple of (low, high)
    """

    def __init__(self, nops, skgen, sch, vw, conn, size=(8,16)):
        SimpleOpGenerator.__init__(self, skgen, sch, vw, conn,
            libstress_op_generator_create_insert(
                nops, skgen._skgen, sch._sch,
                vw._value_watcher if vw is not None else None,
                conn._connection,
                distr_min(size), distr_max(size)))

class UpdateOpGenerator(SimpleOpGenerator):
    """UpdateOpGenerator is like InsertOpGenerator except that, depending on the
    protocol used, the commands it sends over the wire might be no-ops if the
    key to be updated does not already exist. Its constructor has the same
    parameters."""

    def __init__(self, nops, skgen, sch, vw, conn, size=(8,16)):
        SimpleOpGenerator.__init__(self, skgen, sch, vw, conn,
            libstress_op_generator_create_update(
                nops, skgen._skgen, sch._sch,
                vw._value_watcher if vw is not None else None,
                conn._connection,
                distr_min(size), distr_max(size)))

class DeleteOpGenerator(SimpleOpGenerator):
    """DeleteOpGenerator deletes keys. Its constructor has these parameters:
    1. A SeedKeyGenerator, which determines how it interprets the seeds it gets
    2. A SeedChooser, which decides which keys to delete
    3. A ValueWatcher, which it reports its changes to
    4. The connection over which to send the query, as a Connection
    """

    def __init__(self, nops, skgen, sch, vw, conn):
        SimpleOpGenerator.__init__(self, skgen, sch, vw, conn,
            libstress_op_generator_create_delete(
                nops, skgen._skgen, sch._sch,
                vw._value_watcher if vw is not None else None,
                conn._connection))

class AppendPrependOpGenerator(SimpleOpGenerator):
    """AppendPrependOpGenerator appends or prepends to values. Its constructor
    has these parameters:
    1. A SeedKeyGenerator, which determines how it interprets the seeds it gets
    2. A SeedChooser, which decides which keys to append or prepend to
    3. A ValueWatcher, which is what it reports its changes to
    4. The connection over which to send the query, as a Connection
    5. The string "append" or "prepend"
    6. A size distribution for the prefixes or suffixes, as a number or as a
       tuple of (low, high)
    """

    def __init__(self, nops, skgen, sch, vw, conn, mode="append", size=(8,16)):
        if mode == "append": mode_int = 1
        elif mode == "prepend": mode_int = 0
        else: raise ValueError("'mode' should be 'append' or 'prepend'")
        SimpleOpGenerator.__init__(self, skgen, sch, vw, conn,
            libstress_op_generator_create_append_prepend(
                nops, skgen._skgen, sch._sch,
                vw._value_watcher if vw is not None else None,
                conn._connection,
                mode_int, distr_min(size), distr_max(size)))

class PercentageRangeReadOpGenerator(SingleConnectionOpGenerator):
    """PercentageRangeReadOpGenerator performs range reads over a chosen
    percentage of the database. Its constructor has these parameters:
    1. A Connection over which to send the query
    2. The percentage of the database to request each time, as a number or as
       a distribution of (low, high)
    3. The maximum number of keys to request at a time, as a number or as a
       distribution of (low, high)
    4. Optionally, a prefix to put on the keys
    PercentageRangeReadOpGenerator assumes that the database has been populated 
    by keys from one or more SeedKeyGenerators. The prefix you pass to
    PercentageRangeReadOpGenerator's constructor should be the same prefix you
    passed to the SeedKeyGenerator."""

    def __init__(self, nops, conn, percentage=(10,50), limit=(16,128), prefix=""):
        SingleConnectionOpGenerator.__init__(self, conn,
            libstress_op_generator_create_percentage_range_read(
                nops, conn._connection,
                distr_min(percentage), distr_max(percentage),
                distr_min(limit), distr_max(limit),
                prefix))

class CalibratedRangeReadOpGenerator(SingleConnectionOpGenerator):
    """CalibratedRangeReadOpGenerator performs range reads just like
    PercentageRangeReadOpGenerator, but it tries to guess how many keys the
    range will contain by examining a model. Its constructor has these
    parameters:
    1. The model to consult, which must be an ExistenceTracker.
    2. A multiplier factor; the CalibratedRangeReadOpGenerator will assume that
       the total number of keys in the database is the number in the model times
       the multiplier.
    3. The number of keys to try to grab at a time, as a number or distribution of
       (low, high).
    4. The maximum number of keys to request at a time, as a number or as a tuple
       of (low, high).
    5. Optionally, a prefix to put on the keys
    Like CalibratedRangeReadOpGenerator, PercentageRangeReadOpGenerator assumes
    that SeedKeyGenerator was used to populate the database and the prefix you
    pass to the CalibratedRangeReadOpGenerator constructor should be the same
    one passed to the SeedKeyGenerator."""

    def __init__(self, nops, conn, rangesize=(10,50), limit=(16,128), prefix=""):
        SingleConnectionOpGenerator.__init__(self, conn,
            libstress_op_generator_create_calibrated_range_read(
                nops, conn._connection,
                distr_min(rangesize), distr_max(rangesize),
                distr_min(limit), distr_max(limit),
                prefix))

class ConsecutiveSeedModel(ExistenceWatcher, ExistenceTracker):
    """ConsecutiveSeedModel keeps track of which keys are live in the database
    by making sure that the seeds of the live keys are consecutive. To run an
    operation against keys that are known to be live, call the live_chooser()
    method and pass the returned SeedChooser to the Op constructor. If you run
    inserts or deletes against the database, you must use insert_chooser() or
    delete_chooser() as the SeedChooser to make sure that the key range remains
    consecutive. You must also pass the ConsecutiveSeedModel as the "value_watcher"
    parameter to the InsertOp or DeleteOp so that it stays up to date."""

    def __init__(self):
        self._csm = libstress_consecutive_seed_model_create()
        ExistenceWatcher.__init__(self, libstress_consecutive_seed_model_as_existence_watcher(self._csm))
        ExistenceTracker.__init__(self, libstress_consecutive_seed_model_as_existence_tracker(self._csm))

    def __del__(self):
        if hasattr(self, "_csm"):
            libstress_consecutive_seed_model_destroy(self._csm)

    def insert_chooser(self):
        return SeedChooser(libstress_consecutive_seed_model_make_insert_chooser(self._csm))

    def delete_chooser(self):
        return SeedChooser(libstress_consecutive_seed_model_make_delete_chooser(self._csm))

    def live_chooser(self, distr="uniform", mu=1):
        """Returns a SeedChooser that always picks live keys. You can customize the
        random distribution that will be used to pick keys."""
        assert distr in ["uniform", "normal"]
        return SeedChooser(libstress_consecutive_seed_model_make_live_chooser(self._csm, distr, mu))

class FuzzyModel(object):
    """FuzzyModel doesn't keep track of which keys exist. Its random_chooser()
    just picks random keys from a fixed-size range, the size of which is passed
    to the FuzzyModel constructor."""

    def __init__(self, nkeys = 1000):
        self._fm = libstress_fuzzy_model_create(nkeys)

    def __del__(self):
        if hasattr(self, "_fm"):
            libstress_fuzzy_model_destroy(self._fm)

    def random_chooser(self, distr="uniform", mu=1):
        assert distr in ["uniform", "normal"]
        return SeedChooser(libstress_fuzzy_model_make_random_chooser(self._fm, distr, mu))

def initialize_mysql_table(string, max_key, max_value):
    """If you intend to run the stress client against a MySQL server, you must set up a
    table and key/value columns. This convenience function does that work automatically.
    Call initialize_mysql_table(conn_str, max_key, max_value), where conn_str is the
    connection string for the MySQL protocol (without the "mysql," part) and max_key
    and max_value are the sizes of the largest keys and values that will ever be inserted
    by the workload you intend to run against the server."""
    assert isinstance(string, str)
    assert isinstance(max_key, int)
    assert isinstance(max_value, int)
    libstress.py_initialize_mysql_table(string, max_key, max_value)

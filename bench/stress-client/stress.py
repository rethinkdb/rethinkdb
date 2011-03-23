import ctypes, os

# Load the shared library that has the guts of the stress client

my_dir = os.path.split(__file__)[0]
libstress_path = os.path.abspath(os.path.join(my_dir, "libstress.so"))
assert os.path.exists(libstress_path)
libstress = ctypes.cdll.LoadLibrary(libstress_path)

# These functions have a void* return type, but ctypes defaults to int as the
# return type. To avoid possible issues where void* and int are not interchangeable,
# we explicitly specify their return types.

for fname in ["protocol_create", "op_create_read", "op_create_insert", "op_create_update",
        "op_create_delete", "op_create_appendprepend", "op_create_rangeread", "client_create"]:
    fun = libstress[fname]
    fun.restype = ctypes.c_void_p
    globals()["libstress_%s" % fname] = fun

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
    def __init__(self, server_str):
        assert isinstance(server_str, str)
        self._connection = libstress_protocol_create(server_str)
        self.client = None
    def __del__(self):
        if hasattr(self, "_connection"):
            libstress.protocol_destroy(self._connection)

# Op corresponds to op_t in the C++ stress client.

class Op(object):

    def __init__(self, client, _op):
        assert isinstance(client, Client)
        self.client = client
        assert not client.running
        client.ops.append(self)
        self._op = _op

    def poll(self):
        assert self.client.locked or not self.client.running

        queries = ctypes.c_int()
        worst_latency = ctypes.c_float()
        samples_count = ctypes.c_int(100)
        samples = (ctypes.c_float * samples_count.value)()

        libstress.op_poll(self._op,
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
        assert self.client.locked or not self.client.running
        libstress.op_reset(self._op)

    def __del__(self):
        if hasattr(self, "_op"):
            libstress.op_destroy(self._op)

class SingleConnectionOp(Op):
    def __init__(self, client, connection, _op):
        assert isinstance(connection, Connection)
        self.connection = connection
        assert connection.client is None or connection.client is client
        connection.client = client
        Op.__init__(self, client, _op)

class ReadOp(SingleConnectionOp):
    def __init__(self, client, connection, freq=1, batch_factor = (1,16)):
        SingleConnectionOp.__init__(self, client, connection,
            libstress_op_create_read(
                client._client, connection._connection, freq,
                distr_min(batch_factor), distr_max(batch_factor)))

class InsertOp(SingleConnectionOp):
    def __init__(self, client, connection, freq=1, size=(8,16)):
        SingleConnectionOp.__init__(self, client, connection,
            libstress_op_create_insert(
                client._client, connection._connection, freq,
                distr_min(size), distr_max(size)))

class UpdateOp(SingleConnectionOp):
    def __init__(self, client, connection, freq=1, size=(8,16)):
        SingleConnectionOp.__init__(self, client, connection,
            libstress_op_create_update(
                client._client, connection._connection, freq,
                distr_min(size), distr_max(size)))

class DeleteOp(SingleConnectionOp):
    def __init__(self, client, connection, freq=1):
        SingleConnectionOp.__init__(self, client, connection,
            libstress_op_create_delete(
                client._client, connection._connection, freq))

class AppendPrependOp(SingleConnectionOp):
    def __init__(self, client, connection, freq=1, mode="append", size=(8,16)):
        if mode == "append": mode_int = 1
        elif mode == "prepend": mode_int = 0
        else: raise ValueError("'mode' should be 'append' or 'prepend'")
        SingleConnectionOp.__init__(self, client, connection,
            libstress_op_create_appendprepend(
                client._client, connection._connection, freq,
                mode_int, distr_min(size), distr_max(size)))

class RangeReadOp(SingleConnectionOp):
    def __init__(self, client, connection, freq=1, rangesize=(16,128)):
        SingleConnectionOp.__init__(self, client, connection,
            libstress_op_create_rangeread(
                client._client, connection._connection, freq,
                distr_min(rangesize), distr_max(rangesize)))

# Client corresponds to client_t in the C++ stress client.

class Client(object):

    def __init__(self, id = 0, n_clients = 1, keys=(8,16)):
        assert isinstance(id, int)
        assert isinstance(n_clients, int)
        assert id < n_clients
        assert id >= 0
        self._client = libstress_client_create(id, n_clients, distr_min(keys), distr_max(keys))
        self.ops = []
        self.locked = False
        self.running = False

    def start(self):
        assert not self.running
        self.running = True
        libstress.client_start(self._client)

    def lock(self):
        assert not self.locked
        assert self.running
        self.locked = True
        libstress.client_lock(self._client)

    def unlock(self):
        assert self.locked
        self.locked = False
        libstress.client_unlock(self._client)

    def stop(self):
        assert self.running
        assert not self.locked
        self.running = False
        libstress.client_stop(self._client)

    def __del__(self):
        if hasattr(self, "_client"):
            libstress.client_destroy(self._client)

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

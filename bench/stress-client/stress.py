import ctypes, os

my_dir = os.path.split(__file__)[0]
libstress_path = os.path.abspath(os.path.join(my_dir, "libstress.so"))
assert os.path.exists(libstress_path)
libstress = ctypes.cdll.LoadLibrary(libstress_path)

class StressClient(object):

    def __init__(self, server_str, id = 0, n_clients = 1, options = {}):

        opts = (ctypes.c_char_p * (len(options) * 2 + 1))()
        i = 0
        for k,v in options.iteritems():
            opts[i] = str(k);
            opts[i+1] = str(v);
            i += 1;
        opts[i] = None

        self._client = libstress.client_create(server_str, id, n_clients, opts)

    def start(self):
        libstress.client_start(self._client)

    def poll(self):
        queries = ctypes.c_int()
        inserts_minus_deletes = ctypes.c_int()
        worst_latency = ctypes.c_float()
        n_latency_samples = ctypes.c_int(100)
        latency_samples = (ctypes.c_float * n_latency_samples.value)()

        libstress.client_poll(self._client,
            ctypes.byref(queries),
            ctypes.byref(inserts_minus_deletes),
            ctypes.byref(worst_latency),
            ctypes.byref(n_latency_samples),
            latency_samples,
            )

        return {
            "queries": queries.value,
            "inserts_minus_deletes": inserts_minus_deletes.value,
            "worst_latency": worst_latency.value,
            "latency_samples": [latency_samples[i] for i in xrange(n_latency_samples.value)],
            }

    def stop(self):
        libstress.client_stop(self._client)

    def __del__(self):
        libstress.client_destroy(self._client)

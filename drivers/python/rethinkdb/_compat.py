import sys

PY2 = sys.version_info[0] == 2

if PY2:
    str_types = basestring
    int_types = (int, long)
    numeric_types = (int, long, float, complex)
else:
    str_types = str
    int_types = (int,)
    numeric_types = (int, float, complex)

if PY2:
    dict_items = lambda d: d.iteritems()
    dict_keys = lambda d: d.iterkeys()
    range_ = xrange
else:
    dict_items = lambda d: d.items()
    dict_keys = lambda d: d.keys()
    range_ = range

if PY2:
    get_unbound_func = lambda f: f.__func__
else:
    get_unbound_func = lambda f: f

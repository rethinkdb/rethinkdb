# Load the C++ protobuf backend

from os import environ

# This rethinkdb-specific library is required to fully benefit from
# the C++ backend
import rethinkdb._pbcpp

# The google.protobuf package will activate the C++ backend only if this
# variable is set to 'cpp'
environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'cpp'

try:
    import google.protobuf

    # In protobuf 2.4.* and 2.5.0, this module imports correctly only if
    # the C++ backend is active in the google.protobuf package
    from google.protobuf.internal import cpp_message

except ImportError:
    del environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION']
    raise

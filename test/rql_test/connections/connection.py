###
# Tests the driver API for making connections and excercizes the networking code
###

from subprocess import Popen
from time import sleep
from sys import path
path.append("../../../drivers/python2")

import rethinkdb as r

# No servers started yet so this should fail
try:
    conn = r.connect()
    raise Exception("No connect error")
except r.RqlDriverError as err:
    if not str(err) == "Could not connect to localhost:28016.":
        raise Exception("Connect err is wrong")

try:
    conn = r.connect(port=11221)
    raise Exception("No connect error")
except r.RqlDriverError as err:
    if not str(err) == "Could not connect to localhost:11221.":
        print str(err)
        raise Exception("Connect err is wrong")

try:
    conn = r.connect(host="0.0.0.0")
    raise Exception("No connect error")
except r.RqlDriverError as err:
    if not str(err) == "Could not connect to 0.0.0.0:28016.":
        raise Exception("Connect err is wrong")

try:
    conn = r.connect(host="0.0.0.0", port=11221)
    raise Exception("No connect error")
except r.RqlDriverError as err:
    if not str(err) == "Could not connect to 0.0.0.0:11221.":
        raise Exception("Connect err is wrong")

# Run servers in default configuration
cpp_server = Popen(['../../../build/release_clang/rethinkdb'])
sleep(0.1)

try:
    conn = r.connect()
    conn.reconnect()
    conn = r.connect(host='localhost')
    conn.reconnect()
    conn = r.connect(host='localhost', port=28016)
    conn.reconnect()
    conn = r.connect(port=28016)
    conn.reconnect()
except r.RqlDriverError as err:
    raise Exception("Should have connected to default CPP server")

cpp_server.terminate()
sleep(0.1)

js_server =  Popen(['node', '../../../rqljs/build/rqljs'])
sleep(0.1)

try:
    conn = r.connect()
    conn.reconnect()
    conn = r.connect(host='localhost')
    conn.reconnect()
    conn = r.connect(host='localhost', port=28016)
    conn.reconnect()
    conn = r.connect(port=28016)
    conn.reconnect()
except r.RqlDriverError as err:
    raise Exception("Should have connected to default JS server")

try:
    c = r.connect()
    r.expr(1).run(c)
    c.close()
    c.close()
    c.reconnect()
    r.expr(1).run(c)
except r.RqlDriverError as err:
    raise Exception("Should not have thrown")

try:
    c = r.connect()
    r.expr(1).run(c)
    c.close()
    r.expr(1).run(c)
    raise Exception("Should have thrown")
except r.RqlDriverError as err:
    if not str(err) == "Connection is closed.":
        raise Exception("Error message wrong")

try:
    c = r.connect()
    r.expr(1).run(c)

    js_server.terminate()
    sleep(0.1)

    r.expr(1).run(c)
    raise Exception("Should have thrown")
except r.RqlDriverError as err:
    if not str(err) == "Connection is closed.":
        raise Exception("Error message wrong")

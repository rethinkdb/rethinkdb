#!/usr/bin/python
import random, time, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
from vcoptparse import *

def random_key(opts):
    # The reason we have keysuffix is in case another test (such as multi_serial_mix.py) is using
    # this as a subroutine but wants to make sure that random keys don't collide with other random
    # keys.
    suf = opts.get("keysuffix", "")
    return "".join(random.choice("abcdefghijklmnopqrstuvwxyz")
        for i in xrange(random.randint(1, opts["keysize"] - len(suf)))) + suf

def random_value(opts):
    # Most of the time we want to use small values, but we also want to test large values
    # sometimes.
    if random.randint(0, 10) == 0:
        return random.randint(0, opts["valuesize"]) * random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    else:
        return random.randint(0, min(200, opts["valuesize"])) * random.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ")

def fail(k,v,v2):
    raise ValueError("Key %r should have value %r, but had value %r." % (k, v, v2))

def verify_all(opts, mc, clone, deleted):
    for key in clone:
        value = mc.get(key)
        if value != clone[key]:
            fail(key, clone[key], value)
    for key in deleted:
        value = mc.get(key)
        if value is not None:
            fail(key, None, value)

def verify(opts, mc, clone, deleted, key):
    if not opts["thorough"]:
        # Check the specified key
        value = mc.get(key)
        if value != clone.get(key, None):
            fail(key, clone.get(key, None), value)
    else:
        # Check allllll the keys and deleted keys
        verify_all(opts, mc, clone, deleted)

def random_action(opts, mc, clone, deleted):

    what_to_do = random.random()

    if what_to_do < 0.2:
        # Check a random key
        if opts["thorough"]:
            # We check thoroughly after every test anyway
            return
        if not clone: return
        verify(opts, mc, clone, deleted, random.choice(clone.keys()))

    elif what_to_do < 0.25:
        # Check a deleted or nonexistent key
        if random.random() < 0.5 and deleted:
            # A deleted key
            key = random.choice(list(deleted))
        else:
            # A new key
            key = random_key(opts)
        verify(opts, mc, clone, deleted, key)

    elif what_to_do < 0.6:
        # Set
        if random.random() < 0.3 and clone:
            # An existing key
            key = random.choice(clone.keys())
        else:
            # A new key
            key = random_key(opts)
        deleted.discard(key)
        value = random_value(opts)
        clone[key] = value
        ok = mc.set(key, value)
        if ok == 0:
            raise ValueError("Could not set %r to %r." % (key, value))
        verify(opts, mc, clone, deleted, key)

    elif what_to_do < 0.95:
        # Append/prepend
        if not clone: return
        key = random.choice(clone.keys())
        # Make sure that the value we add isn't long enough to make the value larger than our
        # specified maximum value size
        value_to_pend = random_value(opts)[:opts["valuesize"] - len(clone[key])]
        if random.randint(1,2) == 1:
            # Append
            clone[key] += value_to_pend
            ok = mc.append(key, value_to_pend)
        else:
            # Prepend
            clone[key] = value_to_pend + clone[key]
            ok = mc.prepend(key, value_to_pend)
        if ok == 0:
            raise ValueError("Could not append/prepend %r to key %r" % (value_to_pend, key))
        verify(opts, mc, clone, deleted, key)

    else:
        # Delete
        if not clone: return
        key = random.choice(clone.keys())
        del clone[key]
        deleted.add(key)
        ok = mc.delete(key)
        if ok == 0:
            raise ValueError("Could not delete %r." % key)
        verify(opts, mc, clone, deleted, key)

def test(opts, mc, clone, deleted):
    if opts["duration"] == "forever":
        try:
            while True:
                random_action(opts, mc, clone, deleted)
        except KeyboardInterrupt:
            pass
    else:
        start_time = time.time()
        while time.time() < start_time + opts["duration"]:
            random_action(opts, mc, clone, deleted)

def option_parser_for_serial_mix():
    op = memcached_workload_common.option_parser_for_memcache()
    op["keysize"] = IntFlag("--keysize", 250)
    op["valuesize"] = IntFlag("--valuesize", 10000)
    op["thorough"] = BoolFlag("--thorough")
    def int_or_forever_parser(string):
        if string == "forever":
            return "forever"
        else:
            try:
                return int(string)
            except ValueError:
                raise OptError("expected 'forever' or integer, got %r" % string)
    op["duration"] = ValueFlag("--duration", converter = int_or_forever_parser, default = 10)
    return op

if __name__ == "__main__":
    import pickle

    op = option_parser_for_serial_mix()
    op["load"] = StringFlag("--load", None)
    op["save"] = StringFlag("--save", None)
    opts = op.parse(sys.argv)

    if opts["load"] is None:
        clone, deleted = {}, set()
    else:
        print "Loading from %r..." % opts["load"]
        with open(opts["load"]) as f:
            clone, deleted = pickle.load(f)
    with memcached_workload_common.make_memcache_connection(opts) as mc:
        test(opts, mc, clone, deleted)
    if opts["save"] is not None:
        print "Saving to %r..." % opts["save"]
        with open(opts["save"], "w") as f:
            pickle.dump((clone, deleted), f)

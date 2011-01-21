#!/usr/bin/python
import sys, os, time, random, re, threading, struct
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def mutate_pair_offset_zero(block):
    return block[0:28] + struct.pack('=H', 5000) + block[30:4096]

def mutate_pair_value(block):
    (npairs,) = struct.unpack('=H', block[24:26])

    poffs = [struct.unpack('=H', block[28+2*i:28+2*i+2])[0] for i in xrange(npairs)]
    poff = max(poffs)
    voff = poff + 1 + struct.unpack('=B', block[12 + poff])[0]
    # TODO: we could possibly be declaring the value's size to be 251,
    # which is.. okay for now.  extract should definitely fail, but
    # right now it's not failing for the reason that the value size is
    # invalid, it's just failing because the value size is too big.
    return block[:12+voff] + struct.pack('=B', 1 + struct.unpack('=B', block[12 + voff])[0]) + block[12 + voff + 1:]

def mutate_file(f, block_mutator):
    done = False
    last_leaf_blockid = None
    while not done:
        block = f.read(4096)
        if block == '':
            done = True
        else:
            if block[12:16] == 'leaf':
                last_leaf_blockid = block[0:4]

    if last_leaf_blockid == None:
        raise ValueError("the data file had no leaf nodes")

    f.seek(0, os.SEEK_SET)

    high_transaction_number = -1
    high_transaction_index = 0
    index = 0
    done = False
    while not done:
        block = f.read(4096)
        if block == '':
            done = True
        elif block[0:4] == last_leaf_blockid and (block[12:16] == 'inte' or block[12:16] == 'leaf'):
            trans_no = struct.unpack('=q', block[4:12])
            if trans_no > high_transaction_number:
                high_transaction_number = trans_no
                high_transaction_index = index
        index = index + 1

    if high_transaction_number == -1:
        raise RuntimeError("A block seems to have disappeared from the file!  This is very confusing.")

    f.seek(4096 * high_transaction_index, os.SEEK_SET)

    block = f.read(4096)
    if block[0:4] != last_leaf_blockid:
        raise RuntimeError("A block seems to have changed its block id!  This is very confusing.")

    if block[12:16] == 'inte':
        # We accidentally hit a leaf node that eventually becomes
        # an internal node.  This means the test broke, not
        # RethinkDB.  You could ignore this error, if it happens
        # rarely.  Maybe it's not worth fixing.
        raise RuntimeError("The block we are considering turned out to be an internal node (this is ok if it happens infrequently)")

    # set pair_offsets[0] to 5000
    writeblock = block_mutator(block)
    f.seek(-4096, os.SEEK_CUR)
    f.write(writeblock)

def block_based_mutator(block_mutator):
    def file_based_mutator(f):
        mutate_file(f, block_mutator)
    return file_based_mutator

def gen_random_skip():
    x = 0
    while x <= 0:
        ms = [30, 300, 3000, 30000]
        m = ms[random.randint(0, 3)]
        x = random.gauss(m, m / 3.0)
    return x


def garbage_data(length):
    # Some code that parses the results assumes that we don't have
    # crazy newlines.  That's why we omit newlines here.  (Also, there
    # is no reason to include them.)  It's still possible that we
    # might have newline trouble caused by incidental instances of the
    # value 10, though, which would result in incorrect underreportage
    # of keys.
    return ''.join([chr(random.randint(32,126)) for i in xrange(length)])

random_mutation_probability = 0.25

def random_mutator(f):
    f.seek(0, os.SEEK_END)
    filesize = f.tell()
    # Skip the static header
    position = 4096
    f.seek(position, os.SEEK_SET)
    done = False
    p = random_mutation_probability
    while not done:
        junk = random.randint(1, 2) == 1
        skip = int(round(gen_random_skip() * (1 if junk else 1.0/p - 1.0)))
        if position + skip > filesize:
            done = True
        elif junk:
            f.write(garbage_data(skip))
        else:
            f.seek(skip, os.SEEK_CUR)
        position = position + skip

def check_proportional_wrong(mutual_len, dump_len):
    # We'd naively expect 0.75 to be the threshold (with some
    # tolerance) since random_mutation_probability is 0.25, but there
    # are many ways an individual keyvalue can get corrupted.  We're
    # really just interested in seeing that we have _any_ keys at all
    # -- the real benefit of this test is when running under valgrind.
    if float(dump_len) / mutual_len < 0.1:
        raise ValueError("extraction dump has too few keys: %d/%d = %f", dump_len, mutual_len, float(dump_len) / mutual_len)

def check_one_wrong(mutual_len, dump_len):
    # mutation is expected to only remove one key from the playing
    # field.  Feel free to update this code when you create a mutation
    # that modifies more keys or wipes out an entire leaf node.
    if mutual_len - 1 < dump_len:
        raise ValueError("extraction dump has too many keys")
    elif mutual_len - 1 > dump_len:
        raise ValueError("extraction dump lacks keys")


def try_block_mutation(mutation_name, mutation, length_checker):
    print "Trying the '%s' block mutation..." % mutation_name

    shutil.copyfile(server1.data_files.files[0] + '.original', server1.data_files.files[0])

    # Corrupt the file, slightly.  This is a bit of a hack because we
    # merrily assume the last leaf node we see in the file never
    # becomes an internal node.  This is unlikely but not impossible.
    # So we might get some clearly labeled RuntimeErrors.
    with open(server1.data_files.files[0], 'r+b') as f:
        mutation(f)

    # Maybe we shouldn't modify the original file in place and instead write a copy.

    # We could double-check here that fsck gets an error.  But we'll get a failure later when we find that we've received every key we've entered.

    # Run rethinkdb-extract

    extract_path = get_executable_path(opts, "rethinkdb")
    dump_path = test_dir.p("db_data_dump." + mutation_name + ".txt")
    run_executable(
        [extract_path, "extract", "-o", dump_path, "--force-slice-count", str(opts['slices'])] + server1.data_files.rethinkdb_flags(),
        "extractor_output.txt",
        valgrind_tool = opts["valgrind-tool"] if opts["valgrind"] else None,
        test_dir = test_dir
        timeout = 240
        )

    # Make sure extracted info is correct

    dumpfile = open(dump_path, "r")
    dumplines = dumpfile.readlines()

    invalids = 0

    equiv_dict = {}
    for i in xrange(0, len(dumplines) / 2):
        m = re.match(r"set (\d+) 0 0 (\d+) noreply\r\n", dumplines[2 * i])
        if m == None:
            invalids = invalids + 1
            continue
        (key, length) = m.groups()

        next_line = dumplines[2 * i + 1]
        value = next_line[0:int(length)]
        expected_crlf = next_line[int(length):]

        if value != mutual_dict.get(key):
            invalids = invalids + 1
            print "Invalid value for key '%s', has length %d, should have length %d" % (key, len(value), len(mutual_dict[key]))
        if expected_crlf != "\r\n":
            raise ValueError("Lacking CRLF suffix for value of key '%s'" % key)

    length_checker(len(mutual_dict), len(dumplines) / 2 - invalids)

    print "The '%s' block mutation succeeded." % mutation_name






if __name__ == "__main__":
    op = make_option_parser()
    op["mclib"].default = "memcache"   # memcache plays nicer with this test than pylibmc does
    opts = op.parse(sys.argv)
    
    test_dir = TestDir()
    mutual_dict = {}
    
    # Start server
    
    opts_without_valgrind = dict(opts)
    opts_without_valgrind["valgrind"] = False
    server1 = Server(opts_without_valgrind, name="first server", extra_flags = ["--wait-for-flush", "y", "--flush-timer", "0"], test_dir=test_dir)
    server1.start()

    # Put some values in server

    mc = connect_to_port(opts, server1.port)
    for i in xrange(0, 1000):
        key = str(i)
        value = str(str(random.randint(0, 9)) * random.randint(0, 10000))
        ok = mc.set(key, value)
        if ok == 0:
            raise ValueError("Could not insert %r" % value)
        mutual_dict[key] = value
    mc.disconnect_all()
    
    # Kill server
    
    server1.kill()

    shutil.copyfile(server1.data_files.files[0], server1.data_files.files[0] + '.original')

    try_block_mutation('mutate_pair_offset_zero', block_based_mutator(mutate_pair_offset_zero), check_one_wrong)
    try_block_mutation('mutate_pair_value', block_based_mutator(mutate_pair_value), check_one_wrong)
    try_block_mutation('random_mutator', random_mutator, check_proportional_wrong)

    print "Extraction test succeeded."





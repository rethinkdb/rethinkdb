#!/usr/bin/python
import os, sys, socket, random, time, struct, traceback
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def start_master(opts, extra_flags, test_dir):
    repli_port = find_unused_port()
    master = Server(opts, extra_flags=extra_flags + ["--master", "%d" % repli_port], name="master", test_dir=test_dir)
    master.start(False)
    master.master_port = repli_port
#    wait_till_socket_connectible('localhost', repli_port)
    return master

def start_slave(repli_port, opts, extra_flags, test_dir):
    slave = Server(opts, extra_flags=extra_flags + ["--slave", "%s:%d" % ('localhost', repli_port)], name="slave", test_dir=test_dir)
    slave.start()
    return slave

class PacketSplitter(object):
    def __init__(self, name, hello_size):
        self.name = name
        self.buffer = ""
        self.current_msg_size = hello_size
        self.packet_count = 0

    def process(self, recv_buf):
        if len(recv_buf) == 0:
            return ""
        self.buffer = self.buffer + recv_buf
        result = []
        p = 0
        while len(self.buffer)-p >= 2:
            if self.current_msg_size == 0:
                self.current_msg_size = struct.unpack('<H', self.buffer[p:p+2])[0]
            if self.current_msg_size == 0:
                break
            if p+self.current_msg_size <= len(self.buffer):
                #print "%s msg size: %d" % (self.name, self.current_msg_size)
                result.append(self.buffer[p:p+self.current_msg_size])
                p = p + self.current_msg_size
                self.current_msg_size = 0
                self.packet_count = self.packet_count + 1
                if self.packet_count % 1000 == 0:
                    print "%d packets proxied" % self.packet_count
            else:
                break
        self.buffer = self.buffer[p:]
        return result

class PacketCorrupter(object):
    def __init__(self, name, corruption_p = 1e-5):
        self.name = name
        self.corruption_p = corruption_p
        self.corruption_types = [
            (self.corrupt_length, 2),
            (self.corrupt_multipart_aspect, 2),
            (self.corrupt_header, 2),
            (self.lose_packet, 1)
        ]
        self.stream_ids = []
        self.stream_unique_ids = {}
        self.streams_max_unique = 64

    def repack_packet(self, packet_info):
        return ''.join(
            [struct.pack('<H', packet_info['sz']), struct.pack('<B', packet_info['multipart_aspect'])] +
            ([struct.pack('<I', packet_info['stream_id'])] if packet_info['stream_id'] != None else []) +
            [packet_info['header'], packet_info['value']]
        )

    def corrupt_length(self, packet, packet_info):
        old_sz = packet_info['sz']
        packet_info['sz'] = self.choose([
            ((lambda sz: sz-random.randrange(sz/2)), 8),                        # decrease size
            ((lambda sz: sz+random.randrange(sz/3)), 4),                        # increase size
            ((lambda sz: random.randrange(packet_info['header_offset'])), 1),   # check how invalid packet sizes are handled
            ((lambda sz: -random.randrange(sz)), 1)                             # check how negative/big packet sizes are handled
        ], old_sz)
        print "%s: Corrupting packet length: %d -> %d" % (self.name, old_sz, packet_info['sz'])
        return self.repack_packet(packet_info)

    def corrupt_multipart_aspect(self, packet, packet_info):
        multipart_aspect = packet_info['multipart_aspect']
        other_multipart_aspects = filter(lambda x: x != multipart_aspect, [0x81, 0x82, 0x83, 0x84])
        stream_id = packet_info['stream_id']

        def gen_stream_id(s):
            if s is None:
                s = self.get_valid_stream_id()
                if s is None:
                    s = random.randint(0, 0xffffffff)
            return s

        (packet_info['multipart_aspect'], packet_info['stream_id']) = self.choose([
            (lambda m, s: (random.choice(other_multipart_aspects), gen_stream_id(s)), 8),   # use a valid multipart aspect
            (lambda m, s: (random.randint(0, 0xff), s), 1),                                 # use a random multipart aspect
            (lambda m, s: (m, self.get_valid_stream_id()),
                1 if stream_id is not None else 0),                                         # use a (recently) valid stream_id
            (lambda m, s: (m, random.randint(0, 0xffffffff)),
                1 if stream_id is not None else 0)                                          # use a random stream_id
        ], multipart_aspect, stream_id)

        print "%s: Corrupting (multipart_aspect, stream_id): (0x%02x, %s) -> (0x%02x, %s)" % (
            self.name,
            multipart_aspect, ("0x%08x" % stream_id) if stream_id is not None else "",
            packet_info['multipart_aspect'], ("0x%08x" % packet_info['stream_id']) if packet_info['stream_id'] is not None else ""
        )
        return self.repack_packet(packet_info)

    def corrupt_header(self, packet, packet_info):
        header = packet_info['header']
        change_pos = random.randrange(len(header)) 
        packet_info['header'] = header[0:change_pos] + chr(random.randint(0,0xff)) + header[change_pos+1:]

        print "%s: Corrupting header: %r -> %r" % (self.name, header, packet_info['header'])
        return self.repack_packet(packet_info)

    def lose_packet(self, packet, packet_info):
        print "%s: Losing packet: (sz=%d, multipart_aspect=0x%02x, stream_id=%s, header=%r)" % (
            self.name,
            packet_info['sz'],
            packet_info['multipart_aspect'],
            ("0x%08x" % packet_info['stream_id']) if packet_info['stream_id'] is not None else "",
            packet_info['header']
        )

        return None

    def add_stream_id(self, stream_id):
        if not stream_id in self.stream_unique_ids:
            n = len(self.stream_ids)
            if n < self.streams_max_unique:
                stream_ids.append(stream_id)
            else:
                pos = random.randrange(n)
                del self.stream_unique_ids[self.stream_ids[pos]]
                stream_ids[pos] = stream_id
            self.stream_unique_ids[stream_id] = None

    def get_valid_stream_id(self):
        n = len(self.stream_ids)
        if n > 0:
            return self.stream_ids[random.randrange(n)]
        else:
            return None

    def choose(self, choices, *args):
        total = sum([p for (k,p) in choices])
        r = random.random()*total
        a = 0
        for (f,p) in choices:
            a += p
            if a >= r:
                return f(*args)
        return None

    # Packet format:
    #   size: 2 bytes
    #   multipart_aspect: 1 byte
    #       Valid values: 0x81 (small packet), 0x82-0x84 for first/middle/last packet
    #   stream_id: 4 bytes (if multipart_aspect != 0x81), otherwise not present
    #   "header": at most 20 bytes (arbitrary data which we corrupt more often then the data that follows)
    #   "value": all the rest
    def process(self, packet):
        if len(packet) >= 3:
            multipart_aspect = struct.unpack('<B', packet[2])[0]
            assert multipart_aspect >= 0x81 and multipart_aspect <= 0x84, "Error: multipart_aspect has unknown value 0x%02x. Probably packet structure has changed and someone forgot to update the test." % multipart_aspect
            stream_id_present = multipart_aspect != 0x81 # 0x81 is "small packet"
            header_offset = 7 if stream_id_present else 3
            assert len(packet) >= header_offset
            stream_id = struct.unpack("<I", packet[3:7])[0] if stream_id_present else None

            if random.random() <= self.corruption_p:
                sz = struct.unpack('<H', packet[0:2])[0]
                value_offset = min(len(packet), header_offset+20)

                packet_info = {
                    'sz': sz,
                    'multipart_aspect': multipart_aspect,
                    'stream_id': stream_id,
                    'header': packet[header_offset:value_offset],
                    'value': packet[value_offset:],
                    'header_offset': header_offset,
                    'value_offset': value_offset
                }

                packet = self.choose(self.corruption_types, packet, packet_info)

            if stream_id_present:
                self.add_stream_id(stream_id)
            return packet

def start_evil_monkey(master_repli_port, master_to_slave_corruption_p, slave_to_master_corruption_p, dont_corrupt_first, test_dir):
    class EvilMonkey(threading.Thread):
        def __init__(self, master_repli_port, master_to_slave_corruption_p, slave_to_master_corruption_p, dont_corrupt_first):
            threading.Thread.__init__(self)
            self.master_to_slave_corruption_p = master_to_slave_corruption_p
            self.slave_to_master_corruption_p = slave_to_master_corruption_p
            self.dont_corrupt_first = dont_corrupt_first
            self.repli_port = find_unused_port()
            self.master_socket = None
            self.proxied_socket = socket.socket()
            self.proxied_socket.bind(('localhost', self.repli_port))
            self.proxied_socket.listen(1)
            self.shutting_down = False
            self.master_to_slave = None
            self.slave_to_master = None
            self.exception = None

        def connect(self):
            print "Trying to connect to master replication port"
            connect_failures = 0
            while not self.shutting_down:
                try:
                    master_socket = socket.create_connection(('localhost', master_repli_port))
                    self.proxied_socket.settimeout(1)
                    (connected_proxied_socket, address) = self.proxied_socket.accept()
                    print "master<->slave connection established"
                    return (master_socket, connected_proxied_socket)
                except Exception, e:
                    print "connect: %s" % e
                    connect_failures = connect_failures + 1
                    if connect_failures > 16:
                        self.shutting_down = True
                        raise RuntimeError("Failed to connect to master replication port: probably it doesn't want us to connect")
            return (None, None)


        def run(self):
            connected_proxied_socket = None
            try:
                while not self.shutting_down:
                    (self.master_socket, connected_proxied_socket) = self.connect()
                    if self.shutting_down:
                        break

                    self.master_to_slave = threading.Thread(target=self.run_pump, args=(self.master_socket, connected_proxied_socket, self.master_to_slave_corruption_p, self.dont_corrupt_first, 32, 'master->slave'))
                    self.slave_to_master = threading.Thread(target=self.run_pump, args=(connected_proxied_socket, self.master_socket, self.slave_to_master_corruption_p, self.dont_corrupt_first, 28, 'slave->master'))
                    self.master_to_slave.start()
                    self.slave_to_master.start()
                    self.master_to_slave.join()
                    self.master_to_slave = None
                    self.slave_to_master.join()
                    self.slave_to_master = None
            except Exception, e:
                self.exception = e
            finally:
                self.master_socket.close()
                if connected_proxied_socket:
                    connected_proxied_socket.close()

        def run_pump(self, from_socket, to_socket, corruption_p, dont_corrupt_first, hello_size, name):
            #print "Pump %s started" % name
            proto = PacketSplitter(name, hello_size)
            packetCorrupter = PacketCorrupter(name, corruption_p)
            dont_corrupt_packets = dont_corrupt_first
            try:
                while not self.shutting_down:
                    from_socket.settimeout(1)
                    try:
                        recv_buf = from_socket.recv(4096)
                        packets = proto.process(recv_buf)
                        for packet in packets:
                            if dont_corrupt_packets > 0:
                                dont_corrupt_packets = dont_corrupt_packets-1
                            else:
                                packet = packetCorrupter.process(packet)
                                #packetCorrupter.process(packet)

                            if packet:
                                to_socket.sendall(packet)
                    except socket.timeout:
                        pass
            except socket.error, e:
                print "%s: Got a socket error: %r" % (name, e)
                if to_socket:
                    to_socket.close()
                if from_socket:
                    from_socket.close()
            except Exception, e:
                exc_type, exc_value, exc_traceback = sys.exc_info()
                traceback.print_exception(exc_type, exc_value, exc_traceback, file=sys.stderr)

        def shutdown(self):
            if not self.shutting_down:
                self.shutting_down = True
                self.join()
    
    evil_monkey = EvilMonkey(master_repli_port, master_to_slave_corruption_p, slave_to_master_corruption_p, dont_corrupt_first)
    evil_monkey.start()
    return evil_monkey

# Necessary full-test tests:
# - high corruption probability (which causes backfill messages to get corrupted), no stress-client, --dont-corrupt-first=3
#   slave starting does not finish until the slave starts to accept gets, which happens at the end of the backfill.
#   Test succeedes when the slave starts accepting gets.
# - normal corruption probability, no corruption of (say) 100 first packets, stress-client is started, --dont-corrupt-first=30
#   Test checks that packet corruption does not cause master/slave segfault.
def test_function(opts, extra_flags=[], test_dir=TestDir()):
    master = None
    slave = None
    evil_monkey = None
    stresser = None
    stresser_waiter_thread = None
    try:
        master = start_master(opts, extra_flags, test_dir=test_dir)
        evil_monkey = start_evil_monkey(master.master_port, opts['master_to_slave_corruption_p'], opts['slave_to_master_corruption_p'], opts['dont_corrupt_first'], test_dir=test_dir)
        slave = start_slave(evil_monkey.repli_port, opts, extra_flags, test_dir=test_dir)

        if opts['stress_client']:
            time.sleep(10)
            stresser = stress_client(port=master.port, workload=
                                          { "deletes":  opts["ndeletes"],
                                            "updates":  opts["nupdates"],
                                            "inserts":  opts["ninserts"],
                                            "gets":     opts["nreads"],
                                            "appends":  opts["nappends"],
                                            "prepends": opts["nprepends"],
                                            "rgets": opts["nrgets"] },
                                          duration="%ds" % opts["duration"], extra_flags = ['-v', '1-1025'], test_dir=test_dir, run_in_background=True)
            def stresser_waiter():
                try:
                    stresser.wait(opts["duration"]+100)
                except RuntimeError, e:
                    print e

            stresser_waiter_thread = threading.Thread(target=stresser_waiter, args=())
            stresser_waiter_thread.start()
            while evil_monkey.is_alive() and stresser_waiter_thread.is_alive():
                time.sleep(1)
    except Exception, e:
        print >> sys.stderr, "Got an exception in test: %s" % e
        raise
    finally:
        if evil_monkey is not None and evil_monkey.is_alive():
            print "Shutting down Evil Monkey"
            evil_monkey.shutdown()
        if evil_monkey is not None and evil_monkey.exception is not None:
            print >> sys.stderr, "Evil Monkey Error: %s" % evil_monkey.exception
        if stresser_waiter_thread is not None and stresser_waiter_thread.is_alive():
            print "Shutting down stress client"
            try:
                stresser.interrupt()
                stresser.wait(5)
            except RuntimeError, e:
                pass
            stresser = None

        if slave:
            try:
                slave.shutdown()
            except Exception, e:
                print e
        if master:
            try:
                master.shutdown()
            except Exception, e:
                print e

    if evil_monkey is not None and evil_monkey.exception is not None:
        raise evil_monkey.exception

if __name__ == "__main__":
    op = make_option_parser()
    op["master_to_slave_corruption_p"]  = FloatFlag("--corrupt-from-master-prob",  1e-4)
    op["slave_to_master_corruption_p"]  = FloatFlag("--corrupt-from-slave-prob",  1e-4)
    op["dont_corrupt_first"]  = IntFlag("--dont-corrupt-first",  30)
    op["stress_client"]  = BoolFlag("--no-stress", invert=True)
    op["ndeletes"]  = IntFlag("--ndeletes",  1)
    op["nupdates"]  = IntFlag("--nupdates",  4)
    op["ninserts"]  = IntFlag("--ninserts",  8)
    op["nreads"]    = IntFlag("--nreads",    32)
    op["nappends"]  = IntFlag("--nappends",  0)
    op["nprepends"] = IntFlag("--nprepends", 0)
    op["nrgets"] = IntFlag("--nrgets", 0)
    opts = op.parse(sys.argv)
    opts["netrecord"] = False   # We don't want to slow down the network
    test_function(opts)


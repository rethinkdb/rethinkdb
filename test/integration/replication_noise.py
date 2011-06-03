#!/usr/bin/python
import os, sys, socket, random, time, struct, traceback, math
from datetime import datetime
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
    slave = Server(opts, extra_flags=extra_flags + ["--slave", "%s:%d" % ('localhost', repli_port), "--no-rogue"], name="slave", test_dir=test_dir)
    slave.start()
    return slave

class PacketSplitter(object):
    def __init__(self, name):
        self.name = name
        self.buffer = ""
        self.hello_packet = '13rethinkdbrepl\x00\x01\x00\x00\x00'
        self.current_msg_size = len(self.hello_packet)
        self.packet_count = 0

        global corruptions, disconnects
        assert corruptions >= disconnects, "We get disconnected more often (%d times) than we corrupt the packets (%d times), probably the server doesn't want to accept our connection" % (disconnects, corruptions)

    def process(self, recv_buf):
        if len(recv_buf) == 0:
            return []
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
                msg = self.buffer[p:p+self.current_msg_size]

                if self.hello_packet != None:
                    assert self.hello_packet == msg, "Unexpected hello packet, probably someone has changed the protocol but forgot to update the test!"
                    self.hello_packet = None

                result.append(msg)
                p = p + self.current_msg_size
                self.current_msg_size = 0
                self.packet_count = self.packet_count + 1
                if self.packet_count % 1000 == 0:
                    print "%d packets proxied" % self.packet_count
            else:
                break
        self.buffer = self.buffer[p:]
        return result

corruptions = 0
disconnects = 0

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
            assert multipart_aspect >= 0x81 and multipart_aspect <= 0x84, "Error: multipart_aspect has unknown value 0x%02x. Probably packet structure has changed and someone forgot to update the test. Packet contents: %r" % (multipart_aspect, packet)
            stream_id_present = multipart_aspect != 0x81 # 0x81 is "small packet"
            header_offset = 7 if stream_id_present else 3
            assert len(packet) >= header_offset
            stream_id = struct.unpack("<I", packet[3:7])[0] if stream_id_present else None

            if random.random() <= self.corruption_p:
                global corruptions
                corruptions = corruptions + 1
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

class TCPProxy(object):
    # listening_to_connecting_filter and connecting_to_listening_filter must have the following methods:
    #   connected()
    #   process(packet) -> [packets]
    #   disconnected()
    def __init__(self, listening_port, connecting_port, max_connect_failures, listening_to_connecting_filter, connecting_to_listening_filter, listening_to_connecting_name, connecting_to_listening_name):
        self.connecting_port = connecting_port
        self.listening_port = listening_port
        self.max_connect_failures = max_connect_failures
        self.listening_to_connecting_filter = listening_to_connecting_filter
        self.connecting_to_listening_filter = connecting_to_listening_filter
        self.listening_to_connecting_name = listening_to_connecting_name
        self.connecting_to_listening_name = connecting_to_listening_name
        self.shutting_down = False
        self.exception = None

        self.listener = socket.socket()
        self.listener.bind(('localhost', self.listening_port))
        self.listener.listen(1)

    def accept_and_then_connect(self):
        connect_failures = 0
        listening_side = None
        connecting_side = None
        success = False
        try:
            while not self.shutting_down and listening_side == None:
                self.listener.settimeout(1)
                try:
                    (listening_side, address) = self.listener.accept()
                    print "Accepted connect on %d" % self.listening_port
                except socket.timeout:
                    pass
                except Exception, e:
                    print "accept: %s" % e
            while not self.shutting_down:
                try:
                    print "About to connect, time is " + datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                    connecting_side = socket.create_connection(('localhost', self.connecting_port))
                    print "Connected to %d" % self.connecting_port
                    success = True
                    return (listening_side, connecting_side)
                except Exception, e:
                    print "connect: %s" % e
                    connect_failures = connect_failures + 1
                    if connect_failures > self.max_connect_failures:
                        self.shutting_down = True
                        raise RuntimeError("Failed to connect to master replication port: probably it doesn't want us to connect")
            return (None, None)
        finally:
            if not success:
                if listening_side != None:
                    listening_side.close()
                if connecting_side != None:
                    connecting_side.close()

    def run(self):
        def run_pump(from_socket, to_socket, filter, name):
            action = None
            try:
                while not self.shutting_down:
                    action = "settimeout"
                    from_socket.settimeout(1)
                    action = None
                    try:
                        action = "receiving"
                        recv_buf = from_socket.recv(4096)
                        action = None
                        if len(recv_buf) > 0:
                            packets = filter(recv_buf)
                            if packets != None:
                                packet = ''.join([p for p in packets if p != None])
                                if len(packet) > 0:
                                    #print "%s: sending %d" % (name, len(packet))
                                    action = "sending"
                                    to_socket.sendall(packet)
                                    action = None
                    except socket.timeout:
                        pass
            except socket.error, e:
                print "%s: Got a socket error (by %s): %r" % (name, action, e)
                global disconnects
                disconnects = disconnects + 1
                if to_socket:
                    to_socket.close()
                if from_socket:
                    from_socket.close()
            except Exception, e:
                #exc_type, exc_value, exc_traceback = sys.exc_info()
                #traceback.print_exception(exc_type, exc_value, exc_traceback, file=sys.stderr)
                print "Exception in run_pump(%s): %s" % (name, e)
                self.exception = e
                self.shutdown()

        (listening_side, connecting_side) = (None, None)
        try:
            while not self.shutting_down:
                (listening_side, connecting_side) = self.accept_and_then_connect()
                if not self.shutting_down:
                    assert listening_side != None and connecting_side != None

                    try:
                        self.listening_to_connecting_filter.connect()
                        self.connecting_to_listening_filter.connect()
                        master_to_slave = threading.Thread(target=run_pump, args=(connecting_side, listening_side, self.connecting_to_listening_filter.process, self.connecting_to_listening_name))
                        slave_to_master = threading.Thread(target=run_pump, args=(listening_side, connecting_side, self.listening_to_connecting_filter.process, self.listening_to_connecting_name))
                        master_to_slave.start()
                        slave_to_master.start()
                        master_to_slave.join()
                        slave_to_master.join()
                    finally:
                        self.listening_to_connecting_filter.disconnect()
                        self.connecting_to_listening_filter.disconnect()

                        if listening_side != None:
                            print datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f") + " TCPProxy: closing listening socket"
                            listening_side.close()
                        if connecting_side != None:
                            print datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f") + " TCPProxy: closing connecting socket"
                            connecting_side.close()
        except Exception, e:
            print "Exception in proxy: %s" % e
            self.exception = e

    def shutdown(self):
        self.shutting_down = True

def start_evil_monkey(master_repli_port, master_to_slave_corruption_p, slave_to_master_corruption_p, dont_corrupt_first, test_dir):
    class EvilMonkey(threading.Thread):
        def __init__(self, master_repli_port, master_to_slave_corruption_p, slave_to_master_corruption_p, dont_corrupt_first):
            threading.Thread.__init__(self)
            self.master_to_slave_corruption_p = master_to_slave_corruption_p
            self.slave_to_master_corruption_p = slave_to_master_corruption_p
            self.dont_corrupt_first = dont_corrupt_first
            self.repli_port = find_unused_port()
            self.exception = None
            self.max_connect_failures = 60

        def run(self):
            class Processor(object):
                def __init__(self, corruption_p, dont_corrupt_packets, name):
                    self.corruption_p = corruption_p
                    self.dont_corrupt_packets = dont_corrupt_packets
                    self.name = name
                    self.reset()

                def reset(self):
                    self.dont_corrupt_count = self.dont_corrupt_packets
                    self.proto = PacketSplitter(self.name)
                    self.corrupter = PacketCorrupter(self.name, self.corruption_p)

                def process(self, buffer):
                    # print "%s: recv: %d" % (self.name, len(buffer))
                    packets = self.proto.process(buffer)
                    result = []
                    for packet in packets:
                        #print "%s: msg: %d %r" % (self.name, len(packet), packet)
                        if self.dont_corrupt_count > 0:
                            self.dont_corrupt_count = self.dont_corrupt_count-1
                        else:
                            packet = self.corrupter.process(packet)
                        result.append(packet)
                    return result
                def connect(self):
                    self.reset()
                def disconnect(self):
                    pass

            slave_to_master_name = 'slave->master'
            master_to_slave_name = 'master->slave'
            master_to_slave_processor = Processor(self.master_to_slave_corruption_p, self.dont_corrupt_first, master_to_slave_name)
            slave_to_master_processor = Processor(self.slave_to_master_corruption_p, self.dont_corrupt_first, slave_to_master_name)
            self.proxy = TCPProxy(self.repli_port, master_repli_port, self.max_connect_failures, slave_to_master_processor, master_to_slave_processor, slave_to_master_name, master_to_slave_name)
            try:
                self.proxy.run()
            finally:
                self.exception = self.proxy.exception

        def shutdown(self):
            self.proxy.shutdown()
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
        print "Starting master"
        master = start_master(opts, extra_flags, test_dir=test_dir)
        print "Starting Evil Monkey"
        evil_monkey = start_evil_monkey(master.master_port, opts['master_to_slave_corruption_p'], opts['slave_to_master_corruption_p'], opts['dont_corrupt_first'], test_dir=test_dir)
        print "Starting slave"
        slave = start_slave(evil_monkey.repli_port, opts, extra_flags, test_dir=test_dir)

        if opts['stress_client']:
            #time.sleep(10)
            print "Starting stress client"
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



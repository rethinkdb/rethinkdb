from vcoptparse import *
import socket

def option_parser_for_socket():
	op = OptParser()
	op["phase-count"] = BoolFlag("--phase-count")
	op["phase"] = IntFlag("--phase", -1)
	op["port"] = IntFlag("--port")
	op["host"] = StringFlag("--host", "localhost")
	return op

def option_parser_for_memcache():
	op = option_parser_for_socket()
	op["mclib"] = ChoiceFlag("--mclib", ["memcache", "pylibmc"], "pylibmc")
	op["protocol"] = ChoiceFlag("--protocol", ["text", "binary"], "text")
	return op

def make_socket_connection(opts):
	return SocketConnection(opts["host"], opts["port"])

class SocketConnection(object):
	def __init__(self, host, port):
		self.host = host
		self.port = port
	def __enter__(self):
		self.sock = socket.socket()
		self.sock.connect((self.host, self.port))
		return self.sock
	def __exit__(self, val, type, tb):
		self.sock.close()

def make_memcache_connection(opts):
	# If the phase count was requested, no tests should be run
	assert not opts["phase-count"]
	return MemcacheConnection(opts["host"], opts["port"], opts["mclib"], opts["protocol"])

class MemcacheConnection(object):
	def __init__(self, host, port, mclib="pylibmc", protocol="text"):
		assert mclib in ["pylibmc", "memcache"]
		assert protocol in ["text", "binary"]
		self.host = host
		self.port = port
		self.mclib = mclib
		self.protocol = protocol
	def __enter__(self):
		if self.mclib == "pylibmc":
			import pylibmc
			self.mc = pylibmc.Client(["%s:%d" % (self.host, self.port)], binary = (self.protocol == "binary"))

			# libmemcached is annoying: they changed the "poll timeout" behavior name in some version, so we have to do the following:
			for poll_timeout_behavior in ["poll timeout", "_poll_timeout"]:
				if poll_timeout_behavior in self.mc.behaviors:
					self.mc.behaviors[poll_timeout_behavior] = 10 # Tim: "Seconds (I think)"
					break
		else:
			assert self.protocol == "text"   # python-memcache does not support the binary protocol
			import rethinkdb_memcache as memcache
			self.mc = memcache.Client(["%s:%d" % (self.host, self.port)])

		return self.mc
	def __exit__(self, val, type, tb):
		self.mc.disconnect_all()

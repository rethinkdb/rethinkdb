import subprocess, os, time, string, signal

class WorkloadRunner(object):
	def __init__(self, command_line):
		# Check for the executable
		# TODO: this won't work with complex command-lines
		executable = string.split(command_line)[0]
		if not os.access(executable, os.X_OK):
			raise RuntimeError("specified executable '%s' not found or permissions not set" % (executable))
		self.command_line = string.split(command_line)
	
	def run(self, host, port, timeout):
		start_time = time.time()
		end_time = start_time + timeout
		print "Running " + string.join(self.command_line)

		# Set up environment
		new_environ = os.environ.copy()
		new_environ["HOST"] = host
		new_environ["PORT"] = str(port)

		proc = subprocess.Popen(self.command_line, env = new_environ)

		while time.time() < end_time:
			result = self.process.poll()
			if result is None:
				time.sleep(1)
			elif result == 0:
				print "Done (%d seconds)" % (time.time() - self.start_time)
				self.process = None
				return
			else:
				print "Failed (%d seconds)" % (time.time() - self.start_time)
				raise RuntimeError("workload '%s' failed in phase %d with error code %d" % (self.command_line, self.phase, result))
		print "Timed out (%d seconds)" % (time.time() - self.start_time)
		self.process.send_signal(signal.SIGINT) # TODO: SIGKILL if stubborn
		raise RuntimeError("workload timed out before completion")


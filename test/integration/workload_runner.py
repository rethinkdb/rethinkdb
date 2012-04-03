import subprocess, os, time, string, signal

class InfiniteWorkloadRunner(object):
	def __init__(self, command_line):
		self.command_line = string.split(command_line)
		self.process = None

	def run(self, timeout):
		# run the test in a loop until timeout has occurred, then return
		endTime = time.time() + timeout
		self.process = subprocess.Popen(self.command_line)

		while time.time() < endTime:
			time.sleep(1)
			result = self.process.poll()
			if result is None:
				continue
			elif result == 0:
				self.process = subprocess.Popen(self.command_line)
			else:
				raise RuntimeError("workload failed with error code %d" % (result))

	def pause(self):
		assert self.process is not None
		self.process.send_signal(signal.SIGSTOP)
		# TODO: race condition if process died already?

	def resume(self):
		assert self.process is not None
		self.process.send_signal(signal.SIGCONT)
	
	def finish(self):
		# finish the current run of the test
		assert self.process is not None
		result = self.process.wait()
		if result != 0:
			raise RuntimeError("workload failed with error code %d" % (result))

class PhasedWorkloadRunner(object):
	def __init__(self, command_line, phaseCount):
		self.command_line = string.split(command_line) + ["--phase"]
		self.process = None
		self.phase = 0
		self.maxPhase = phaseCount - 1
	
	def _launch_next_phase(self):
		assert self.process is None
		print "Running " + string.join(self.command_line) + str(self.phase)
		self.process = subprocess.Popen(self.command_line + [str(self.phase)])
		self.phase += 1
	
	def _check_last_phase(self):
		assert self.process is not None
		result = self.process.wait()
		if result != 0:
			raise RuntimeError("workload '%s' failed in phase %d with error code %d" % (self.command_line, self.phase, result))
		self.process = None

	def run(self, timeout):
		assert self.phase == 0
		self._launch_next_phase()
		# timeout is ignored, since we only return at a phase break
	
	def pause(self):
		assert self.phase < self.maxPhase # if equal, we are on the last phase, pause will be after the test
		self._check_last_phase()

	def resume(self):
		assert self.phase < self.maxPhase
		self._launch_next_phase()

	def finish(self):
		assert self.phase != 0
		self._check_last_phase()
		while self.phase <= self.maxPhase:
			self._launch_next_phase()
			self._check_last_phase()

def create_workload(command_line):
	# Check for the executable
	executable = string.split(command_line)[0]
	if not os.access(executable, os.X_OK):
		raise RuntimeError("specified executable '%s' not found or permissions not set" % (executable))

	# Get the number of phases
	phaseCount = subprocess.call(string.split(command_line) + ["--phase-count"])
	if phaseCount == 0:
		return InfiniteWorkloadRunner(command_line)
	elif phaseCount > 0:
		return PhasedWorkloadRunner(command_line, phaseCount)
	else:
		raise RuntimeError("error when determining phase count of executable '%s': %d" % (executable, phaseCount))


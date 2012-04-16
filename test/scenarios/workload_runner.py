import subprocess, os, time, string, signal

def run(command_line, host, port, timeout):
	start_time = time.time()
	end_time = start_time + timeout
	print "Running %r..." % command_line

	# Set up environment
	new_environ = os.environ.copy()
	new_environ["HOST"] = host
	new_environ["PORT"] = str(port)

	proc = subprocess.Popen(command_line, shell = True, env = new_environ, preexec_fn = os.setsid)

	while time.time() < end_time:
		result = proc.poll()
		if result is None:
			time.sleep(1)
		elif result == 0:
			print "Done (%d seconds)" % (time.time() - start_time)
			return
		else:
			print "Failed (%d seconds)" % (time.time() - start_time)
			raise RuntimeError("workload '%s' failed with error code %d" % (command_line, result))
	print "Timed out (%d seconds)" % (time.time() - start_time)
	try:
            proc.send_signal(signal.SIGKILL)
        except OSError:
            pass
	raise RuntimeError("workload timed out before completion")


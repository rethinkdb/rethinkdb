import subprocess, os, time, string, signal

def run(command_line, host, port, timeout):
    start_time = time.time()
    end_time = start_time + timeout
    print "Running %r..." % command_line

    # Set up environment
    new_environ = os.environ.copy()
    new_environ["HOST"] = host
    new_environ["PORT"] = str(port)

    proc = subprocess.Popen(command_line, shell = True, env = new_environ, preexec_fn = lambda: os.setpgid(0, 0))

    try:
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
    finally:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except OSError:
            pass
    raise RuntimeError("workload timed out before completion")

class ContinuousWorkload(object):
    def __init__(self, command_line, host, port):
        self.command_line = command_line
        self.host = host
        self.port = port
        self.running = False

    def __enter__(self):
        return self

    def start(self):
        assert not self.running
        print "Starting %r..." % self.command_line

        # Set up environment
        new_environ = os.environ.copy()
        new_environ["HOST"] = self.host
        new_environ["PORT"] = str(self.port)

        self.proc = subprocess.Popen(self.command_line, shell = True, env = new_environ, preexec_fn = lambda: os.setpgid(0, 0))

        self.running = True

        self.check()

    def check(self):
        assert self.running
        result = self.proc.poll()
        if result is not None:
            self.running = False
            raise RuntimeError("workload '%s' stopped prematurely with error code %d" % (self.command_line, result))

    def stop(self):
        self.check()
        print "Stopping %r..." % self.command_line
        self.proc.send_signal(signal.SIGTERM)
        shutdown_grace_period = 10   # seconds
        end_time = time.time() + shutdown_grace_period
        while time.time() < end_time:
            result = self.proc.poll()
            if result is None:
                time.sleep(1)
            elif result == 0 or result == -signal.SIGTERM:
                print "OK"
                self.running = False
                break
            else:
                self.running = False
                raise RuntimeError("workload '%s' failed when interrupted with error code %d" % (self.command_line, result))
        else:
            raise RuntimeError("workload '%s' failed to terminate within %d seconds of SIGTERM" % (self.command_line, shutdown_grace_period))

    def __exit__(self, exc, ty, tb):
        if self.running:
            try:
                os.killpg(self.proc.pid, signal.SIGTERM)
            except OSError:
                pass


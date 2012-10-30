# Copyright 2010-2012 RethinkDB, all rights reserved.
import subprocess, os, signal, time, sys

def cmd_join(x):
    """The opposite of shlex.split()."""
    y = []
    for e in x:
        if " " not in e:
            y.append(e)
        else:
            y.append("\"%s\"" % e)
    return " ".join(y)

def signal_with_number(number):
    """Converts a numeric signal ID to a string like "SIGINT"."""
    for name in dir(signal):
        if name.startswith("SIG") and getattr(signal, name) == number:
            return name
    return str(number)

def wait_with_timeout(process, timeout):
    "waits with timeout"
    if timeout is None:
        return process.wait()
    else:
        wait_interval = 0.05
        n_intervals = int(timeout / wait_interval) + 1
        while process.poll() is None:
            n_intervals -= 1
            if n_intervals == 0:
                break
            time.sleep(wait_interval)
        return process.poll()

valgrind_error_code = 100

class SubProcess(object):
    def __init__(self, command_line, output_path, test_dir, valgrind_tool=None, interactive=False):
        if not os.path.exists(command_line[0]):
            raise RuntimeError("Bad executable: %r does not exist." % command_line[0])

        self.summary = os.path.basename(command_line[0])
        if len(command_line) > 4:
            self.summary += " " + cmd_join(command_line[1:4]) + " [...]"
        else:
            self.summary += " " + cmd_join(command_line[1:])

        self.valgrind_tool = valgrind_tool
        if valgrind_tool is not None:
            cmd_line = ["valgrind",
                "--log-file=%s" % test_dir.make_file(output_path + ".valgrind"),
                "--tool=%s" % valgrind_tool,
                "--error-exitcode=%d" % valgrind_error_code]
            if valgrind_tool == "memcheck":
                cmd_line.append("--leak-check=full")
                cmd_line.append("--track-origins=yes")
                cmd_line.append("--show-reachable=yes")
            if valgrind_tool == "drd":
                cmd_line.append("--read-var-info=yes")
            if interactive: cmd_line.append("--db-attach=yes")
            command_line = cmd_line + command_line

        self.interactive = interactive
        if interactive:
            self.output_path = "<stdout>"
            subprocess_stdout = sys.stdout
        else:
            self.output_path = test_dir.make_file(output_path)
            subprocess_stdout = file(self.output_path, "w")

        self.sub = subprocess.Popen(command_line,
            stdout=subprocess_stdout, stderr=subprocess.STDOUT)

    def verify(self):
        res = self.sub.poll()
        if res is None:
            return   # All is well

        if res == 0:
            raise RuntimeError("%r shut down without being asked to, but return code was 0." % \
                self.summary)
        elif self.valgrind_tool is not None and res == valgrind_error_code:
            raise RuntimeError("%r terminated unexpectedly with Valgrind errors. Output is at %r." % \
                (self.summary, self.output_path))
        elif res > 0:
            raise RuntimeError("%r terminated unexpectedly with return code %d. Output is at %r." % \
                (self.summary, res, self.output_path))
        elif res < 0:
            raise RuntimeError("%r terminated unexpectedly with signal %s. Output is at %r." % \
                (self.summary, signal_with_number(-res), self.output_path))

    def interrupt(self, timeout = 5):
        self.verify()

        try:
            self.sub.send_signal(signal.SIGINT)
        except:
            pass

        if self.valgrind_tool is not None:
            # Sometimes valgrind loses track of the first SIGINT, especially if it receives it
            # right as it is starting up. Send a second SIGINT to make sure the server gets it.
            # This is stupid and we shouldn't have to do this.
            if wait_with_timeout(self.sub, 10) is None:
                self.sub.send_signal(signal.SIGINT)

        self.wait(timeout)

    def wait(self, timeout):
        try:
            res = wait_with_timeout(self.sub, timeout)
        finally:
            try:
                self.sub.terminate()
            except:
                pass

        if res is None:
            raise RuntimeError("%r took more than %d seconds to shut down. Output is at %r." % \
                (self.summary, timeout, self.output_path))
        elif self.valgrind_tool is not None and res == valgrind_error_code:
            raise RuntimeError("Valgrind reported errors in %r. Output is at %r." % \
                (self.summary, self.output_path))
        elif res > 0:
            raise RuntimeError("%r exited with exit status %d. Output is at %r." % \
                (self.summary, res, self.output_path))
        elif res < 0:
            raise RuntimeError("%r exited with signal %s. Output is at %r." % \
                (self.summary, signal_with_number(-res), self.output_path))

    def kill(self):
        self.verify()
        try:
            self.sub.terminate()
        except:
            pass

    def __del__(self):
        # Avoid leaking processes in case there is an error
        if hasattr(self, "sub") and self.sub.poll() is None:
            self.sub.send_signal(signal.SIGKILL)

def run_executable(command_line, output_path, test_dir, timeout=60, valgrind_tool=None):

    summary = os.path.basename(command_line[0])
    for chunk in command_line[1:]:
        if len(summary) + len(chunk) > 200:
            summary += " [...]"
            break
        elif " " in chunk:
            summary += " \"%s\"" % chunk
        else:
            summary += " " + chunk
    print "Running %r;" % summary,
    sys.stdout.flush()

    sub = SubProcess(command_line, output_path, test_dir, valgrind_tool)

    start_time = time.time()
    sub.wait(timeout)
    end_time = time.time()

    print "done after %f seconds." % (end_time - start_time)

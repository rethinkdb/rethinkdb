# Copyright 2010-2012 RethinkDB, all rights reserved.
# General approach of this wrapper:
#  read files test_command and test_timeout, then do like run_test
#  write the result of the test into the files result_start_time, result_result and result_description


import subprocess, shlex, signal, os, time, shutil, tempfile, sys, traceback, types

poll_interval = 0.1

output_dir_name = "output_from_test"


# this is taken from the plain retester:
class SmartTemporaryFile(object):
    """tempfile.NamedTemporaryFile is poorly designed. A very common use case for a temporary file
    is to write some things into it, then close it, but want to leave it on the disk so it can be
    passed to something else. However, tempfile.NamedTemporaryFile either destroys its file as soon
    as it is closed or never destroys it at all. SmartTemporaryFile destroys its file only when it
    is GCed, unless take_file() is called, in which case it never destroys its file at all."""
    
    def __init__(self, *args, **kwargs):
        
        assert "delete" not in kwargs
        kwargs["delete"] = False
        
        # Clearly identify retest2 output files so that we can tell where they are coming from.
        kwargs["prefix"] = "rt2_" + kwargs.get("prefix", "u_")
        
        self.file = tempfile.NamedTemporaryFile(*args, **kwargs)
        self.need_to_delete = True
    
    def take_file(self):
        """By calling take_file(), the caller takes responsibility for destroying the temporary
        file."""
        if not self.need_to_delete:
            raise ValueError("take_file() called twice.")
        self.need_to_delete = False
        return self.name
    
    def __del__(self):
        if hasattr(self, "need_to_delete") and self.need_to_delete:
            try:
                os.remove(self.name)
            except AttributeError:
                # This is probably because os has been unloaded by
                # now. Unfortunate that we won't get to delete the
                # temp file, but alas
                pass
    
    # Forward everything to our internal NamedTemporaryFile object
    def __getattr__(self, name):
        return getattr(self.file, name)


def format_exit_code(code):
    """If the exit code is positive, return it in string form. If it is negative, try to figure out
    which signal it corresponds to."""
    
    if code < 0:
        for name in dir(signal):
            if name.startswith("SIG") and -getattr(signal, name) == code:
                return "signal %s" % name
    return "exit code %d" % code



# this is a modified variant of run_test() from retester...  

command = open('test_command', 'r').read()
timeout = open('test_timeout', 'r').read()
if timeout == "":
    timeout = None
else:
    timeout = int(timeout)

  
cwd = os.path.abspath(os.getcwd())

# Remove the old output directory if there is one
output_dir = os.path.join(cwd, output_dir_name)
if os.path.exists(output_dir):
    shutil.rmtree(output_dir)

# Capture the command's output
output = SmartTemporaryFile()

# Instruct the command to put any temp files in a new temporary directory in case it is bad at
# cleaning up after itself
temp_dir = os.path.join(cwd, "tmp")
if not os.path.isdir(temp_dir):
    os.mkdir(temp_dir)
environ = dict(os.environ)
environ["TMP"] = temp_dir
environ["PYTHONUNBUFFERED"] = "1"

start_time = time.time()

process = subprocess.Popen(
    command,
    stdout = output,
    stderr = subprocess.STDOUT,
    cwd = cwd,
    # Make a new session to make sure that the test doesn't spam our controlling terminal for
    # any reason
    preexec_fn = os.setsid,
    shell = True,
    env = environ
    )
try:
    if timeout is None:
    
        process.wait()
        if process.returncode == 0:
            result = (start_time, "pass", "")
        else:
            result = (start_time, "fail", "%r exited with %s." % \
                (command, format_exit_code(process.returncode)))
                
    else:
        # Wait 'timeout' seconds and see it if it dies on its own
        for i in xrange(int(timeout / poll_interval) + 1):
            time.sleep(poll_interval)
            
            if process.poll() is not None:
                # Cool, it died on its own.
                if process.returncode == 0:
                    result = (start_time, "pass", "")
                else:
                    result = (start_time, "fail", "%r exited with %s." % \
                        (command, format_exit_code(process.returncode)))
                break
        
        # Uh-oh, the timeout elapsed and it's still alive
        else:
        
            # First try to kill it nicely with SIGINT. This will give it a chance to shut down
            # smoothly on its own. There might be data in the output buffers it would be nice
            # to recover; also, it might have resources to clean up or subprocess of its own
            # to kill.
            try: process.send_signal(signal.SIGINT)
            except OSError: pass
            time.sleep(1)
            
            if process.poll() is not None:
                # SIGINT worked.
                result = (start_time, "fail", "%r failed to terminate within %g seconds, but " \
                    "exited with %s after being sent SIGINT." % \
                    (command, timeout, format_exit_code(process.poll())))
            
            else:
                # SIGINT didn't work, escalate to SIGQUIT
                try: process.send_signal(signal.SIGQUIT)
                except OSError: pass
                time.sleep(1)
                
                if process.poll() is not None:
                    result = (start_time, "fail", "%r failed to terminate within %g seconds and " \
                        "did not respond to SIGINT, but exited with %s after being sent " \
                        "SIGQUIT." % (command, timeout, format_exit_code(process.poll())))
                
                else:
                    # SIGQUIT didn't work either. We'll have to use SIGKILL, and we direct it at
                    # the entire process group (because our immediate child probably isn't going
                    # to be able to clean up after itself)
                    try: os.killpg(process.pid, signal.SIGKILL)
                    except OSError: pass
                    time.sleep(1)
                    
                    if process.poll() is not None:
                        result = (start_time, "fail", "%r failed to terminate within %g seconds and " \
                            "did not respond to SIGINT or SIGQUIT." % (command, timeout))
                    
                    else:
                        # I don't expect this to ever happen
                        result = (start_time, "fail", "%r failed to terminate within %g seconds and " \
                            "did not respond to SIGINT or SIGQUIT. Even SIGKILL had no " \
                            "apparent effect against this monster. I recommend you terminate " \
                            "it manually, because it's probably still rampaging through your " \
                            "system." % (command, timeout))

finally:
    # In case we ourselves receive SIGINT, or there is an exception in the above code, or we
    # terminate our immediate child process and it dies without killing the grandchildren.
    try: os.killpg(process.pid, signal.SIGKILL)
    except OSError: pass

output.close()

# In difference to the local script, we keep the output dir on the node as it is per-test anyway.
# Just move the test_output to there:
os.rename(output.take_file(), os.path.join(output_dir, "test_output.txt"))
    
    
# store results
open('result_running_time', 'w').write(str(time.time() - result[0]))
open('result_start_time', 'w').write(str(result[0]))
open('result_result', 'w').write(result[1])
open('result_description', 'w').write(result[2])


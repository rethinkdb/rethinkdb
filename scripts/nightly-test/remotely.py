import sys, subprocess32, tempfile, string, signal, os

class RemotelyInternalError(Exception):
    pass

class ScriptFailedError(Exception):
    def __init__(self, exit_code):
        self.exit_code = exit_code
    def __str__(self):
        return "Remote script failed with exit code %d" % self.exit_code

safe_characters = string.letters + string.digits + "-._/"
def escape_shell_arg(string):
    """escape_shell_arg(str) -> str
    If the given string contains any characters that will be intepreted
    specially by a shell, including whitespace, then return the string
    wrapped in single quotes and with any single quotes escaped."""
    for char in string:
        if char not in safe_characters:
            return "'" + string.replace("'", "\\'") + "'"
    else:
        return string

class SafePopen(object):
    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
    def __enter__(self):
        self.process = subprocess32.Popen(*self.args, **self.kwargs)
        return self.process
    def __exit__(self, type, value, traceback):
        try:
            self.process.terminate()
            self.process.wait()
        except OSError:
            pass

def run(command_line, stdout = sys.stdout, inputs = [], outputs = [],
        on_begin_transfer = lambda: None,
        on_begin_script = lambda: None, on_end_script = lambda: None,
        input_root = ".", output_root = ".",
        constraint = None,
        timeout = 60 * 6   # minutes
        ):
    """Runs `command_line` on a remote machine. Output will be written to
`stdout`. The working directory will be a temporary directory; paths in `inputs`
will be copied to the remote directory before running the script, and paths in
`outputs` will be copied back. When the script is scheduled and has actually
started to run, `on_begin_script()` will be called if it is not `None`. When the
script is done and its exit status is zero, `on_end_script()` will be called if
it is not `None`. If the script's exit status is non-zero, `run()` will throw a
`ScriptFailedError` and will not attempt to copy any files. If anything else
goes wrong, such as inputs or outputs not being found, `run()` will throw a
`RemotelyInternalError`."""
    for input in inputs:
        assert not os.path.isabs(input)
    for output in outputs:
        assert not os.path.isabs(output)
    command_script = """
set -eou pipefail
DIR="/tmp/$(cd /tmp; mktemp -d "remotely.XXXXXXXXXX")"
pushd "$DIR" > /dev/null
"""
    if inputs:
        command_script += """
echo "SEND_TARBALL"
tar --extract --gzip --touch --file=- -- %s
""" % " ".join(escape_shell_arg(input) for input in inputs)
    else:
        command_script += """
echo "SEND_NO_TARBALL"
"""
    if isinstance(command_line, (list, tuple)):
        command_line = " ".join(escape_shell_arg(cl) for cl in command_line)
    command_script += """
echo "SCRIPT_BEGIN"
set +e
(
%s
) | sed -u "s/^/STDOUT:/"
EXIT_CODE=$?
set -e
""" % command_line
    command_script += """
if [ $EXIT_CODE == 0 ]; then
echo "SCRIPT_SUCCESS"
"""
    if outputs:
        command_script += """
echo "HERE_IS_TARBALL"
tar --create --gzip --file=- -- %s
""" % " ".join(escape_shell_arg(out) for out in outputs)
    else:
        command_script += """
echo "HERE_IS_NO_TARBALL"
"""
    command_script += """
else
echo "SCRIPT_FAILURE $EXIT_CODE"
fi
popd > /dev/null
rm -rf "$DIR"
"""
    srun_command = ["srun"]
    if constraint is not None:
        srun_command += ["-C", constraint]
    if timeout is not None:
        srun_command += ["-t", str(timeout)]
    srun_command += ["bash", "-c", command_script]
    # For some reason, it's essential to set `close_fds` to `True` or else
    # `run()` will lock up if you run it in multiple threads concurrently.
    with SafePopen(srun_command, stdin = subprocess32.PIPE, stdout = subprocess32.PIPE, close_fds = True) as srun_process:
        line = srun_process.stdout.readline()
        if inputs:
            if not line.startswith("SEND_TARBALL"):
                raise RemotelyInternalError("Expected 'SEND_TARBALL', got %r" % line)
            on_begin_transfer()
            subprocess32.check_call(
                ["tar", "--create", "--gzip", "--file=-", "-C", input_root, "--"] + inputs,
                stdout = srun_process.stdin
                )
        else:
            if not line.startswith("SEND_NO_TARBALL"):
                raise RemotelyInternalError("Expected 'SEND_NO_TARBALL', got %r" % line)
            on_begin_transfer()
        srun_process.stdin.close()
        line = srun_process.stdout.readline()
        if not line.startswith("SCRIPT_BEGIN"):
            raise RemotelyInternalError("Expected 'SCRIPT_BEGIN', got %r" % line)
        on_begin_script()
        while True:
            line = srun_process.stdout.readline()
            if line.startswith("STDOUT"):
                stdout.write(line[line.find(":")+1:])
            else:
                break
        if line.startswith("SCRIPT_SUCCESS"):
            on_end_script()
            line = srun_process.stdout.readline()
            if outputs:
                if not line.startswith("HERE_IS_TARBALL"):
                    raise RemotelyInternalError("Expected 'HERE_IS_TARBALL', got %r" % line)
                # The Python file's current `tell()` location points at the
                # beginning of the tarball, but the underlying file
                # descriptor's `tell()` location might point to somewhere
                # past that because of Python's read buffering. Calling
                # `flush()` forces Python to set the underlying FD's
                # location to the beginning of the tarball so that our `tar`
                # subprocess gets the right input.
                srun_process.stdout.flush()
                subprocess32.check_call(
                    ["tar", "--extract", "--gzip", "--touch", "--file=-", "-C", output_root, "--"] + outputs,
                    stdin = srun_process.stdout
                    )
            else:
                if not line.startswith("HERE_IS_NO_TARBALL"):
                    raise RemotelyInternalError("Expected 'HERE_IS_NO_TARBALL', got %r" % line)
        elif line.startswith("SCRIPT_FAILURE"):
            final_exit_code = int(line.split()[1].strip())
            raise ScriptFailedError(final_exit_code)
        else:
            raise RemotelyInternalError("bad line: %r" % line)
        exit_code = srun_process.wait()
        if exit_code != 0:
            raise RemotelyInternalError("remote script failed: %d" % exit_code)

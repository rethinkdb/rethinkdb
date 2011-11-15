import sys, subprocess, tempfile, string, signal, os

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

def run(command_line, stdout = sys.stdout, inputs = [], outputs = [],
        on_begin_script = None, on_end_script = None):
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
echo "BEGIN"
set -eou pipefail
DIR="/tmp/$(cd /tmp; mktemp -d "remotely.XXXXXXXXXX")"
pushd "$DIR" > /dev/null
"""
    if inputs:
        input_tar_process = subprocess.Popen(
            ["tar", "--create", "-z", "--file=-"] + inputs,
            stdout = subprocess.PIPE
            )
        command_script += """
tar --extract -z --file=-
"""
    if isinstance(command_line, (list, tuple)):
        command_line = " ".join(escape_shell_arg(cl) for cl in command_line)
    command_script += """
echo "SCRIPT_BEGIN"
set +e
(
%s
) | sed "s/^/STDOUT:/"
EXIT_CODE=$?
set -e
""" % command_line
    command_script += """
if [ $EXIT_CODE == 0 ]; then
echo "SCRIPT_SUCCESS"
"""
    if outputs:
        command_script += """
echo "TARBALL"
tar --create -z --file=- -- %s
""" % " ".join(escape_shell_arg(out) for out in outputs)
    else:
        command_script += """
echo "NO_TARBALL"
"""
    command_script += """
else
echo "SCRIPT_FAILURE $EXIT_CODE"
fi
popd > /dev/null
rm -rf "$DIR"
"""
    with tempfile.NamedTemporaryFile() as unparsed_output_file:
        srun_command = [
            "srun",
            "--output", unparsed_output_file.name,
            "--error", "all",
            "bash",
            "-c",
            command_script
            ]
        srun_process = subprocess.Popen(
            srun_command,
            stdin = input_tar_process.stdout if inputs else open("/dev/null")
            )
        try:
            line = unparsed_output_file.readline()
            if not line.startswith("BEGIN"):
                raise RemotelyInternalError("Expected 'BEGIN', got %r" % line)
            if inputs:
                exit_code = input_tar_process.wait()
                if exit_code != 0:
                    srun_process.terminate()
                    raise RemotelyInternalError("input tar failed: %d" % exit_code_1)
            line = unparsed_output_file.readline()
            if not line.startswith("SCRIPT_BEGIN"):
                raise RemotelyInternalError("Expected 'SCRIPT_BEGIN', got %r" % line)
            if on_begin_script is not None:
                on_begin_script()
            while True:
                line = unparsed_output_file.readline()
                if line.startswith("STDOUT"):
                    stdout.write(line[line.find(":")+1:])
                else:
                    break
            if line.startswith("SCRIPT_SUCCESS"):
                if on_end_script is not None:
                    on_end_script()
                line = unparsed_output_file.readline()
                if outputs:
                    if not line.startswith("TARBALL"):
                        raise RemotelyInternalError("Expected 'TARBALL', got %r" % line)
                    # The Python file's current `tell()` location points at the
                    # beginning of the tarball, but the underlying file
                    # descriptor's `tell()` location might point to somewhere
                    # past that because of Python's read buffering. Calling
                    # `flush()` forces Python to set the underlying FD's
                    # location to the beginning of the tarball so that our `tar`
                    # subprocess gets the right input.
                    unparsed_output_file.flush()
                    output_tar_process = subprocess.Popen(
                        ["tar", "--extract", "-z", "--file=-"],
                        stdin = unparsed_output_file
                        )
                    exit_code = output_tar_process.wait()
                    if exit_code != 0:
                        raise RemotelyInternalError("output tar failed: %d" % exit_code)
                else:
                    if not line.startswith("NO_TARBALL"):
                        raise RemotelyInternalError("Expected 'NO_TARBALL', got %r" % line)
            elif line.startswith("SCRIPT_FAILURE"):
                final_exit_code = int(line.split()[1].strip())
                raise ScriptFailedError(final_exit_code)
            else:
                raise RemotelyInternalError("bad line: %r" % line)
            exit_code = srun_process.wait()
            if exit_code != 0:
                raise RemotelyInternalError("remote script failed: %d" % exit_code_2)
        finally:
            srun_process.terminate()

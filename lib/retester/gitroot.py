# Copyright 2010-2012 RethinkDB, all rights reserved.

import subprocess, os

def run_command(cmd, expected_rcs = [0]):
    """Run the command and return the result as a string. Substitute for subprocess.check_output()
    for those of us on Python 2.6."""
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output = process.communicate()[0]
    if process.returncode not in expected_rcs:
        raise IOError("%s exited with return code %d and message %r" % \
            (repr(" ".join(cmd)), process.returncode, output))
    return output

def get_repo_root():
    """Returns the root of the git repository we're currently in."""
    return run_command(["git", "rev-parse", "--show-cdup"]).strip()

def set_repo_cwd(_dir):
    """Sets the current directory relative to the repo"""
    os.chdir(os.path.join(os.path.abspath(get_repo_root()), _dir))


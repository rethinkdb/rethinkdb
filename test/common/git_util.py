# Copyright 2010-2012 RethinkDB, all rights reserved.
import os
from subprocess import Popen, PIPE, check_call

def repo_toplevel():
    p = Popen("git rev-parse --show-toplevel".split(' '), stdout=PIPE)
    result = p.communicate()[0].rstrip('\n')
    if p.wait() == 0:
        return result
    else:
        return None

def repo_has_local_changes():
    check_call("git update-index -q --refresh".split(' '))
    output = Popen("git diff-index --name-only HEAD --".split(' '), stdout=PIPE).communicate()[0]
    return bool(output)

def repo_version():
    tl = repo_toplevel()
    if tl:
        p = Popen([os.path.join(tl, "scripts/gen-version.sh")], stdout=PIPE)
        result = p.communicate()[0].rstrip('\n')
        if p.wait() == 0:
            return result
    return None

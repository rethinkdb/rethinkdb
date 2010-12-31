import os
from subprocess import Popen, PIPE, check_call

def repo_toplevel():
    # import pdb; pdb.set_trace()
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



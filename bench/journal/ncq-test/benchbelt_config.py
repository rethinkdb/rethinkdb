
from benchbelt_util import *

# Some config values
duration = 30
password = 'legion'
logname = 'stats-ncq-%Y-%m-%d.%H-%M-%S'

# Run each benchmark at least 3 times. Stop after 10 runs or when the
# margin of error is less than 5% of the mean, whichever comes first
margins = (3, 10, 0.05)

# Parameters for the run
devices = ['/mnt/ssd/test']
rebench_args_ex = ['-b', '4096', '-s', '4096']

# Run setup
def setup_run(depth, args):
    run = get_arg(args, 'type')
    args = set_arg(args, 'eventfd', 'off')
    if run == 'stateless':
        args = set_arg(args, 'threads', depth)
        args = del_arg(args, 'queue-depth')
    elif run == 'naio_eventfd':
        args = set_arg(args, 'type', 'naio')
        args = set_arg(args, 'eventfd', 'on')
        rebench_args_ex.extend(['--eventfd'])
    return args

# Run description
run_config = [('type', ['stateless', 'naio', 'naio_eventfd'],
               { 'line-break': True }),
              ('eventfd', ['']),     # We want this to show up in the log
              ('queue-depth', [2**x for x in range(0, 8)],
               { 'setup' : setup_run })]

# Exceptions
rebench_except = ['eventfd']


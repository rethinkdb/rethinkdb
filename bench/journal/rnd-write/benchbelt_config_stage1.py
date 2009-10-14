
from rnd_write_setup import *

# Some config values
password = 'legion25'
logname = 'stats'
duration = 30

# Run each benchmark at least 3 times. Stop after 10 runs or when the
# margin of error is less than 5% of the mean, whichever comes first
margins = (3, 10, 0.05)

# Parameters for the run
devices = ['/dev/sdb']
mount_dir = '/mnt/ssd'
rebench_args_ex = ['-o', 'write']

# Run description - a list of tuples. For each tuple, the first
# element is passed to rebench (with '--' prepended to it), the second
# element is treated as a list of values to pass on every
# iteration. The third (optional) element is a hash with various
# settings (documented below).
run_config = [('used-space', [15], # specified in percentages
               { 'setup' : setup_disk(password, mount_dir) }),
              ('block_size', [2**x * 512 for x in range(0, 11)],
               { 'line-break': True }),
              ('stride', [2**x * 512 for x in range(0, 11)])]

# Arguments on this list will not be passed to rebench
rebench_except = ['used-space']

# Documentation of hash parameters:
#
# 'line-break' - if True, puts a line break into the output file after
# each benchmark is completed.
#
# 'setup' - calls a function before the execution of the benchmark with
# the argument from the list.
#
# 'teardown' - calls a function after the execution of the benchmark
# with the argument from the list.



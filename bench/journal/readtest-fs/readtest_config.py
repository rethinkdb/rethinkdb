
from readtest_fs_setup import *

# Some config values
duration = 1
password = 'legion25'
logname = 'stats'

# Run each benchmark at least 3 times. Stop after 10 runs or when the
# margin of error is less than 5% of the mean, whichever comes first
margins = (3, 10, 0.05)

# Parameters for the run
devices = ['/dev/sdb']
mount_dir = '/mnt/ssd'
test_file_size = 1048576 # This is in 1024 blocks, so 1GB

# Run description - a list of tuples. For each tuple, the first
# element is passed to rebench (with '--' prepended to it), the second
# element is treated as a list of values to pass on every
# iteration. The third (optional) element is a hash with various
# settings (documented below).
run_config = [('partitioning', ['none', 'regular', 'aligned'],
               { 'setup' : fdisk(password) }),
              ('filesystem', ['none', 'ext2'],
               { 'setup' : prepare_fs(password, mount_dir, test_file_size),
                 'teardown' : cleanup_fs(password, mount_dir) }),
              ('block_size', [x * 512 for x in range(1, 2)],
               { 'line-break': True }),
              ('stride', [x * 512 for x in range(1, 2)])]

# Arguments on this list will not be passed to rebench
rebench_except = ['partitioning', 'filesystem']

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



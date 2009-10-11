
# Some config values
duration = 30
password = 'legion25'
logname = 'stats'

# Run each benchmark at least 3 times. Stop after 10 runs or when the
# margin of error is less than 5% of the mean, whichever comes first
margins = (3, 10, 0.05)

# Parameters for the run
devices = ['/dev/sdb']

# Run description - a list of tuples. For each tuple, the first
# element is passed to rebench (with '--' prepended to it), the second
# element is treated as a list of values to pass on every
# iteration. The third (optional) element is a hash with various
# settings (documented below).
run_config = [('type', ['stateless']),
              ('workload', ['seq', 'rnd']),
              ('threads', [2**x for x in range(0, 10)]),
              ('block_size', [2**x * 512 for x in range(0, 5)],
               { 'line-break': True }),
              ('stride', [2**x * 512 for x in range(0, 5)])]


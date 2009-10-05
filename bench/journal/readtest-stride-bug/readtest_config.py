
# Parameters to the benchmark
duration = 30
password = 'legion25'
devices = ['/dev/sda', '/dev/sdb']
distros = ['uniform', 'normal', 'pow']
blocks = [x * 512 for x in range(1, 21)]
logname = 'stats'

# Run each benchmark at least 3 times. Stop after 10 runs or when the
# margin of error is less than 5% of the mean, whichever comes first
runs = (3, 10, 0.05)


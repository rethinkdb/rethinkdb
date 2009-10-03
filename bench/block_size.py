#!/usr/bin/python

import subprocess
from StringIO import StringIO

# Parameters to the benchmark
duration = 5
password = 'legion25'
devices = ['/dev/sda', '/dev/sdb']
distros = ['uniform', 'normal', 'pow']
blocks = [x * 512 for x in range(1, 21)]

# Run the benchmark
for device in devices:
    for distro in distros:
        for block in blocks:
            p = subprocess.Popen(['sudo', '-S',
                                  'rebench', '-d', str(duration), '-u', distro, '-b', str(block), device],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            p.stdin.write(password)
            p.stdin.close()
            p.wait()
            for line in p.stdout:
                print line



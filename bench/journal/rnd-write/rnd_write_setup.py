import os
import sys
from benchbelt_util import *
import subprocess

def get_device_length(device, password):
    p = subprocess.Popen(['sudo', '-S', './filelen.py', device],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    p.stdin.write(password)
    p.stdin.close()
    p.wait()
    return int(p.stdout.readline())

def setup_disk(password, mount_dir):
    def fdisk_aux(space, args):
        device = get_arg(args, 'device')
        file_path = os.path.join(mount_dir, 'bench_file')
        length = get_device_length(device, password)
        length = int(length / 100.0 * space / 1024.0)
        subprocess.call([os.path.join(sys.argv[1], 'setup-disk'),
                         password, device, 'sdb.aligned', mount_dir, device + '1', file_path, str(length)])
        device = file_path
        return set_arg(args, 'device', device)
    return fdisk_aux


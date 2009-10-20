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
    def fdisk_aux(duration, args):
        # some setup
        space = get_arg(args, 'used-space')
        device = get_arg(args, 'device')
        file_path = os.path.join(mount_dir, 'bench_file')
        # figure out the space
        device_length = get_device_length(device, password)
        file_size = int(device_length / 100.0 * space / 1024.0)
        # setup disk
        subprocess.call([os.path.join(sys.argv[1], 'setup-disk'),
                         password, device, 'sdb.aligned', mount_dir, device + '1', file_path, str(file_size)])
        # Set the target file and duration
        args = set_arg(args, 'device', file_path)
        args = set_arg(args, 'duration',
                       str(int(device_length / 100.0 * duration / 1024.0 / 1024.0)) + 'm')
        return args
    return fdisk_aux


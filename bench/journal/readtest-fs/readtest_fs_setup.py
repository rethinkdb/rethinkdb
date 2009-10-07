
from readtest_util import *

# Setting up the benchmark
def fdisk(mode, args):
    device = get_arg(args, 'device')
    if mode == 'none':
        pass
    elif mode == 'regular':
        # Do a regular partition
        device += '1'
    elif mode == 'aligned':
        # Do an 'aligned' partition
        device += '1'
    return set_arg(args, 'device', device)

def prepare_fs(name, args):
    if name == 'none':
        pass
    elif name == 'ext2':
        # Create the filesystem
        # Mount the filesystem
        # Create the file
        # Pass the file name
        pass

def teardown_fs(name, args):
    if mode == 'ext2':
        # Unmount the filesystem
        pass


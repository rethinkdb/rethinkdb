
import os
import sys
from readtest_util import *
from subprocess import call

# Setting up the benchmark
def fdisk(password):
    def fdisk_aux(mode, args):
        device = get_arg(args, 'device')
        if mode == 'none':
            pass
        elif mode == 'regular':
            # Do a regular partition
            call([os.path.join(sys.argv[1], 'format-disk'), password, device, 'sdb.regular'])
            device += '1'
        elif mode == 'aligned':
            # Do an aligned partition
            call([os.path.join(sys.argv[1], 'format-disk'), password, device, 'sdb.aligned'])
            device += '1'
        return set_arg(args, 'device', device)
    return fdisk_aux

def prepare_fs(password, mount_dir, test_file_size):
    def prepare_fs_aux(name, args):
        device = get_arg(args, 'device')
        if name == 'none':
            pass
        elif name == 'ext2':
            # Create the filesystem
            test_file = os.path.join(mount_dir, 'test.file')
            call([os.path.join(sys.argv[1], 'prepare-fs'), password, device, mount_dir, test_file,
                  str(test_file_size)])
            return set_arg(args, 'device', test_file)
    return prepare_fs_aux

def cleanup_fs(password, mount_dir):
    def cleanup_fs_aux(name):
        if name == 'ext2':
            # Cleanup
            call([os.path.join(sys.argv[1], 'cleanup-fs'), password, mount_dir])
    return cleanup_fs_aux


#!/usr/bin/python

import sys
import os

def get_device_length(device):
    f = open(device, 'r')
    f.seek(0, os.SEEK_END)
    s = f.tell()
    f.close()
    return s

def main(argv):
    length = get_device_length(argv[1])
    print length

if __name__ == '__main__':
    sys.exit(main(sys.argv))

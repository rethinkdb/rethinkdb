#!/usr/bin/python
import os
import array
import random
import sys

bits = map(lambda x : x ** 2, range(8))

#p is the probability of flipping a bit
def corrupt(filename, p):

    if p == 0.0:
        return 

    print "Corrupting...",

    def flipcoin():
        return random.random() < p

    with open(filename, 'rb') as in_fd:
        in_data = in_fd.read()
        data = array.array('B')
        data.fromstring(in_data)

        for i in range(len(data)):
            for bit in bits:
                if flipcoin():
                    data[i] ^= bit

    with open(filename, 'wb') as out_fd:
        data.tofile(out_fd)

    print "...Done"

def main():
    def usage(argv):
        print 'Usage: %s p' % argv[0]
        print 'p : each bits probability of being flipped'
    if len(sys.argv) != 2:
        usage(sys.argv)
        exit(2)
    corrupt(sys.stdin, sys.stdout, float(sys.argv[1]))

if __name__ == '__main__':
    main()

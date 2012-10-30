#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import array
import random

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
            for bit in xrange(8):
                if flipcoin():
                    data[i] ^= (1 << bit)

    with open(filename, 'wb') as out_fd:
        data.tofile(out_fd)

    print "...Done"

if __name__ == '__main__':
    print "Sorry, you can't do that."
    exit(2)



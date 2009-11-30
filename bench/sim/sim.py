#!/usr/bin/python
import sys
from threading import Thread
import socket
import random

nthreads = 500
nrequests = 5000
ip = '192.168.2.5'
port = 8080

class test_client(Thread):
    def run(__self__):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((ip, port))
        for x in xrange(0, nrequests):
            sock.send(str(random.randrange(0, 3288576, 512)))
            sock.recv(10)
        sock.close()

def run_script():
    random.seed()
    threads = []
    for x in xrange(0, nthreads):
        thread = test_client()
        threads.append(thread)
        thread.start()
    for x in xrange(0, nthreads):
        threads[x].join()
        
if __name__ == '__main__':
    run_script()

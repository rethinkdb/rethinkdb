# Copyright 2010-2012 RethinkDB, all rights reserved.
import threading, os

class LinearDBWriter(object):
    def __init__(self, path):
        self.file = open(path, "w")
        self.lock = threading.Lock()
    def __enter__(self):
        return self
    def __exit__(self, type, value, traceback):
        self.file.close()
    def write(self, *path, **values):
        for thing in path:
            assert isinstance(thing, (str, int))
        with self.lock:
            print >>self.file, (path, values)
            self.file.flush()

def read_linear_db(filename):
    root = {}
    if os.path.exists(filename):
        with open(filename, "r") as file:
            for line in file:
                try:
                    (path, values) = eval(line)
                except Exception, e:
                    break
                object = root
                for thing in path:
                    if thing not in object:
                        object[thing] = {}
                    object = object[thing]
                object.update(values)
    return root

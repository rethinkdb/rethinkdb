#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys
from sys import stdout, exit, path
import json
import os
import math


path.insert(0, "../../drivers/python")

from util import compare

def load_files(file1, file2):
    """
    Loads the results from two tests, and generate an HTML page with the differences.
    """

    f = open("results/" + file1, "r")
    results = json.loads(f.read())
    f.close()

    f = open("results/" + file2, "r")
    previous_results = json.loads(f.read())
    f.close()

    compare(results, previous_results)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print """
        Usage: python comparison.py <file1> <file2>
        Where file1 and file2 are the names of the files in results/ and file1 is the recent results
        """
    else:
        load_files(sys.argv[1], sys.argv[2])

#!/usr/bin/python
# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import sys
import time
import json
import os
import math
import subprocess

from util import gen_doc, gen_num_docs, compare
from queries import constant_queries, table_queries, write_queries, delete_queries

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, utils

r = utils.import_python_driver()

# We define 4 tables (small/normal cache with small/big documents
servers_settings = [
    {
        "cache_size": 1024,
        "name": "inmemory"
    },
    {
        "cache_size": 16,
        "name": "outmemory"
    }
]

tables = [
    {
        "name": "smalldoc", # Name of the table
        "size_doc": "small", # Small docs
        "ids": [] # Used to perform get
    },
    {
        "name": "bigdoc",
        "size_doc": "big",
        "ids": []
    }
]

# We execute each query for 60 seconds or 1000 times, whatever comes first
time_per_query = 60 # 1 minute max per query
executions_per_query = 1000 # 1000 executions max per query

# Global variables -- so we don't have to pass them around
results = {} # Save the time per query (average, min, max etc.)
connection = None

def run_tests(build=None, data_dir='./'):
    global connection, servers_settings
    
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)
    if not os.path.isdir(data_dir):
        raise ValueError('data_dir is not a directory: %s' % str(data_dir))
    
    executable_path = utils.find_rethinkdb_executable() if build is None else os.path.realpath(os.path.join(build, 'rethinkdb'))
    
    if not os.path.basename(os.path.dirname(executable_path)).startswith('release'):
        sys.stderr.write('Warning: Testing a non-release build: %s\n' % executable_path)
    else:
        print('Testing: %s' % executable_path)
    
    i = 0
    for settings in servers_settings:

        print("Starting server with cache_size " + str(settings["cache_size"]) + " MB...", end=' ')
        sys.stdout.flush()
        metacluster = driver.Metacluster()
        serverFiles = driver.Files(metacluster, settings["name"], db_path=os.path.join(data_dir, settings["name"]))
        
        with driver.Process(files=serverFiles, executable_path=executable_path, extra_options=['--cache-size', str(settings["cache_size"])]) as server:
            
            print(" Done.\nConnecting...", end=' ')
            sys.stdout.flush()

            connection = r.connect(host="localhost", port=server.driver_port)
            print(" Done.")
            sys.stdout.flush()

            init_tables(connection)

            # Tests
            execute_read_write_queries(settings["name"])

            if i == 0:
                execute_constant_queries()
            i = i + 1

    save_compare_results()

def init_tables(connection):
    """Create the tables we are going to use"""
    global tables

    print("Creating databases/tables...", end=' ')
    sys.stdout.flush()
    try:
        r.db_drop("test").run(connection)
    except r.errors.RqlRuntimeError as e:
        pass

    r.db_create("test").run(connection)

    for table in tables:
        r.db("test").table_create(table["name"]).run(connection)

    for table in tables:
        r.db("test").table(table["name"]).index_create("field0").run(connection)
        r.db("test").table(table["name"]).index_create("field1").run(connection)
        r.db("test").table(table["name"]).index_wait().run(connection)

    print(" Done.")
    sys.stdout.flush()



def execute_read_write_queries(suffix):
    """
    Execute all the queries (inserts/update, reads, delete)
    """
    global results, connection, time_per_query, executions_per_query, constant_queries

    print("Running inserts...", end=' ')
    sys.stdout.flush()
    for table in tables:
        docs = []
        num_writes = gen_num_docs(table["size_doc"])
        for i in xrange(num_writes):
            docs.append(gen_doc(table["size_doc"], i))

        
        i = 0

        durations = []
        start = time.time()
        while time.time() - start < time_per_query and i < num_writes:
            start_query = time.time()
            result = r.db('test').table(table['name']).insert(docs[i]).run(connection)
            durations.append(time.time() - start_query)

            if "generated_keys" in result:
                table["ids"].append(result["generated_keys"][0])
            i += 1

        durations.sort()
        results["single-inserts-" + table["name"] + "-" + suffix] = {
            "average": (time.time() - start) / i,
            "min": durations[0],
            "max": durations[len(durations) - 1],
            "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
            "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
        }

        # Save it to know how many batch inserts we did
        single_inserts = i

        # Finish inserting the remaining data
        size_batch = 500
        durations = []
        start = time.time()
        count_batch_insert = 0;
        if i < num_writes:
            while i + size_batch < num_writes:
                start_query = time.time()
                result = r.db('test').table(table['name']).insert(docs[i:i + size_batch]).run(connection)
                durations.append(time.time() - start_query)
                end = time.time()
                count_batch_insert += 1

                table["ids"] += result["generated_keys"]
                i += size_batch

            if i < num_writes:
                result = r.db('test').table(table['name']).insert(docs[i:len(docs)]).run(connection)
                table["ids"] += result["generated_keys"]
        
        if num_writes - single_inserts != 0:
            results["batch-inserts-" + table["name"] + "-" + suffix] = {
                "average": (end - start) / (count_batch_insert * size_batch),
                "min": durations[0],
                "max": durations[len(durations) - 1],
                "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
                "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
            }
        
        table["ids"].sort()
    
    print(" Done.")
    sys.stdout.flush()


    # Execute the insert queries
    print("Running update/replace...", end=' ')
    sys.stdout.flush()
    for table in tables:
        for p in xrange(len(write_queries)):
            docs = []
            num_writes = gen_num_docs(table["size_doc"])
            for i in xrange(num_writes):
                docs.append(gen_doc(table["size_doc"], i))

            i = 0

            durations = []
            start = time.time()
            while time.time() - start < time_per_query and i < len(table["ids"]):
                start_query = time.time()
                eval(write_queries[p]["query"]).run(connection)
                durations.append(time.time() - start_query)
                i += 1

            durations.sort()
            results[write_queries[p]["tag"] + "-" + table["name"] + "-" + suffix] = {
                "average": (time.time() - start) / i,
                "min": durations[0],
                "max": durations[len(durations) - 1],
                "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
                "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
            }

            i -= 1 # We need i in write_queries[p]["clean"] (to revert only the document we updated)
            # Clean the update
            eval(write_queries[p]["clean"]).run(connection)

    print(" Done.")
    sys.stdout.flush()


    # Execute the read queries on every tables
    print("Running reads...", end=' ')
    sys.stdout.flush()
    for table in tables:
        for p in xrange(len(table_queries)):
            count = 0
            i = 0
            if "imax" in table_queries[p]:
                max_i = table_queries[p]["imax"] + 1
            else:
                max_i = 1

            durations = []
            start = time.time()
            while time.time() - start < time_per_query and count < executions_per_query:
                start_query = time.time()
                try:
                    cursor = eval(table_queries[p]["query"]).run(connection)
                    if isinstance(cursor, r.net.Cursor):
                        list(cursor)
                        cursor.close()

                    if i >= len(table["ids"]) - max_i:
                        i = 0
                    else:
                        i += 1
                except:
                    print("Query failed")
                    print(constant_queries[p])
                    sys.stdout.flush()
                    break
                durations.append(time.time() - start_query)
                count += 1

            durations.sort()
            results[table_queries[p]["tag"] + "-" + table["name"] + "-" + suffix] = {
                "average": (time.time() - start) / count,
                "min": durations[0],
                "max": durations[len(durations) - 1],
                "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
                "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
            }


    print(" Done.")
    sys.stdout.flush()


    # Execute the delete queries
    print("Running delete...", end =' ')
    sys.stdout.flush()
    for table in tables:
        for p in xrange(len(delete_queries)):
            start = time.time()

            i = 0

            durations = []
            start = time.time()
            while time.time() - start < time_per_query and i < len(table["ids"]):
                start_query = time.time()
                eval(delete_queries[p]["query"]).run(connection)
                durations.append(time.time() - start_query)

                i += 1

            durations.sort()
            results[delete_queries[p]["tag"] + "-" + table["name"] + "-" + suffix] = {
                "average": (time.time() - start) / i,
                "min": durations[0],
                "max": durations[len(durations) - 1],
                "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
                "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
            }


    print(" Done.")
    sys.stdout.flush()

def execute_constant_queries():
    global results

    # Execute the queries that do not require a table
    print("Running constant queries...", end=' ')
    sys.stdout.flush()
    for p in xrange(len(constant_queries)):
        count = 0
        durations = []
        start = time.time()
        while time.time() - start < time_per_query and count < executions_per_query:
            start_query = time.time()
            if type(constant_queries[p]) == type(""):
                try:
                    cursor = eval(constant_queries[p]).run(connection)
                    if isinstance(cursor, r.net.Cursor):
                        list(cursor)
                        cursor.close()
                except:
                    print("Query failed")
                    print(constant_queries[p])
                    sys.stdout.flush()
            else:
                cursor = eval(constant_queries[p]["query"]).run(connection)
                if isinstance(cursor, r.net.Cursor):
                    list(cursor)
                    cursor.close()
            durations.append(time.time() - start_query)
            
            count += 1
        
        durations.sort()
        if type(constant_queries[p]) == type(""):
            results[constant_queries[p]] = {
                "average": (time.time() - start) / count,
                "min": durations[0],
                "max": durations[len(durations) - 1],
                "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
                "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
            }
        else:
            results[constant_queries[p]["tag"]] = {
                "average": (time.time() - start) / count,
                "min": durations[0],
                "max": durations[len(durations) - 1],
                "first_centile": durations[int(math.floor(len(durations) / 100. * 1))],
                "last_centile": durations[int(math.floor(len(durations) / 100. * 99))]
            }

    print(" Done.")
    sys.stdout.flush()

def stop_cluster(cluster):
    """Stop the cluster"""
    cluster.check_and_stop()

def check_driver():
    '''If this driver is protobuf based, make sure we are using the C++ backend.'''
    if hasattr(r, 'protobuf_implementation') and r.protobuf_implementation == 'py':
        sys.exit("Please install the C++ backend for the tests.")

def save_compare_results():
    """Save the current results, and if previous results are available, generate an HTML page with the differences"""
    global results, str_date

    commit = out = subprocess.Popen(['git', 'log', '-n 1', '--pretty=format:"%H"'], stdout=subprocess.PIPE).communicate()[0]
    results["hash"] = commit

    # Save results
    if not os.path.exists("results"):
        os.makedirs("results")

    str_date = time.strftime("%y.%m.%d-%H:%M:%S")
    f = open("results/result_" + str_date + ".txt", "w")

    str_res = json.dumps(results, indent=2)
    f.write(str_res)
    f.close()

    # Read all the previous results stored in results/
    file_paths = []
    for root, directories, files in os.walk("results/"):
        for filename in files:
            # Join the two strings in order to form the full filepath.
            filepath = os.path.join(root, filename)
            file_paths.append(filepath)  # Add it to the list.

    # Sort files, we just need the last one (the most recent one)
    file_paths.sort()
    if len(file_paths) > 1:
        last_file = file_paths[-2] # The last file is the one we just saved

        f = open(last_file, "r")
        previous_results = json.loads(f.read())
        f.close()
    else:
        previous_results = {}

    compare(results, previous_results)


def main(data_dir):
    """Main method"""
    check_driver()
    run_tests(data_dir=data_dir)

if __name__ == "__main__":
    data_dir = './'
    if len(sys.argv) > 1:
        data_dir = sys.argv[1]
    main(data_dir)

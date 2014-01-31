#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys
from sys import stdout, exit
import time
import json
import os

from util import gen_doc, gen_num_docs
from queries import constant_queries, table_queries, write_queries, delete_queries, range_write_queries

import rethinkdb as r

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'rql_test')))
from test_util import RethinkDBTestServers


import pdb

# We define 4 tables (small/normal cache with small/big documents
tables = [
    {
        "name": "smallinmemory", # Name of the table
        "size_doc": "small", # Small docs
        "cache": 1024*1024*1024, #1024MB
        "ids": [] # Used to perform get
    },
    {
        "name": "smalloutmemory",
        "size_doc": "small",
        "cache": 1024*1024*16,
        "ids": []
    },
    {
        "name": "biginmemory",
        "size_doc": "big",
        "cache": 1024*1024*1024,
        "ids": []
    },
    {
        "name": "bigoutmemory",
        "size_doc": "big",
        "cache": 1024*1024*16,
        "ids": []
    }
]

# We execute each query for 60 seconds or 1000 times, whatever comes first
time_per_query = 60 # 1 minute max per query
executions_per_query = 1000 # 1000 executions max per query

# Global variables -- so we don't have to pass them around
results = {}
connection = None

def run_tests(build="../../build/release"):
    global connection
    print "Starting cluster...",
    with RethinkDBTestServers(1, server_build_dir=build) as servers:
        print " Done."

        print "Connecting...",
        connection = connect(servers)
        print " Done."

        init_tables()

        # Tests
        execute_queries()

    save_compare_results()


def connect(servers):
    return r.connect(host="localhost", port=servers.driver_port())


def init_tables():
    """
    Create the tables we are going to use
    """
    global connection, tables

    print "Creating databases/tables...",
    try:
        r.db_drop("test").run(connection)
    except r.errors.RqlRuntimeError, e:
        pass

    r.db_create("test").run(connection)

    for table in tables:
        r.db("test").table_create(table["name"], cache_size=table["cache"]).run(connection)

    for table in tables:
        r.db("test").table(table["name"]).index_create("field0").run(connection)
        r.db("test").table(table["name"]).index_create("field1").run(connection)

    print " Done."



def execute_queries():
    """
    Execute all the queries (inserts/update, reads, terms, delete)
    """
    global results, connection, time_per_query, executions_per_query, constant_queries

    # Execute the insert queries
    print "Running inserts...",
    for table in tables:
        for p in xrange(len(write_queries)):
            docs = []
            num_writes = gen_num_docs(table["size_doc"])
            for i in xrange(num_writes):
                docs.append(gen_doc(table["size_doc"], i))

            start = time.time()

            for i in xrange(num_writes):
                result = eval(write_queries[p]["query"]).run(connection)
                if "generated_keys" in result:
                    table["ids"].append(result["generated_keys"][0])

            results[write_queries[p]["tag"]+"-"+table["name"]] = num_writes/(time.time()-start)
            table["ids"].sort()

    print " Done."


    # Execute the read queries on every tables
    print "Running reads...",
    for table in tables:
        for p in xrange(len(table_queries)):
            start = time.time()
            count = 0
            i = 0
            if "imax" in table_queries[p]:
                max_i = table_queries[p]["imax"]+1
            else:
                max_i = 1

            while (time.time()-start < time_per_query) & (count < executions_per_query):
                cursor = eval(table_queries[p]["query"]).run(connection)
                if isinstance(cursor, r.net.Cursor):
                    list(cursor)
                    cursor.close()

                if i >= len(table["ids"])-max_i:
                    i = 0
                else:
                     i+= 1


                count+=1

            results[table_queries[p]["tag"]+"-"+table["name"]] = count/(time.time()-start)

    print " Done."


    # Execute the queries that do not require a table
    print "Running constant queries...",
    for p in xrange(len(constant_queries)):
        start = time.time()
        count = 0
        while (time.time()-start < time_per_query) & (count < executions_per_query):
            if type(constant_queries[p]) == type(""):
                try:
                    cursor = eval(constant_queries[p]).run(connection)
                    if isinstance(cursor, r.net.Cursor):
                        list(cursor)
                        cursor.close()
                except:
                    print constant_queries[p]
            else:
                cursor = eval(constant_queries[p]["query"]).run(connection)
                if isinstance(cursor, r.net.Cursor):
                    list(cursor)
                    cursor.close()
            count+=1

        if type(constant_queries[p]) == type(""):
            results[constant_queries[p]] = count/(time.time()-start)
        else:
            results[constant_queries[p]["tag"]] = count/(time.time()-start)

    print " Done."


    # Execute the delete queries
    print "Running delete...",
    for table in tables:
        for p in xrange(len(delete_queries)):
            start = time.time()

            num_writes = gen_num_docs(table["size_doc"])
            for i in xrange(num_writes):
                result = eval(delete_queries[p]["query"]).run(connection)

            results[delete_queries[p]["tag"]+"-"+table["name"]] = num_writes/(time.time()-start)

    print " Done."



def stop_cluster(cluster):
    """
    Stop the cluster
    """
    cluster.check_and_stop()

def check_driver():
    """
    Make sure we are using the C++ backend.
    Exit if we don't
    """
    if r.protobuf_implementation == 'py':
        print "Please install the C++ backend for the tests."
        exit(1)


def save_compare_results():
    """
    Save the current results, and if previous results are available, generate an HTML page with the differences
    """
    global results, str_date

    # Save results
    str_date = time.strftime("%y.%m.%d-%H:%M:%S")
    f = open("results/result_"+str_date+".txt", "w")
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
        last_file = file_paths[-2] #The last file is the one we just saved
        f = open(last_file, "r")
        previous_results = json.loads(f.read())
        f.close()


        f = open("comparisons/comparison_"+str_date+".html", "w")
        f.write("<html><head><style>table{padding: 0px; margin: 0px;border-collapse:collapse;}\ntd{border: 1px solid #000; padding: 5px 8px; margin: 0px; text-align: right;}</style></head><body><table>")
        f.write("<tr><td>Query</td><td>Previous q/s</td><td>q/s</td><td>Previous ms/q</td><td>ms/q</td><td>Diff</td><td>Status</td></tr>")
        for key in sorted(results):
            if key in previous_results:
                if results[key] > 0:
                    diff = 1.*(previous_results[key]-results[key])/(results[key])
                else:
                    diff = "undefined"

                if (type(diff) == type(0.)):
                    if(diff < 0.2):
                        status = "Success"
                        color = "green"
                    else:
                        status = "Fail"
                        color = "red"
                else:
                    status = "Bug"
                    color = "gray"
                f.write("<tr><td>"+str(key)[:50]+"</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.4f</td>"%(previous_results[key], results[key], 1000/previous_results[key], 1000/results[key], diff)+"<td style='background: "+str(color)+"'>"+str(status)+"</td></tr>")
            else:
                status = "Unknown"
                color = "gray"

                f.write("<tr><td>"+str(key)[:50]+"</td><td>Unknown</td><td>%.2f</td><td>Unknown</td><td>%.2f</td><td>Unknown</td>"%(results[key], 1000/results[key])+"<td style='background: "+str(color)+"'>"+str(status)+"</td></tr>")

        f.write("</table></body></html>")
        f.close()
    else:
        print "No previous results to compare to."
 


def main():
    """
    Main method
    """
    check_driver()
    run_tests()

if __name__ == "__main__":
    main()

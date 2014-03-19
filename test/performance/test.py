#!/usr/bin/python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys
from sys import stdout, exit, path
import time
import json
import os
import math

from util import gen_doc, gen_num_docs
from queries import constant_queries, table_queries, write_queries, delete_queries

path.insert(0, "../../drivers/python")

from os import environ
import rethinkdb as r

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'rql_test')))
from test_util import RethinkDBTestServers


import pdb

# We define 4 tables (small/normal cache with small/big documents
servers_data = [
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
results = {}
connection = None

def run_tests(build="../../build/release"):
    global connection, servers_data
    for i in range(0, len(servers_data)):
        server_data = servers_data[i]

        print "Starting server with cache_size "+str(server_data["cache_size"])+" MB...",
        sys.stdout.flush()

        with RethinkDBTestServers(1, server_build_dir=build, cache_size=server_data["cache_size"]) as servers:
            print " Done."
            sys.stdout.flush()

            print "Connecting...",
            sys.stdout.flush()

            connection = connect(servers)
            print " Done."
            sys.stdout.flush()

            init_tables()

            # Tests
            execute_read_write_queries(server_data["name"])

            if i == 0:
                execute_constant_queries()

    save_compare_results()


def connect(servers):
    return r.connect(host="localhost", port=servers.driver_port())


def init_tables():
    """
    Create the tables we are going to use
    """
    global connection, tables

    print "Creating databases/tables...",
    sys.stdout.flush()
    try:
        r.db_drop("test").run(connection)
    except r.errors.RqlRuntimeError, e:
        pass

    r.db_create("test").run(connection)

    for table in tables:
        r.db("test").table_create(table["name"]).run(connection)

    for table in tables:
        r.db("test").table(table["name"]).index_create("field0").run(connection)
        r.db("test").table(table["name"]).index_create("field1").run(connection)

    print " Done."
    sys.stdout.flush()



def execute_read_write_queries(suffix):
    """
    Execute all the queries (inserts/update, reads, delete)
    """
    global results, connection, time_per_query, executions_per_query, constant_queries

    print "Running inserts...",
    sys.stdout.flush()
    for table in tables:
        docs = []
        num_writes = gen_num_docs(table["size_doc"])
        for i in xrange(num_writes):
            docs.append(gen_doc(table["size_doc"], i))

        
        i = 0

        start = time.time()
        start_query = time.time()
        durations = []
        while (time.time()-start < time_per_query) & (i < num_writes):
            result = r.db('test').table(table['name']).insert(docs[i]).run(connection)
            if "generated_keys" in result:
                table["ids"].append(result["generated_keys"][0])
            i += 1
            durations.append(time.time()-start_query)
            start_query = time.time()

        durations.sort()
        results["single-inserts-"+table["name"]+"-"+suffix] = {
            "average": (time.time()-start)/i,
            "min": durations[0],
            "max": durations[len(durations)-1],
            "first_centile": durations[int(math.floor(len(durations)/100.*1))],
            "last_centile": durations[int(math.floor(len(durations)/100.*99))]
        }

        # Save it to know how many batch inserts we did
        single_inserts = i

        # Finish inserting the remaining data
        size_batch = 500
        if i < num_writes:
            while i+size_batch < num_writes:
                resutl = r.db('test').table(table['name']).insert(docs[i:i+size_batch]).run(connection)
                table["ids"] += result["generated_keys"]
                i += size_batch

            if i < num_writes:
                result = r.db('test').table(table['name']).insert(docs[i:len(docs)]).run(connection)
                table["ids"] += result["generated_keys"]
        
        if num_writes-single_inserts != 0:
            results["batch-inserts-"+table["name"]+"-"+suffix] = {
                "average": (time.time()-start)/(num_writes-single_inserts),
                "min": durations[0],
                "max": durations[len(durations)-1],
                "first_centile": durations[int(math.floor(len(durations)/100.*1))],
                "last_centile": durations[int(math.floor(len(durations)/100.*99))]
            }

    
        table["ids"].sort()
        
    print " Done."
    sys.stdout.flush()


    # Execute the insert queries
    print "Running update/replace...",
    sys.stdout.flush()
    for table in tables:
        for p in xrange(len(write_queries)):
            docs = []
            num_writes = gen_num_docs(table["size_doc"])
            for i in xrange(num_writes):
                docs.append(gen_doc(table["size_doc"], i))

            i = 0

            start = time.time()
            start_query = time.time()
            durations = []
            while (time.time()-start < time_per_query) & (i < len(table["ids"])):
                eval(write_queries[p]["query"]).run(connection)
                durations.append(time.time()-start_query)
                start_query = time.time()
                i += 1

            durations.sort()
            results[write_queries[p]["tag"]+"-"+table["name"]+"-"+suffix] = {
                "average": (time.time()-start)/i,
                "min": durations[0],
                "max": durations[len(durations)-1],
                "first_centile": durations[int(math.floor(len(durations)/100.*1))],
                "last_centile": durations[int(math.floor(len(durations)/100.*99))]
            }

            i -= 1
            # Clean the update
            eval(write_queries[p]["clean"]).run(connection)

    print " Done."
    sys.stdout.flush()


    # Execute the read queries on every tables
    print "Running reads...",
    sys.stdout.flush()
    for table in tables:
        for p in xrange(len(table_queries)):
            count = 0
            i = 0
            if "imax" in table_queries[p]:
                max_i = table_queries[p]["imax"]+1
            else:
                max_i = 1

            start = time.time()
            start_query = time.time()
            durations = []
            while (time.time()-start < time_per_query) & (count < executions_per_query):
                try:
                    cursor = eval(table_queries[p]["query"]).run(connection)
                    if isinstance(cursor, r.net.Cursor):
                        list(cursor)
                        cursor.close()

                    if i >= len(table["ids"])-max_i:
                        i = 0
                    else:
                         i+= 1
                except:
                    print "Query failed"
                    print constant_queries[p]
                    sys.stdout.flush()
                    break

                count+=1

                durations.append(time.time()-start_query)
                start_query = time.time()

            durations.sort()
            results[table_queries[p]["tag"]+"-"+table["name"]+"-"+suffix] = {
                "average": (time.time()-start)/count,
                "min": durations[0],
                "max": durations[len(durations)-1],
                "first_centile": durations[int(math.floor(len(durations)/100.*1))],
                "last_centile": durations[int(math.floor(len(durations)/100.*99))]
            }


    print " Done."
    sys.stdout.flush()


    # Execute the delete queries
    print "Running delete...",
    sys.stdout.flush()
    for table in tables:
        for p in xrange(len(delete_queries)):
            start = time.time()

            i = 0

            start = time.time()
            start_query = time.time()
            durations = []
            while (time.time()-start < time_per_query) & (i < len(table["ids"])):
                eval(delete_queries[p]["query"]).run(connection)
                i += 1

                durations.append(time.time()-start_query)
                start_query = time.time()


            durations.sort()
            results[delete_queries[p]["tag"]+"-"+table["name"]+"-"+suffix] = {
                "average": (time.time()-start)/i,
                "min": durations[0],
                "max": durations[len(durations)-1],
                "first_centile": durations[int(math.floor(len(durations)/100.*1))],
                "last_centile": durations[int(math.floor(len(durations)/100.*99))]
            }


    print " Done."
    sys.stdout.flush()

def execute_constant_queries():
    global results

    # Execute the queries that do not require a table
    print "Running constant queries...",
    sys.stdout.flush()
    for p in xrange(len(constant_queries)):
        count = 0
        start = time.time()
        start_query = time.time()
        durations = []
        while (time.time()-start < time_per_query) & (count < executions_per_query):
            if type(constant_queries[p]) == type(""):
                try:
                    cursor = eval(constant_queries[p]).run(connection)
                    if isinstance(cursor, r.net.Cursor):
                        list(cursor)
                        cursor.close()
                except:
                    print "Query failed"
                    print constant_queries[p]
                    sys.stdout.flush()
            else:
                cursor = eval(constant_queries[p]["query"]).run(connection)
                if isinstance(cursor, r.net.Cursor):
                    list(cursor)
                    cursor.close()
            count+=1

            durations.append(time.time()-start_query)
            start_query = time.time()



        durations.sort()
        if type(constant_queries[p]) == type(""):
            results[constant_queries[p]] = {
                "average": (time.time()-start)/count,
                "min": durations[0],
                "max": durations[len(durations)-1],
                "first_centile": durations[int(math.floor(len(durations)/100.*1))],
                "last_centile": durations[int(math.floor(len(durations)/100.*99))]
            }
        else:
            results[constant_queries[p]["tag"]] = {
                "average": (time.time()-start)/count,
                "min": durations[0],
                "max": durations[len(durations)-1],
                "first_centile": durations[int(math.floor(len(durations)/100.*1))],
                "last_centile": durations[int(math.floor(len(durations)/100.*99))]
            }



    print " Done."
    sys.stdout.flush()





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
        sys.stdout.flush()
        exit(1)


def save_compare_results():
    """
    Save the current results, and if previous results are available, generate an HTML page with the differences
    """
    global results, str_date

    # Save results
    if not os.path.exists("results"):
        os.makedirs("results")
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
    else:
        previous_results = {}

    if not os.path.exists("comparisons"):
        os.makedirs("comparisons")

    f = open("comparisons/comparison_"+str_date+".html", "w")
    f.write("<html><head><style>table{padding: 0px; margin: 0px;border-collapse:collapse;}\nth{cursor: hand} td, th{border: 1px solid #000; padding: 5px 8px; margin: 0px; text-align: right;}</style><script type='text/javascript' src='jquery-latest.js'></script><script type='text/javascript' src='jquery.tablesorter.js'></script><script type='text/javascript' src='main.js'></script></head><body><table>")
    f.write("<thead><tr><th>Query</th><th>Previous avg q/s</th><th>Avg q/s</th><th>Previous 1st centile q/s</th><th>1st centile q/s</th><th>Previous 99 centile q/s</th><th>99 centile q/s</th><th>Diff</th><th>Status</th></tr></thead><tbody>")
    for key in results:
        if key in previous_results:
            if results[key]["average"] > 0:
                diff = 1.*(previous_results[key]["average"]-results[key]["average"])/(results[key]["average"])
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
            try:
                f.write("<tr><td>"+str(key)[:50]+"</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.2f</td><td>%.4f</td>"%(1/previous_results[key]["average"], 1/results[key]["average"], 1/previous_results[key]["first_centile"], 1/results[key]["first_centile"], 1/previous_results[key]["last_centile"], 1/results[key]["last_centile"], diff)+"<td style='background: "+str(color)+"'>"+str(status)+"</td></tr>")
            except:
                print key

        else:
            status = "Unknown"
            color = "gray"

            try:
                f.write("<tr><td>"+str(key)[:50]+"</td><td>Unknown</td><td>%.2f</td><td>Unknown</td><td>%.2f</td><td>Unknown</td><td>%.2f</td><td>%.4f</td>"%(1/results[key]["average"], 1/results[key]["first_centile"], 1/results[key]["last_centile"], diff)+"<td style='background: "+str(color)+"'>"+str(status)+"</td></tr>")
            except:
                print key


    f.write("</tbody></table></body></html>")
    f.close()


def main():
    """
    Main method
    """
    check_driver()
    run_tests()

if __name__ == "__main__":
    main()

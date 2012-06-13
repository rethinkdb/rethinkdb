#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin
from vcoptparse import *

op = OptParser()
op["mode"] = StringFlag("--mode", "debug")
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path = driver.find_rethinkdb_executable(opts["mode"])
    print "Spinning up two processes..."
    prince_hamlet_files = driver.Files(metacluster, machine_name = "Prince Hamlet", db_path = "prince-hamlet-db", executable_path = executable_path)
    prince_hamlet = driver.Process(cluster, prince_hamlet_files, log_path = "prince-hamlet-log", executable_path = executable_path)
    king_hamlet_files = driver.Files(metacluster, machine_name = "King Hamlet", db_path = "king-hamlet-db", executable_path = executable_path)
    king_hamlet = driver.Process(cluster, king_hamlet_files, log_path = "king-hamlet-log", executable_path = executable_path)
    prince_hamlet.wait_until_started_up()
    king_hamlet.wait_until_started_up()
    cluster.check()
    access = http_admin.ClusterAccess([("localhost", prince_hamlet.http_port)])
    assert access.get_issues() == []
    print "Killing one of them..."
    king_hamlet.close()
    time.sleep(1)
    cluster.check()
    print "Checking that the other has an issue..."
    issues = access.get_issues()
    assert len(issues) == 1
    assert issues[0]["type"] == "MACHINE_DOWN"
    print "Declaring it dead..."
    access.declare_machine_dead(issues[0]["victim"])
    time.sleep(1)
    cluster.check()
    print "Checking that the issue is gone..."
    assert access.get_issues() == []
    print "Bringing it back as a ghost..."
    ghost_of_king_hamlet = driver.Process(cluster, king_hamlet_files, log_path = "king-hamlet-ghost-log", executable_path = executable_path)
    ghost_of_king_hamlet.wait_until_started_up()
    cluster.check()
    print "Checking that there is an issue..."
    issues = access.get_issues()
    assert len(issues) == 1
    assert issues[0]["type"] == "MACHINE_GHOST"
    cluster.check_and_stop()
print "Done."


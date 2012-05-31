#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Spinning up two processes..."
    prince_hamlet_files = driver.Files(metacluster, machine_name = "Prince Hamlet", db_path = "prince-hamlet-db")
    prince_hamlet = driver.Process(cluster, prince_hamlet_files, log_path = "prince-hamlet-log")
    king_hamlet_files = driver.Files(metacluster, machine_name = "King Hamlet", db_path = "king-hamlet-db")
    king_hamlet = driver.Process(cluster, king_hamlet_files, log_path = "king-hamlet-log")
    time.sleep(1)
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
    ghost_of_king_hamlet = driver.Process(cluster, king_hamlet_files, log_path = "king-hamlet-ghost-log")
    time.sleep(1)
    cluster.check()
    print "Checking that there is an issue..."
    issues = access.get_issues()
    assert len(issues) == 1
    assert issues[0]["type"] == "MACHINE_GHOST"
    cluster.check_and_close()
print "Done."


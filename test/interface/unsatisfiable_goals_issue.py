#!/usr/bin/env python
import sys, os, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    print "Spinning up a process..."
    files = driver.Files(metacluster, db_path = "db")
    process = driver.Process(cluster, files, log_path = "log")
    time.sleep(1)
    cluster.check()
    access = http_admin.ClusterAccess([("localhost", process.http_port)])
    assert access.get_issues() == []
    print "Creating a namespace with impossible goals..."
    datacenter = access.add_datacenter()
    namespace = access.add_namespace(primary = datacenter)
    time.sleep(1)
    cluster.check()
    print "Checking that there is an issue about this..."
    issues = access.get_issues()
    assert len(issues) == 1
    assert issues[0]["type"] == "UNSATISFIABLE_GOALS"
    assert issues[0]["namespace_id"] == namespace.uuid
    assert issues[0]["primary_datacenter"] == datacenter.uuid
    cluster.check_and_stop()
print "Done."


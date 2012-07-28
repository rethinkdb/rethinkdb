#!/usr/bin/env python
import sys, os, time, re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    executable_path, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    print "Spinning up cluster..."
    processes = [
        driver.Process(cluster,
                       driver.Files(metacluster,
                                    executable_path = executable_path,
                                    command_prefix = command_prefix),
                       executable_path = executable_path,
                       command_prefix = command_prefix,
                       extra_options = serve_options)
        for i in xrange(4)]
    for process in processes:
        process.wait_until_started_up()
    print "Creating namespace..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for machine_id in http.machines:
        http.move_server_to_datacenter(machine_id, dc)
    ns = http.add_namespace(protocol = "memcached", primary = dc)
    time.sleep(10)
    host, port = driver.get_namespace_host(ns.port, processes)

    access = http

    no_failures = True

    invalid_requests = ["foo=bar","filter=\|","time0ut=1","timeout=","timeout=0"]
    for req in invalid_requests:
        failed=False
        try:
            access.get_stat(req)
        except http_admin.BadServerResponse:
            failed=True
        if not failed:
            print "*** ERROR ***: Request '%s' should have failed!" % req
            no_failures = False
    assert no_failures

    all_stats = access.get_stat("")

    machines = all_stats.keys()
    machines.remove("machines")
    for machine in machines:
        machine_query = "machine_whitelist=%s" % machine
        machine_stats = access.get_stat(machine_query)

        other_machines = [x for x in machines if x != machine]
        other_machines_query="machine_whitelist=%s" % other_machines[0]
        for other_machine in other_machines[1:]:
            other_machines_query += ",%s" % other_machine
        other_machine_stats = access.get_stat(other_machines_query)

        assert(machine_stats.keys().count(machine) == 1)
        assert(other_machine_stats.keys().count(machine) == 0)
        for other_machine in other_machines:
            assert(machine_stats.keys().count(other_machine) == 0)
            assert(other_machine_stats.keys().count(other_machine) == 1)

        filter_query = "filter=.*/parser/cmd_get"
        filtered_machine_query = "%s&%s" % (machine_query, filter_query)
        filtered_machine_stats=access.get_stat(filtered_machine_query)
        stats_top = machine_stats
        for i in [x for x in stats_top.keys() if x != "machines"]:
            machine_top = stats_top[i]
            for (f, namespace_top) in machine_top.items():
                if not re.match('[^-]{8}-', f):
                    machine_top.pop(f)
                else:
                    for (k, parser_top) in namespace_top.items():
                        if k != "parser":
                            namespace_top.pop(k)
                        else:
                            for m in parser_top.keys():
                                if m != "cmd_get":
                                    parser_top.pop(m)
        assert(stats_top == filtered_machine_stats)

    cluster.check_and_stop()
print "Done."

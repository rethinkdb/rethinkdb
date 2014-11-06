#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys, os, time, re
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import driver, http_admin, scenario_common
from vcoptparse import *

op = OptParser()
scenario_common.prepare_option_parser_mode_flags(op)
opts = op.parse(sys.argv)

with driver.Metacluster() as metacluster:
    cluster = driver.Cluster(metacluster)
    _, command_prefix, serve_options = scenario_common.parse_mode_flags(opts)
    
    print "Spinning up cluster..."
    processes = [
        driver.Process(
        	cluster,
            driver.Files(metacluster, console_output="create-output-%d" % i, command_prefix=command_prefix),
            command_prefix=command_prefix, extra_options=serve_options)
        for i in xrange(4)]
    for process in processes:
        process.wait_until_started_up()
    
    print "Creating table..."
    http = http_admin.ClusterAccess([("localhost", p.http_port) for p in processes])
    dc = http.add_datacenter()
    for server_id in http.servers:
        http.move_server_to_datacenter(server_id, dc)
    ns = http.add_table(primary = dc)
    time.sleep(10)
    host, port = driver.get_table_host(processes)
    access = http
    
    # -- check that invalid requests are treated as such
    
    invalid_requests = ["foo=bar", r"filter=\|", "time0ut=1", "timeout=", "timeout=0"]
    for req in invalid_requests:
        failed = False
        try:
            access.get_stat(req)
        except http_admin.BadServerResponse:
            failed = True
        assert failed, "Request '%s' should have failed!" % req
    
    # -- check that the filter option works
    
    all_stats = access.get_stat("")
    servers = all_stats.keys()
    servers.remove("servers")
    for server in servers:
        
        server_query = "server_whitelist=%s" % server
        server_stats = access.get_stat(server_query)
        
        filter_query = "filter=.*/serializers/serializer"
        filtered_server_query = "%s&%s" % (server_query, filter_query)
        filtered_server_stats = access.get_stat(filtered_server_query)
        
        other_servers = [x for x in servers if x != server]
        other_servers_query = "server_whitelist=" + ",".join(other_servers)
        other_server_stats = access.get_stat(other_servers_query)

        assert server_stats.keys().count(server) == 1
        assert other_server_stats.keys().count(server) == 0
        for other_server in other_servers:
            assert server_stats.keys().count(other_server) == 0
            assert other_server_stats.keys().count(other_server) == 1

        stats_top = server_stats
        for i in [x for x in stats_top.keys() if x != "servers"]:
            server_top = stats_top[i]
            for f, namespace_top in server_top.items():
                if not re.match('[^-]{8}-', f):
                    server_top.pop(f)
                else:
                    for k, parser_top in namespace_top.items():
                        if k != "serializers":
                            namespace_top.pop(k)
                        else:
                            for m in parser_top.keys():
                                if m != "serializer":
                                    parser_top.pop(m)
        assert stats_top == filtered_server_stats, "the filter (%s) does not work: %s does not equal %s" % (filter_query, stats_top, filtered_server_stats)

    cluster.check_and_stop()
print "Done."

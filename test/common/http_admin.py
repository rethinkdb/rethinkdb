import os
import re
import json
import copy
import time
import socket
import random
from httplib import HTTPConnection

""" The `http_admin.py` module is a Python wrapper around the HTTP interface to
RethinkDB. It is not responsible for starting and stopping RethinkDB processes;
use the `driver.py` module for that, or just start the processes manually.

Things currently not very well supported
 - Blueprints - proposals and checking of blueprint data is not implemented
 - Renaming things - Machines, Datacenters, etc, may be renamed through the cluster, but not yet by this script
 - Value conflicts - if a value conflict arises due to a split cluster (or some other method), most operations
    will fail until the conflict is resolved
"""

def validate_uuid(json_uuid):
    assert isinstance(json_uuid, str) or isinstance(json_uuid, unicode)
    assert json_uuid.count("-") == 4
    assert len(json_uuid) == 36
    return json_uuid

def is_uuid(json_uuid):
    try:
        validate_uuid(json_uuid)
        return True
    except AssertionError:
        return False

class BadClusterData(StandardError):
    def __init__(self, expected, actual):
        self.expected = expected
        self.actual = actual
    def __str__(self):
        return "Cluster is inconsistent between nodes\nexpected: " + str(self.expected) + "\nactual: " + str(self.actual)

class BadServerResponse(StandardError):
    def __init__(self, status, reason):
        self.status = status
        self.reason = reason
    def __str__(self):
        return "Server returned error code: %d %s" % (self.status, self.reason)

class ValueConflict(object):
    def __init__(self, target, field, resolve_data):
        self.target = target
        self.field = field
        self.values = [ ]
        for value_data in resolve_data:
            self.values.append(value_data[1])
        print self.values

    def __str__(self):
        values = ""
        for value in self.values:
            values += ", \"%s\"" % value
        return "Value conflict on field %s with possible values%s" % (self.field, values)

class Datacenter(object):
    def __init__(self, uuid, json_data):
        self.uuid = uuid
        self.name = json_data[u"name"]

    def check(self, data):
        return data[u"name"] == self.name

    def to_json(self):
        return { u"name": self.name }

    def __str__(self):
        return "Datacenter(name:%s)" % (self.name)

class Blueprint(object):
    def __init__(self, json_data):
        self.peers_roles = json_data[u"peers_roles"]

    def to_json(self):
        return { u"peers_roles": self.peers_roles }

    def __str__(self):
        return "Blueprint()"

class Namespace(object):
    def __init__(self, uuid, json_data):
        self.uuid = validate_uuid(uuid)
        self.blueprint = Blueprint(json_data[u"blueprint"])
        self.primary_uuid = None if json_data[u"primary_uuid"] is None else validate_uuid(json_data[u"primary_uuid"])
        self.replica_affinities = json_data[u"replica_affinities"]
        self.ack_expectations = json_data[u"ack_expectations"]
        self.shards = self.parse_shards(json_data[u"shards"])
        self.name = json_data[u"name"]
        self.port = json_data[u"port"]
        self.primary_pinnings = json_data[u"primary_pinnings"]
        self.secondary_pinnings = json_data[u"secondary_pinnings"]

    def check(self, data):
        return data[u"name"] == self.name and \
            data[u"primary_uuid"] == self.primary_uuid and \
            data[u"replica_affinities"] == self.replica_affinities and \
            data[u"ack_expectations"] == self.ack_expectations and \
            self.parse_shards(data[u"shards"]) == self.shards and \
            data[u"port"] == self.port and \
            data[u"primary_pinnings"] == self.primary_pinnings and \
            data[u"secondary_pinnings"] == self.secondary_pinnings

    def to_json(self):
        return {
            unicode("blueprint"): self.blueprint.to_json(),
            unicode("name"): self.name,
            unicode("primary_uuid"): self.primary_uuid,
            unicode("replica_affinities"): self.replica_affinities,
            unicode("ack_expectations"): self.ack_expectations,
            unicode("shards"): self.shards_to_json(),
            unicode("port"): self.port,
            unicode("primary_pinnings"): self.primary_pinnings,
            unicode("secondary_pinnings"): self.secondary_pinnings
            }

    def __str__(self):
        affinities = ""
        if len(self.replica_affinities) == 0:
            affinities = "None, "
        else:
            for uuid, count in self.replica_affinities.iteritems():
                affinities += uuid + "=" + str(count) + ", "
        if len(self.replica_affinities) == 0:
            shards = "None, "
        else:
            for uuid, count in self.replica_affinities.iteritems():
                shards += uuid + "=" + str(count) + ", "
        return "Namespace(name:%s, port:%d, primary:%s, affinities:%sprimary pinnings:%s, secondary_pinnings:%s, shard boundaries:%s, blueprint:NYI)" % (self.name, self.port, self.primary_uuid, affinities, self.primary_pinnings, self.secondary_pinnings, self.shards)

class DummyNamespace(Namespace):
    def __init__(self, uuid, json_data):
        Namespace.__init__(self, uuid, json_data)

    def shards_to_json(self):
        return self.shards

    def parse_shards(self, subset_strings):
        subsets = [ ]
        superset = set()
        for subset_string in subset_strings:
            subset = u""
            for c in subset_string:
                if c in u"{}, ":
                    pass
                elif c in u"abcdefghijklmnopqrstuvwxyz":
                    assert c not in superset
                    superset.add(c)
                    subset += c
                else:
                    raise RuntimeError("Invalid value in DummyNamespace shard set")
            assert len(subset) != 0
            subsets.append(subset)
        return subsets

    def add_shard(self, new_subset):
        # The shard is a string of characters, a-z, which must be unique across all shards
        if isinstance(new_subset, str):
            new_subset = unicode(new_subset)
        assert isinstance(new_subset, unicode)
        for i in range(len(self.shards)):
            for c in new_subset:
                if c in self.shards[i]:
                    self.shards[i]= self.shards[i].replace(c, u"")
        self.shards.append(new_subset)

    def remove_shard(self, subset):
        if isinstance(subset, str):
            subset = unicode(subset)
        assert isinstance(subset, unicode)
        assert subset in self.shards
        assert len(self.shards) > 0
        self.shards.remove(subset)
        self.shards[0] += subset # Throw the old shard into the first one

    def __str__(self):
        return "Dummy" + Namespace.__str__(self)

class MemcachedNamespace(Namespace):
    def __init__(self, uuid, json_data):
        Namespace.__init__(self, uuid, json_data)

    def shards_to_json(self):
        # Build the ridiculously formatted shard data
        shard_json = []
        last_split = u""
        for split in self.shards:
            shard_json.append(u"[\"%s\", \"%s\"]" % (last_split, split))
            last_split = split
        shard_json.append(u"[\"%s\", null]" % (last_split))
        return shard_json

    def parse_shards(self, shards):
        # Build the ridiculously formatted shard data
        splits = [ ]
        last_split = u""
        matches = None
        for shard in shards:
            matches = re.match(u"^\[\"(\w*)\", \"(\w*)\"\]$|^\[\"(\w*)\", null\]$", shard)
            assert matches is not None
            if matches.group(3) is None:
                assert matches.group(1) == last_split
                splits.append(matches.group(2))
                last_split = matches.group(2)
            else:
                assert matches.group(3) == last_split
        if matches is not None:
            assert matches.group(3) is not None
        assert sorted(splits) == splits
        return splits

    def add_shard(self, split_point):
        if isinstance(split_point, str):
            split_point = unicode(split_point)
        assert split_point not in self.shards
        self.shards.append(split_point)
        self.shards.sort()

    def remove_shard(self, split_point):
        if isinstance(split_point, str):
            split_point = unicode(split_point)
        assert split_point in self.shards
        self.shards.remove(split_point)

    def __str__(self):
        return "Memcached" + Namespace.__str__(self)

class Server(object):
    def __init__(self, host, http_port):
        self.host = host
        self.http_port = http_port
        self.uuid = validate_uuid(self.do_query("GET", "/ajax/me"))
        serv_info = self.do_query("GET", "/ajax/machines/" + self.uuid)
        self.datacenter_uuid = serv_info[u"datacenter_uuid"]
        self.name = serv_info[u"name"]
        self.port_offset = serv_info[u"port_offset"]

    def check(self, data):
        return data[u"datacenter_uuid"] == self.datacenter_uuid and data[u"name"] == self.name and data[u"port_offset"] == self.port_offset

    def to_json(self):
        return { u"datacenter_uuid": self.datacenter_uuid, u"name": self.name, u"port_offset": self.port_offset }

    def do_query(self, method, route, payload = None):
        conn = HTTPConnection(self.host, self.http_port)
        conn.connect()
        if payload is not None:
            conn.request(method, route, json.dumps(payload))
        else:
            conn.request(method, route)
        response = conn.getresponse()
        if response.status == 200:
            return json.loads(response.read())
        else:
            raise BadServerResponse(response.status, response.reason)

    def __str__(self):
        return "Server(%s:%d, name:%s, datacenter:%s, port_offset:%d)" % (self.host, self.http_port, self.name, self.datacenter_uuid, self.port_offset)

class ClusterAccess(object):
    def __init__(self, addresses = []):
        self.machines = { }
        self.datacenters = { }
        self.dummy_namespaces = { }
        self.memcached_namespaces = { }
        self.conflicts = [ ]

        for host, port in addresses:
            self.add_existing_machine((host, port))

    def __str__(self):
        retval = "Machines:"
        for i in self.machines.iterkeys():
            retval += "\n%s: %s" % (i, self.machines[i])
        retval += "\nDatacenters:"
        for i in self.datacenters.iterkeys():
            retval += "\n%s: %s" % (i, self.datacenters[i])
        retval += "\nNamespaces:"
        for i in self.dummy_namespaces.iterkeys():
            retval += "\n%s: %s" % (i, self.dummy_namespaces[i])
        for i in self.memcached_namespaces.iterkeys():
            retval += "\n%s: %s" % (i, self.memcached_namespaces[i])
        return retval

    def print_machines(self):
        for i in self.machines.iterkeys():
            print "%s: %s" % (i, self.machines[i])

    def print_namespaces(self):
        for i in self.dummy_namespaces.iterkeys():
            print "%s: %s" % (i, self.dummy_namespaces[i])
        for i in self.memcached_namespaces.iterkeys():
            print "%s: %s" % (i, self.memcached_namespaces[i])

    def print_datacenters(self):
        for i in self.datacenters.iterkeys():
            print "%s: %s" % (i, self.datacenters[i])

    def _get_server_for_command(self, servid = None):
        if servid is None:
            for serv in self.machines.itervalues():
                return serv
        else:
            return self.machines[servid]

    def add_existing_machine(self, (host, http_port)):
        serv = Server(host, http_port)
        self.machines[serv.uuid] = serv
        self.update_cluster_data()
        return serv

    def add_datacenter(self, name = None, servid = None):
        if name is None:
            name = str(random.randint(0, 1000000))
        info = self._get_server_for_command(servid).do_query("POST", "/ajax/datacenters/new", {
            "name": name
            })
        time.sleep(0.2) # Give some time for changes to hit the rest of the cluster
        assert len(info) == 1
        uuid, json_data = next(info.iteritems())
        datacenter = Datacenter(uuid, json_data)
        self.datacenters[datacenter.uuid] = datacenter
        self.update_cluster_data()
        return datacenter

    def _find_thing(self, what, type_class, type_str, search_space):
        if isinstance(what, (str, unicode)):
            if is_uuid(what):
                return search_space[what]
            else:
                hits = [x for x in search_space.values() if x.name == what]
                if len(hits) == 0:
                    raise ValueError("No %s named %r" % (type_str, what))
                elif len(hits) == 1:
                    return hits[0]
                else:
                    raise ValueError("Multiple %ss named %r" % (type_str, what))
        elif isinstance(what, type_class):
            assert search_space[what.uuid] is what
            return what
        else:
            raise TypeError("Can't interpret %r as a %s" % (what, type_str))

    def find_machine(self, what):
        return self._find_thing(what, Server, "machine", self.machines)

    def find_datacenter(self, what):
        return self._find_thing(what, Datacenter, "data center", self.datacenters)

    def find_namespace(self, what):
        nss = {}
        nss.update(self.memcached_namespaces)
        nss.update(self.dummy_namespaces)
        return self._find_thing(what, Namespace, "namespace", nss)

    def get_directory(self, servid = None):
        return self._get_server_for_command(servid).do_query("GET", "/ajax/directory")

    def move_server_to_datacenter(self, serv, datacenter, servid = None):
        serv = self.find_machine(serv)
        datacenter = self.find_datacenter(datacenter)
        serv.datacenter_uuid = datacenter.uuid
        self._get_server_for_command(servid).do_query("POST", "/ajax/machines/" + serv.uuid + "/datacenter_uuid", datacenter.uuid)
        time.sleep(0.2) # Give some time for changes to hit the rest of the cluster
        self.update_cluster_data()

    def move_namespace_to_datacenter(self, namespace, primary, servid = None):
        namespace = self.find_namespace(namespace)
        primary = None if primary is None else self.find_datacenter(primary)
        namespace.primary_uuid = primary.uuid
        if isinstance(namespace, MemcachedNamespace):
            self._get_server_for_command(servid).do_query("POST", "/ajax/memcached_namespaces/" + namespace.uuid, namespace.to_json())
        elif isinstance(namespace, DummyNamespace):
            self._get_server_for_command(servid).do_query("POST", "/ajax/dummy_namespaces/" + namespace.uuid, namespace.to_json())
        time.sleep(0.2) # Give some time for the changes to hit the rest of the cluster
        self.update_cluster_data()

    def set_namespace_affinities(self, namespace, affinities = { }, servid = None):
        namespace = self.find_namespace(namespace)
        aff_dict = { }
        for datacenter, count in affinities.iteritems():
            aff_dict[self.find_datacenter(datacenter).uuid] = count
        namespace.replica_affinities = aff_dict
        if isinstance(namespace, MemcachedNamespace):
            self._get_server_for_command(servid).do_query("POST", "/ajax/memcached_namespaces/" + namespace.uuid, namespace.to_json())
        elif isinstance(namespace, DummyNamespace):
            self._get_server_for_command(servid).do_query("POST", "/ajax/dummy_namespaces/" + namespace.uuid, namespace.to_json())
        time.sleep(0.2) # Give some time for the changes to hit the rest of the cluster
        self.update_cluster_data()
        return namespace

    def add_namespace(self, protocol = "memcached", name = None, port = None, primary = None, affinities = { }, servid = None):
        if port is None:
            port = random.randint(20000, 60000)
        if name is None:
            name = str(random.randint(0, 1000000))
        if primary is not None:
            primary = self.find_datacenter(primary).uuid
        else:
            primary = random.choice(self.datacenters.keys())
        aff_dict = { }
        for datacenter, count in affinities.iteritems():
            aff_dict[self.find_datacenter(datacenter).uuid] = count
        info = self._get_server_for_command(servid).do_query("POST", "/ajax/%s_namespaces/new" % protocol, {
            "name": name,
            "port": port,
            "primary_uuid": primary,
            "replica_affinities": aff_dict
            })
        time.sleep(0.2) # Give some time for changes to hit the rest of the cluster
        assert len(info) == 1
        uuid, json_data = next(info.iteritems())
        type_class = {"memcached": MemcachedNamespace, "dummy": DummyNamespace}[protocol]
        namespace = type_class(uuid, json_data)
        getattr(self, "%s_namespaces" % protocol)[namespace.uuid] = namespace
        self.update_cluster_data()
        return namespace

    def rename(self, target, name, servid = None):
        type_targets = { MemcachedNamespace: self.memcached_namespaces, DummyNamespace: self.dummy_namespaces, InternalServer: self.machines, ExternalServer: self.machines, Datacenter: self.datacenters }
        type_objects = { MemcachedNamespace: "memcached_namespaces", DummyNamespace: "dummy_namespaces", InternalServer: "machines", ExternalServer: "machines", Datacenter: "datacenters" }
        assert type_targets[type(target)][target.uuid] is target
        object_type = type_objects[type(target)]
        target.name = name
        info = self._get_server_for_command(servid).do_query("POST", "/ajax/%s/%s/name" % (object_type, target.uuid), name)
        time.sleep(0.2)
        self.update_cluster_data()

    def get_conflicts(self):
        return self.conflicts

    def resolve_conflict(self, conflict, value, servid = None):
        assert conflict in self.conflicts
        assert value in conflict.values
        type_targets = { MemcachedNamespace: self.memcached_namespaces, DummyNamespace: self.dummy_namespaces, InternalServer: self.machines, ExternalServer: self.machines, Datacenter: self.datacenters }
        type_objects = { MemcachedNamespace: "memcached_namespaces", DummyNamespace: "dummy_namespaces", InternalServer: "machines", ExternalServer: "machines", Datacenter: "datacenters" }
        assert type_targets[type(conflict.target)][conflict.target.uuid] is conflict.target
        object_type = type_objects[type(conflict.target)]
        info = self._get_server_for_command(servid).do_query("POST", "/ajax/%s/%s/%s/resolve" % (object_type, conflict.target.uuid, conflict.field), value)
        # Remove the conflict and update the field in the target 
        self.conflicts.remove(conflict)
        setattr(conflict.target, conflict.field, value) # TODO: this probably won't work for certain things like shards that we represent differently locally than the strict json format
        time.sleep(0.2)
        self.update_cluster_data()

    def add_namespace_shard(self, namespace, split_point, servid = None):
        type_namespaces = { MemcachedNamespace: self.memcached_namespaces, DummyNamespace: self.dummy_namespaces }
        type_protocols = { MemcachedNamespace: "memcached", DummyNamespace: "dummy" }
        assert type_namespaces[type(namespace)][namespace.uuid] is namespace
        protocol = type_protocols[type(namespace)]
        namespace.add_shard(split_point)
        info = self._get_server_for_command(servid).do_query("POST", "/ajax/%s_namespaces/%s/shards" % (protocol, namespace.uuid), namespace.shards_to_json())
        time.sleep(0.2)
        self.update_cluster_data()

    def remove_namespace_shard(self, namespace, split_point, servid = None):
        type_namespaces = { MemcachedNamespace: self.memcached_namespaces, DummyNamespace: self.dummy_namespaces }
        type_protocols = { MemcachedNamespace: "memcached", DummyNamespace: "dummy" }
        assert type_namespaces[type(namespace)][namespace.uuid] is namespace
        protocol = type_protocols[type(namespace)]
        namespace.remove_shard(split_point)
        info = self._get_server_for_command(servid).do_query("POST", "/ajax/%s_namespaces/%s/shards" % (protocol, namespace.uuid), namespace.shards_to_json())
        time.sleep(0.2)
        self.update_cluster_data()

    def compute_port(self, namespace, machine):
        namespace = self.find_namespace(namespace)
        machine = self.find_machine(machine)
        return namespace.port + machine.port_offset

    def get_namespace_host(self, namespace, selector = None):
        # selector may be a specific machine or datacenter to use, none will take any
        type_namespaces = { MemcachedNamespace: self.memcached_namespaces, DummyNamespace: self.dummy_namespaces }
        assert type_namespaces[type(namespace)][namespace.uuid] is namespace
        if selector is None:
            machines = [ ]
            for serv in self.machines.itervalues():
                machines.append(serv)
            machine = random.choice(machines)
        elif isinstance(selector, Datacenter):
            # Take any machine from the specified datacenter
            machine = self.get_machine_in_datacenter(selector)
        elif isinstance(selector, Server):
            # Use the given server directly
            machine = selector

        return (machine.host, self.compute_port(namespace, machine))

    def get_datacenter_in_namespace(self, namespace, primary = None):
        type_namespaces = { MemcachedNamespace: self.memcached_namespaces, DummyNamespace: self.dummy_namespaces }
        assert type_namespaces[type(namespace)][namespace.uuid] is namespace
        if primary is not None:
            return self.datacenters[namespace.primary_uuid]

        # Build a list of datacenters in the given namespace
        datacenters = [ self.datacenters[namespace.primary_uuid] ]
        for uuid in namespace.replica_affinities.iterkeys():
            datacenters.append(self.datacenters[uuid])
        return random.choice(datacenters)

    def get_machine_in_datacenter(self, datacenter):
        assert self.datacenters[datacenter.uuid] is datacenter
        # Build a list of machines in the given datacenter
        machines = [ ]
        for serv in self.machines.itervalues():
            if serv.datacenter_uuid == datacenter.uuid:
                machines.append(serv)
        return random.choice(machines)

    def _pull_cluster_data(self, cluster_data, local_data, data_type):
        for uuid in cluster_data.iterkeys():
            validate_uuid(uuid)
            if uuid not in local_data:
                local_data[uuid] = data_type(uuid, cluster_data[uuid])
        assert len(cluster_data) == len(local_data)

    # Get the list of machines/namespaces from the cluster, verify that it is consistent across each machine
    def _verify_consistent_cluster(self):
        expected = self._get_server_for_command().do_query("GET", "/ajax")
        # Filter out the "me" value - it will be different on each machine
        assert expected.pop("me") is not None
        for i in self.machines.iterkeys():
            actual = self.machines[i].do_query("GET", "/ajax")
            assert actual.pop("me") == self.machines[i].uuid
            if actual != expected:
                raise BadClusterData(expected, actual)
        return expected

    def _verify_cluster_data_chunk(self, local, remote):
        for uuid, obj in local.iteritems():
            check_obj = True
            for field, value in remote[uuid].iteritems():
                if value == u"VALUE_IN_CONFLICT":
                    if obj not in self.conflicts:
                        # Get the possible values and create a value conflict object
                        type_objects = { MemcachedNamespace: "memcached_namespaces", DummyNamespace: "dummy_namespaces", InternalServer: "machines", ExternalServer: "machines", Datacenter: "datacenters" }
                        object_type = type_objects[type(obj)]
                        resolve_data = self._get_server_for_command().do_query("GET", "/ajax/%s/%s/%s/resolve" % (object_type, obj.uuid, field))
                        self.conflicts.append(ValueConflict(obj, field, resolve_data))
                    print "Warning: value conflict"
                    check_obj = False
                    
            if check_obj and not obj.check(remote[uuid]):
                raise ValueError("inconsistent cluster data: %r != %r" % (obj.to_json(), remote[uuid]))

    # Check the data from the server against our data
    def _verify_cluster_data(self, data):
        self._verify_cluster_data_chunk(self.machines, data[u"machines"])
        self._verify_cluster_data_chunk(self.datacenters, data[u"datacenters"])
        self._verify_cluster_data_chunk(self.dummy_namespaces, data[u"dummy_namespaces"])
        self._verify_cluster_data_chunk(self.memcached_namespaces, data[u"memcached_namespaces"])

    def update_cluster_data(self):
        data = self._verify_consistent_cluster()
        self._pull_cluster_data(data[u"machines"], self.machines, DummyServer)
        self._pull_cluster_data(data[u"datacenters"], self.datacenters, Datacenter)
        self._pull_cluster_data(data[u"dummy_namespaces"], self.dummy_namespaces, DummyNamespace)
        self._pull_cluster_data(data[u"memcached_namespaces"], self.memcached_namespaces, MemcachedNamespace)
        self._verify_cluster_data(data)
        return data

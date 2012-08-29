import os
import re
import json
import copy
import time
import socket
import random
from httplib import HTTPConnection
import urllib   # for `quote()` and `unquote()`

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
        self.uuid = validate_uuid(uuid)
        self.name = json_data[u"name"]

    def check(self, data):
        return data[u"name"] == self.name

    def to_json(self):
        return { u"name": self.name }

    def __str__(self):
        return "Datacenter(name:%s)" % (self.name)

class Database(object):
    def __init__(self, uuid, json_data):
        self.uuid = validate_uuid(uuid)
        self.name = json_data[u"name"]

    def check(self, data):
        return data[u"name"] == self.name

    def to_json(self):
        return { u"name": self.name }

    def __str__(self):
        return "Database(name:%s)" % (self.name)

class Blueprint(object):
    def __init__(self, json_data):
        self.peers_roles = json_data[u"peers_roles"]

    def to_json(self):
        return { u"peers_roles": self.peers_roles }

    def __str__(self):
        return "Blueprint(%r)" % self.to_json()

class Namespace(object):
    def __init__(self, uuid, json_data, protocol):
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
        self.database_uuid = None if json_data[u"database"] is None else validate_uuid(json_data[u"database"])
        self.protocol = protocol

    def check(self, data):
        return data[u"name"] == self.name and \
            data[u"primary_uuid"] == self.primary_uuid and \
            data[u"replica_affinities"] == self.replica_affinities and \
            data[u"ack_expectations"] == self.ack_expectations and \
            self.parse_shards(data[u"shards"]) == self.shards and \
            data[u"port"] == self.port and \
            data[u"primary_pinnings"] == self.primary_pinnings and \
            data[u"secondary_pinnings"] == self.secondary_pinnings and \
            data[u"database"] == self.database_uuid

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
            unicode("secondary_pinnings"): self.secondary_pinnings,
            unicode("database"): self.database
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
        return "Namespace(name:%s, port:%d, primary:%s, affinities:%sprimary pinnings:%s, secondary_pinnings:%s, shard boundaries:%s, blueprint:NYI, database:%s)" % (self.name, self.port, self.primary_uuid, affinities, self.primary_pinnings, self.secondary_pinnings, self.shards, self.database_uuid)

class DummyNamespace(Namespace):
    def __init__(self, uuid, json_data):
        Namespace.__init__(self, uuid, json_data, "dummy")

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
        Namespace.__init__(self, uuid, json_data, "memcached")

    def shards_to_json(self):
        # Build the ridiculously formatted shard data
        shard_json = []
        last_split = u""
        for split in self.shards:
            shard_json.append(json.dumps([urllib.quote(last_split), urllib.quote(split)]))
            last_split = split
        shard_json.append(json.dumps([urllib.quote(last_split), None]))
        return shard_json

    def parse_shards(self, shards):
        # Build the ridiculously formatted shard data
        splits = [ ]
        last_split = u""
        matches = None
        parsed_shards = [ ]
        for shard in shards:
            left, right = json.loads(shard)
            assert isinstance(left, basestring)
            assert right is None or isinstance(right, basestring)
            parsed_shards.append((urllib.unquote(left), urllib.unquote(right) if right is not None else None))
        parsed_shards.sort()
        last_split = u""
        for left, right in parsed_shards:
            assert left == last_split
            if right is not None:
                splits.append(right)
            last_split = right
        assert last_split is None
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

class RDBNamespace(Namespace):
    def __init__(self, uuid, json_data):
        Namespace.__init__(self, uuid, json_data, "rdb")

    def shards_to_json(self):
        # Build the ridiculously formatted shard data
        shard_json = []
        last_split = u""
        for split in self.shards:
            shard_json.append(json.dumps([urllib.quote(last_split), urllib.quote(split)]))
            last_split = split
        shard_json.append(json.dumps([urllib.quote(last_split), None]))
        return shard_json

    def parse_shards(self, shards):
        # Build the ridiculously formatted shard data
        splits = [ ]
        last_split = u""
        matches = None
        parsed_shards = [ ]
        for shard in shards:
            left, right = json.loads(shard)
            assert isinstance(left, basestring)
            assert right is None or isinstance(right, basestring)
            parsed_shards.append((urllib.unquote(left), urllib.unquote(right) if right is not None else None))
        parsed_shards.sort()
        last_split = u""
        for left, right in parsed_shards:
            assert left == last_split
            if right is not None:
                splits.append(right)
            last_split = right
        assert last_split is None
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
        return "RDB" + Namespace.__str__(self)

namespace_classes_by_protocol_name = {
    "dummy": DummyNamespace,
    "memcached": MemcachedNamespace,
    "rdb": RDBNamespace
    }

class Machine(object):
    def __init__(self, uuid, json_data):
        self.uuid = validate_uuid(uuid)
        self.datacenter_uuid = json_data[u"datacenter_uuid"]
        self.name = json_data[u"name"]

    def check(self, data):
        return data[u"datacenter_uuid"] == self.datacenter_uuid and data[u"name"] == self.name

    def to_json(self):
        return { u"datacenter_uuid": self.datacenter_uuid, u"name": self.name }

    def __str__(self):
        return "Server(uuid:%s, name:%s, datacenter:%s)" % (self.uuid, self.name, self.datacenter_uuid)

class ClusterAccess(object):
    def __init__(self, addresses = []):
        for host, http_port in addresses:
            assert isinstance(host, str)
            assert isinstance(http_port, int)
        self.addresses = addresses

        self.machines = { }
        self.datacenters = { }
        self.dummy_namespaces = { }
        self.memcached_namespaces = { }
        self.rdb_namespaces = { }
        self.databases = { }
        self.conflicts = [ ]

        self.update_cluster_data(0)

    def do_query(self, method, route, payload = None):
        host, http_port = random.choice(self.addresses)
        return self.do_query_specific(host, http_port, method, route, payload)

    def do_query_plaintext(self, method, route, payload = None):
        host, http_port = random.choice(self.addresses)
        return self.do_query_specific_plaintext(host, http_port, method, route, payload)

    def do_query_specific(self, host, http_port, method, route, payload = None):
        if payload is not None:
            payload = json.dumps(payload)
            headers = {"Content-Type": "application/json"}
        else:
            headers = {}
        return json.loads(self.do_query_specific_plaintext(host, http_port, method, route, payload, headers))

    def do_query_specific_plaintext(self, host, http_port, method, route, payload = None, headers = {}):
        conn = HTTPConnection(host, http_port, timeout = 120)
        conn.connect()
        if payload is not None:
            conn.request(method, route, payload, headers)
        else:
            conn.request(method, route, "", headers)
        response = conn.getresponse()
        if response.status == 200:
            return response.read()
        else:
            raise BadServerResponse(response.status, response.reason)

    def __str__(self):
        retval = "Machines:"
        for i in self.machines.iterkeys():
            retval += "\n%s: %s" % (i, self.machines[i])
        retval += "\nDatacenters:"
        for i in self.datacenters.iterkeys():
            retval += "\n%s: %s" % (i, self.datacenters[i])
        retval += "\nDatabases:"
        for i in self.datagbases.iterkeys():
            retval += "\n%s: %s" % (i, self.databases[i])
        retval += "\nNamespaces:"
        for i in self.dummy_namespaces.iterkeys():
            retval += "\n%s: %s" % (i, self.dummy_namespaces[i])
        for i in self.memcached_namespaces.iterkeys():
            retval += "\n%s: %s" % (i, self.memcached_namespaces[i])
        for i in self.rdb_namespaces.iterkeys():
            retval += "\n%s: %s" % (i, self.rdb_namespaces[i])
        return retval

    def print_machines(self):
        for i in self.machines.iterkeys():
            print "%s: %s" % (i, self.machines[i])

    def print_namespaces(self):
        for i in self.dummy_namespaces.iterkeys():
            print "%s: %s" % (i, self.dummy_namespaces[i])
        for i in self.memcached_namespaces.iterkeys():
            print "%s: %s" % (i, self.memcached_namespaces[i])
        for i in self.rdb_namespaces.iterkeys():
            print "%s: %s" % (i, self.rdb_namespaces[i])

    def print_datacenters(self):
        for i in self.datacenters.iterkeys():
            print "%s: %s" % (i, self.datacenters[i])

    def add_datacenter(self, name = None):
        if name is None:
            name = str(random.randint(0, 1000000))
        info = self.do_query("POST", "/ajax/semilattice/datacenters/new", {
            "name": name
            })
        assert len(info) == 1
        uuid, json_data = next(info.iteritems())
        datacenter = Datacenter(uuid, json_data)
        self.datacenters[datacenter.uuid] = datacenter
        self.update_cluster_data(10)
        return datacenter

    def add_database(self, name = None):
        if name is None:
            name = str(random.randint(0, 1000000))
        info = self.do_query("POST", "/ajax/semilattice/databases/new", {
            "name": name
            })
        assert len(info) == 1
        uuid, json_data = next(info.iteritems())
        database = Database(uuid, json_data)
        self.databases[database.uuid] = database
        self.update_cluster_data(10)
        return database

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
        return self._find_thing(what, Machine, "machine", self.machines)

    def find_datacenter(self, what):
        return self._find_thing(what, Datacenter, "data center", self.datacenters)

    def find_database(self, what):
        return self._find_thing(what, Database, "data base", self.databases)

    def find_namespace(self, what):
        nss = {}
        nss.update(self.dummy_namespaces)
        nss.update(self.memcached_namespaces)
        nss.update(self.rdb_namespaces)
        return self._find_thing(what, Namespace, "namespace", nss)

    def get_directory(self):
        return self.do_query("GET", "/ajax/directory")

    def get_log(self, machine_id, max_length = 100):
        log = self.do_query("GET", "/ajax/log/%s?max_length=%d" % (machine_id, max_length))[machine_id]
        if isinstance(log, basestring):
            raise BadServerResponse(log)
        assert isinstance(log, list)
        return log

    def get_stat(self, query):
        return self.do_query("GET", "/ajax/stat?%s" % query)

    def declare_machine_dead(self, machine):
        machine = self.find_machine(machine)
        del self.machines[machine.uuid]
        self.do_query("DELETE", "/ajax/semilattice/machines/" + machine.uuid)
        self.update_cluster_data(10)

    def move_server_to_datacenter(self, serv, datacenter):
        serv = self.find_machine(serv)
        datacenter = self.find_datacenter(datacenter)
        serv.datacenter_uuid = datacenter.uuid
        self.do_query("POST", "/ajax/semilattice/machines/" + serv.uuid + "/datacenter_uuid", datacenter.uuid)
        self.update_cluster_data(10)

    def move_namespace_to_datacenter(self, namespace, primary):
        namespace = self.find_namespace(namespace)
        primary = None if primary is None else self.find_datacenter(primary)
        namespace.primary_uuid = primary.uuid
        self.do_query("POST", "/ajax/semilattice/%s_namespaces/%s/primary_uuid" % (namespace.protocol, namespace.uuid), primary.uuid)
        self.update_cluster_data(10)

    def set_namespace_affinities(self, namespace, affinities = { }):
        namespace = self.find_namespace(namespace)
        aff_dict = { }
        for datacenter, count in affinities.iteritems():
            aff_dict[self.find_datacenter(datacenter).uuid] = count
        namespace.replica_affinities.update(aff_dict)
        self.do_query("POST", "/ajax/semilattice/%s_namespaces/%s/replica_affinities" % (namespace.protocol, namespace.uuid), aff_dict)
        self.update_cluster_data(10)

    def set_namespace_ack_expectations(self, namespace, ack_expectations = { }):
        namespace = self.find_namespace(namespace)
        ae_dict = { }
        for datacenter, count in ack_expectations.iteritems():
            ae_dict[self.find_datacenter(datacenter).uuid] = count
        namespace.ack_expectations.update(ae_dict)
        self.do_query("POST", "/ajax/semilattice/%s_namespaces/%s/ack_expectations" % (namespace.protocol, namespace.uuid), ae_dict)
        self.update_cluster_data(10)

    def add_namespace(self, protocol = "memcached", name = None, port = None, primary = None, affinities = { }, ack_expectations = { }, primary_key = None, database = None, check = False):
        assert protocol in ["dummy", "memcached", "rdb"]
        if port is None:
            port = random.randint(10000, 20000)
        if name is None:
            name = str(random.randint(0, 1000000))
        if primary is not None:
            primary = self.find_datacenter(primary).uuid
        else:
            primary = random.choice(self.datacenters.keys())
        aff_dict = { }
        for datacenter, count in affinities.iteritems():
            aff_dict[self.find_datacenter(datacenter).uuid] = count
        ack_dict = { }
        for datacenter, count in ack_expectations.iteritems():
            ack_dict[self.find_datacenter(datacenter).uuid] = count
        if database is None:
            database_uuid = None
        else:
            database_uuid = self.find_database(database).uuid
        data_to_post = {
            "name": name,
            "port": port,
            "primary_uuid": primary,
            "replica_affinities": aff_dict,
            "ack_expectations": ack_dict,
            "database": database_uuid
            }
        if protocol == "rdb":
            if primary_key is None:
                primary_key = "id"
            # Right now `primary_key` is required to be `"id"` because the
            # server doesn't support setting the primary key.
            # data_to_post["primary_key"] = primary_key
            assert primary_key == "id"
        else:
            assert primary_key is None
        info = self.do_query("POST", "/ajax/semilattice/%s_namespaces/new" % protocol, data_to_post)
        assert len(info) == 1
        uuid, json_data = next(info.iteritems())
        type_class = namespace_classes_by_protocol_name[protocol]
        namespace = type_class(uuid, json_data)
        getattr(self, "%s_namespaces" % protocol)[namespace.uuid] = namespace
        self.update_cluster_data(10)
        if check:
            self._wait_for_namespace(namespace, 90)
            print "Namespace available"
        return namespace

    def _wait_for_namespace(self, namespace, timeout):
        while True:
            try:
                self.get_distribution(namespace)
                return
            except BadServerResponse:
                time.sleep(1)
                timeout = timeout - 1
                if timeout <= 0:
                    raise

    def rename(self, target, name):
        types = {
            DummyNamespace: (self.dummy_namespaces, "dummy_namespaces"),
            MemcachedNamespace: (self.memcached_namespaces, "memcached_namespaces"),
            RDBNamespace: (self.rdb_namespaces, "rdb_namespaces"),
            Machine: (self.machines, "machines"),
            Datacenter: (self.datacenters, "datacenters")
            }
        assert types[type(target)][0][target.uuid] is target
        object_type = types[type(target)][1]
        target.name = name
        info = self.do_query("POST", "/ajax/semilattice/%s/%s/name" % (object_type, target.uuid), name)

    def get_conflicts(self):
        return self.conflicts

    def resolve_conflict(self, conflict, value):
        assert conflict in self.conflicts
        assert value in conflict.values
        types = {
            DummyNamespace: (self.dummy_namespaces, "dummy_namespaces"),
            MemcachedNamespace: (self.memcached_namespaces, "memcached_namespaces"),
            RDBNamespace: (self.rdb_namespaces, "rdb_namespaces"),
            Machine: (self.machines, "machines"),
            Datacenter: (self.datacenters, "datacenters")
            }
        assert types[type(conflict.target)][conflict.target.uuid] is conflict.target
        object_type = types[type(conflict.target)]
        info = self.do_query("POST", "/ajax/semilattice/%s/%s/%s/resolve" % (object_type, conflict.target.uuid, conflict.field), value)
        # Remove the conflict and update the field in the target
        self.conflicts.remove(conflict)
        setattr(conflict.target, conflict.field, value) # TODO: this probably won't work for certain things like shards that we represent differently locally than the strict json format
        self.update_cluster_data(10)

    def add_namespace_shard(self, namespace, split_point):
        namespace = self.find_namespace(namespace)
        namespace.add_shard(split_point)
        info = self.do_query("POST", "/ajax/semilattice/%s_namespaces/%s/shards" % (namespace.protocol, namespace.uuid), namespace.shards_to_json())
        self.update_cluster_data(10)

    def remove_namespace_shard(self, namespace, split_point):
        namespace = self.find_namespace(namespace)
        namespace.remove_shard(split_point)
        info = self.do_query("POST", "/ajax/semilattice/%s_namespaces/%s/shards" % (namespace.protocol, namespace.uuid), namespace.shards_to_json())
        self.update_cluster_data(10)

    def change_namespace_shards(self, namespace, adds=[], removes=[]):
        namespace = self.find_namespace(namespace)
        for split_point in adds:
            namespace.add_shard(split_point)
        for split_point in removes:
            namespace.remove_shard(split_point)
        info = self.do_query("POST", "/ajax/semilattice/%s_namespaces/%s/shards" % (namespace.protocol, namespace.uuid), namespace.shards_to_json())
        self.update_cluster_data(10)

    def get_datacenter_in_namespace(self, namespace, primary = None):
        namespace = self.find_namespace(namespace)
        if primary is not None:
            return self.datacenters[namespace.primary_uuid]

        # Build a list of datacenters in the given namespace
        datacenters = [ self.datacenters[namespace.primary_uuid] ]
        for uuid in namespace.replica_affinities.iterkeys():
            datacenters.append(self.datacenters[uuid])
        return random.choice(datacenters)

    def get_progress(self):
        return self.do_query("GET", "/ajax/progress")

    def get_issues(self):
        return self.do_query("GET", "/ajax/issues")

    def check_no_issues(self):
        issues = self.get_issues()
        if issues:
            message = ""
            for issue in issues:
                message += issue["description"] + "\n"
            raise RuntimeError("Cluster has issues:\n" + message)

    def get_distribution(self, namespace, depth = 1):
        return self.do_query("GET", "/ajax/distribution?namespace=%s&depth=%d" % (namespace.uuid, depth))

    def is_blueprint_satisfied(self, namespace):
        namespace = self.find_namespace(namespace)
        directory = self.do_query("GET", "/ajax/directory/_")
        blueprint = self.do_query("GET", "/ajax/semilattice/%s_namespaces/%s/blueprint" % (namespace.protocol, namespace.uuid))
        for peer, shards in blueprint["peers_roles"].iteritems():
            if peer in directory:
                subdirectory = directory[peer]
            else:
                return False
            if namespace.uuid in subdirectory[namespace.protocol + "_namespaces"]["reactor_bcards"]:
                reactor_bcard = subdirectory[namespace.protocol + "_namespaces"]["reactor_bcards"][namespace.uuid]
            else:
                return False
            for shard_range, shard_role in shards.iteritems():
                for act_id, (act_range, act_info) in reactor_bcard["activity_map"].iteritems():
                    if act_range == shard_range:
                        if shard_role == "role_primary" and act_info["type"] == "primary" and act_info["replier_present"] is True:
                            break
                        elif shard_role == "role_secondary" and act_info["type"] == "secondary_up_to_date":
                            break
                        elif shard_role == "role_nothing" and act_info["type"] == "nothing":
                            break
                else:
                    return False
        return True

    def wait_until_blueprint_satisfied(self, namespace, timeout = 600):
        start_time = time.time()
        while not self.is_blueprint_satisfied(namespace):
            time.sleep(1)
            if time.time() - start_time > timeout:
                ajax = self.do_query("GET", "/ajax")
                progress = self.do_query("GET", "/ajax/progress")
                raise RuntimeError("Blueprint still not satisfied after %d seconds.\nContents of /ajax =\n%r\nContents of /ajax/progress =\n%r" % (timeout, ajax, progress))

    def _pull_cluster_data(self, cluster_data, local_data, data_type):
        for uuid in cluster_data.iterkeys():
            validate_uuid(uuid)
            if uuid not in local_data:
                local_data[uuid] = data_type(uuid, cluster_data[uuid])
        assert len(cluster_data) == len(local_data)

    # Get the list of machines/namespaces from the cluster, verify that it is consistent across each machine
    def _verify_consistent_cluster(self, timeout):
        timeout = max(1, timeout)
        last_error = ("", "")
        consistent = False

        while timeout > 0 and not consistent:
            time.sleep(1)
            expected = self.do_query_specific(self.addresses[0][0], self.addresses[0][1], "GET", "/ajax/semilattice")
            del expected[u"me"]
            matches = 0
            for address in self.addresses:
                actual = self.do_query_specific(address[0], address[1], "GET", "/ajax/semilattice")
                del actual[u"me"]
                if actual == expected:
                    matches = matches + 1
                else:
                    last_error = (actual, expected)
            if matches == len(self.addresses):
                consistent = True
            timeout = timeout - 1

        if not consistent:
            raise BadClusterData(last_error[0], last_error[1])

        def remove_nones(d):
            for key in d.keys():
                if d[key] is None:
                    del d[key]
        remove_nones(expected[u"machines"])
        remove_nones(expected[u"datacenters"])
        remove_nones(expected[u"dummy_namespaces"])
        remove_nones(expected[u"memcached_namespaces"])
        remove_nones(expected[u"rdb_namespaces"])
        return expected

    def _verify_cluster_data_chunk(self, local, remote):
        for uuid, obj in local.iteritems():
            check_obj = True
            for field, value in remote[uuid].iteritems():
                if value == u"VALUE_IN_CONFLICT":
                    if obj not in self.conflicts:
                        # Get the possible values and create a value conflict object
                        if isinstance(obj, Namespace):
                            path = obj.protocol + "_namespaces"
                        elif isinstance(obj, Machine):
                            path = "machine"
                        elif isinstance(obj, Datacenter):
                            path = "datacenter"
                        resolve_data = self.do_query("GET", "/ajax/semilattice/%s/%s/%s/resolve" % (path, obj.uuid, field))
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
        self._verify_cluster_data_chunk(self.rdb_namespaces, data[u"rdb_namespaces"])

    def update_cluster_data(self, timeout):
        data = self._verify_consistent_cluster(timeout)
        self._pull_cluster_data(data[u"machines"], self.machines, Machine)
        self._pull_cluster_data(data[u"datacenters"], self.datacenters, Datacenter)
        self._pull_cluster_data(data[u"dummy_namespaces"], self.dummy_namespaces, DummyNamespace)
        self._pull_cluster_data(data[u"memcached_namespaces"], self.memcached_namespaces, MemcachedNamespace)
        self._pull_cluster_data(data[u"rdb_namespaces"], self.rdb_namespaces, RDBNamespace)
        self._verify_cluster_data(data)
        return data

#!/usr/bin/env python

"""
The RethinkDB Administration Web Server Program
"""

from flask import Flask, render_template, jsonify, request, abort
from beaker.middleware import SessionMiddleware
import json
from random import randrange
import datetime
import argparse
import os
import uuid as python_uuid
from backend import SimulationBackend

# Parse the command-line arguments
def parse_cmd_line_args():
    "Parses the command line arguments, setting backend and port."

    parser = argparse.ArgumentParser(
        description='RethinkDB web administration front-end')
    behelp = "Specify the backend to use for data (simulation or host:port)"
    parser.add_argument('--backend', dest='backend', default='simulation',
                        help=behelp)
    parser.add_argument('--port', default=os.getenv('RDB_ADMIN_PORT', '5000'),
                        help='The port on which the web UI is served.')

    parser_args = parser.parse_args()

    the_port = int(parser_args.port)
    if parser_args.backend != 'simulation':
        raise "Sorry, you should have used the simulation backend."

    the_backend = SimulationBackend()

    return the_backend, the_port

backend, port = parse_cmd_line_args()

# helper functions to get namespaces, machines, and datacenters as json
def get_namespaces():
    "Returns the namespaces of the backend."
    return_data = []
    for namespace in backend.get('/namespaces'):
        return_data.append(backend.get('/namespaces/'+namespace))
    return json.dumps(return_data)

def get_machines():
    "Returns the machines of the backend."
    return_data = []
    for machine in backend.get('/machines'):
        return_data.append(backend.get('/machines/'+machine))
    return json.dumps(return_data)

def get_datacenters():
    "Returns the datacenters of the backend."
    return_data = []
    for datacenter in backend.get('/datacenters'):
        return_data.append(backend.get('/datacenters/'+datacenter))
    return json.dumps(return_data)

# compare two json objects and return a list of diffs in json
def diff(old_data, new_data):
    "Returns a list of diffs between two jsonable objects."

    diffs = []
    for typ in new_data:
        for element_id in new_data[typ]:
            # check for new elements
            if not element_id in old_data.get(typ, {}):
                diffs.append({
                    'operation': 'add',
                    'element': typ,
                    'id': element_id,
                    'data': new_data[typ][element_id]
                })

            # check if the element is different
            elif old_data[typ][element_id] != new_data[typ][element_id]:
                diffs.append({
                    'operation': 'update',
                    'element': typ,
                    'id': element_id,
                    'data': new_data[typ][element_id]
                })

    for typ in old_data:
        for element_id in old_data[typ]:
            # check for deleted elements
            if not element_id in new_data[typ]:
                diffs.append({
                    'operation': 'delete',
                    'element': typ,
                    'id': element_id,
                    'data': {}
                })

    return diffs

def result_json(result, token):
    "Returns a json object with result and diffs."
    return json.dumps({ 'op_result': result,
                        'diffs': get_updates(token) })

# Define the Flask application
app = Flask(__name__)

# Set up assets
from flaskext.assets import Environment

assets = Environment(app)
assets.config['less_path'] = '/usr/bin/lessc'

# Options for the beaker session
session_opts = {
    'session.type': 'memory',
    'session.auto': True
}

tokensess = { }

@app.route('/')
def cluster():
    "The cluster route."
    # set the session data to the latest json state, and give the data
    # to the template to be bootstrapped into the Backbone application
    sess = request.environ['beaker.session']
    json_state = json.dumps(backend.get('/'))
    token = str(python_uuid.uuid4())
    tokensess[token] = json_state
    return render_template('cluster.html',
                           namespaces=get_namespaces(),
                           machines=get_machines(),
                           datacenters=get_datacenters(),
                           token=token)

@app.route('/ajax/full_state')
def get_full_state():
    "Returns a full state update."
    return json.dumps(backend.get('/'))

@app.route('/ajax/updates')
def do_get_updates():
    """Gets a diff of the latest state, or a flag indicating that the
    client should be refreshed."""

    return json.dumps(get_updates(request.args['token']))


def get_updates(token):
    "Returns a dict for the updates since a given timestamp."
    if not token in tokensess:
        res = {"full_update": "true"}
    else:
        json_state = backend.get('/')
        res = diff(json.loads(tokensess[token]), json_state)
        tokensess[token] = json.dumps(json_state)
    return res

@app.route('/ajax/reset_data')
def reset_data():
    "Resets simulation data."
    backend.reset()
    print "Reset simulation data."
    return ''

# TODO: Have this reset the tokensess or get rid of this.
@app.route('/ajax/reset_session')
def reset_session():
    "Resets the session data."
    request.environ['beaker.session'].invalidate()
    return ''

@app.route('/ajax/performance/throughput/current')
def performance_throughput_turrent():
    "ajax performance print return_data(?)"
    return jsonify(performance=randrange(0, 1000))

@app.route('/ajax/logs/rethinkdb')
def logs_rethinkdb():
    "ajax logging api"
    return jsonify(log_message='Request received from ' +
                   request.remote_addr + ' on '
                   + str(datetime.datetime.now()))

@app.route('/ajax/terminal/command', methods=['GET','POST'])
def terminal_command():
    "ajax terminal api"
    return ('You typed: ' + request.form['command'] +
            ' (performed on ' + request.form['namespace'] + '/' +
            request.form['protocol'] + ')')

@app.route('/ajax/datacenters', methods=['GET', 'POST', 'DELETE'])
def datacenters():
    "ajax datacenters api"
    if request.method == 'POST':
        uuid = backend.add('datacenters', {
            'name': request.form['name'],
            'health': randrange(0,100),
            'status': 'Available',
        })

        return result_json(result={ 'uuid' : uuid,
                                    'name' : request.form['name']},
                           token=request.args['token'])

    elif request.method == 'DELETE':
        to_delete = request.args.get('ids').split(',')
        results = []
        for uuid in to_delete:
            try:
                # get the name of the datacenter before we delete it
                name = backend.get('/datacenters/'+uuid+'/name')

                # delete the namespace
                backend.delete('datacenters', uuid)

                # record the result
                results.append({
                    'uuid': uuid,
                    'name': name
                })
            except KeyError:
                print 'Datacenter with uuid ' + uuid + ' not found.'

        return result_json(results, request.args['token'])
    else:
        return get_datacenters()

@app.route('/ajax/datacenters/<uuid>', methods=['GET', 'DELETE'])
def datacenter_summary(uuid):
    "Gets or deletes individual datacenter."
    if request.method == 'DELETE':
        try:
            backend.delete('datacenters', uuid)
            return ''
        except KeyError:
            abort(404)
    else:
        try:
            datacenter = backend.get('/datacenters/' + uuid)
            return json.dumps(datacenter)
        except KeyError:
            abort(404)

@app.route('/ajax/namespaces', methods=['GET', 'POST', 'DELETE'])
def namespaces():
    "ajax namespace api"

    if request.method == 'POST':
        uuid = backend.add('namespaces', {
            'name': request.form['name'],
            'protocol': request.form['protocol'].lower(),
            'primary_uuid': request.form['primary-datacenter'],
            'replica_affinities': {
                request.form['primary-datacenter']: 3,
            }
        })

        return result_json(result={ 'uuid' : uuid,
                                    'name' : request.form['name']},
                           token=request.args['token'])

    elif request.method == 'DELETE':
        to_delete = request.args.get('ids').split(',')
        results = []
        for uuid in to_delete:
            try:
                # get the name of the namespace before we delete it
                name = backend.get('/namespaces/'+uuid+'/name')

                # delete the namespace
                backend.delete('namespaces', uuid)

                # record the result
                results.append({
                    'uuid': uuid,
                    'name': name
                })
            except KeyError:
                print 'Namespace with uuid ' + uuid + ' not found.'

        return result_json(results, request.args['token'])
    else:
        return get_namespaces()

@app.route('/ajax/namespaces/<uuid>', methods=['GET', 'DELETE'])
def namespace_summary(uuid):
    "Namespace get/deletion."

    if request.method == 'DELETE':
        try:
            backend.delete('namespaces', uuid)
            return ''
        except KeyError:
            abort(404)
    else:
        try:
            namespace = backend.get('/namespaces/'+uuid)
            return json.dumps(namespace)
        except KeyError:
            abort(404)

def pick_machines_from_datacenter(datacenter_id, machs, num_to_pick):
    all_machines = backend.get('/machines')
    candidates = [m['id'] for m in all_machines if all_machines[m]['datacenter_uuid'] == datacenter_id]
    choices = [c for c in candidates if c not in machs]
    return machs + choices[0:num_to_pick]


@app.route('/ajax/namespaces/<uuid>/set_desired_replicas_and_acks', methods=['POST'])
def namespace_set_desired_replicas_and_acks(uuid):
    "Sets the desired replica count and ack count for a namespace."

    # TODO actually support num_acks
    datacenter_id = request.form['datacenter']
    num_acks = int(request.form['num_acks'])
    num_replicas = int(request.form['num_replicas'])

    machines_by_shard = backend.get('/namespaces/%s/replica_affinities/%s/machines' % (uuid, datacenter_id))

    new_machines_by_shard = []
    for machs in machines_by_shard:
        new_machs = None
        if num_replicas < len(machs):
            new_machs = machs[0:num_replicas]
        elif num_replicas > len(machs):
            new_machs = pick_machines_from_datacenter(datacenter_id, machs, num_replicas)
        else:
            new_machs = machs
        new_machines_by_shard += [new_machs]

    backend.set('/namespaces/%s/replica_affinities/%s' % (uuid, datacenter_id),
                { 'desired_replication_count': num_replicas,
                  'machines': new_machines_by_shard })

    return result_json({}, request.args['token'])

@app.route('/ajax/namespaces/<uuid>/edit_machines', methods=['POST'])
def namespace_edit_machines(uuid):
    "Edits the machines for a particular datacenter."

    datacenter_id = request.form['datacenter']

    replicas = []
    slice_ix = 0
    slice_machines = request.form.getlist('machine_for_%d' % slice_ix)
    while len(slice_machines) > 0:
        replicas += [slice_machines]
        slice_ix += 1
        slice_machines = request.form.getlist('machine_for_%d' % slice_ix)

    backend.set('/namespaces/%s/replica_affinities/%s/machines' % (uuid, datacenter_id), replicas)

    return result_json({}, request.args['token'])

@app.route('/ajax/namespaces/<uuid>/modify_replicas', methods=['POST'])
def namespace_modify_replicas(uuid):
    "Adds or removes replicas and sets the ack count for a particular datacenter."


    datacenter_id = request.form['datacenter']
    num_acks = int(request.form['num_acks'])
    num_replicas = int(request.form['num_replicas'])

    replicas = []
    slice_ix = 0
    slice_machines = request.form.getlist('machine_for_%d' % slice_ix)
    while len(slice_machines) > 0:
        replicas += [slice_machines]
        slice_ix += 1
        slice_machines = request.form.getlist('machine_for_%d' % slice_ix)

    backend.set('/namespaces/%s/replica_affinities/%s' % (uuid, datacenter_id),
                { 'desired_replication_count': num_replicas,
                  'machines': replicas })

    return result_json({}, request.args['token'])

@app.route('/ajax/namespaces/<uuid>/apply_shard_plan', methods=['POST'])
def namespace_apply_shard_plan(uuid):
    "Applies a shard plan for a resharding."

    # Diff the previous and existing shard plans.
    existing = backend.get('/namespaces/%s/shards' % uuid)
    proposed = request.form.getlist('proposed_shard_boundaries')

    i = 0
    j = 0

    replica_affinities = backend.get('/namespaces/%s/replica_affinities' % uuid)

    # current_sharding's keys are datacenter ids, values, a list of
    # machines for the current shard.
    def get_current_sharding(index):
        return dict([(k, replica_affinities[k]['machines'][index]) for k in replica_affinities])

    # we will build this list. (of type List<Map<DatacenterID, List<MachineId>>>)
    sharding_scheme = [get_current_sharding(i)]

    while i < len(existing) or j < len(proposed):
        if j == len(proposed):
            i += 1
        elif i == len(existing) or existing[i] > proposed[j]:
            j += 1
            sharding_scheme.append(get_current_sharding(i))
        elif existing[i] < proposed[j]:
            i += 1
        else:
            i += 1
            j += 1
            sharding_scheme.append(get_current_sharding(i))

    # suggestions is of type Map<DatacenterId, List<List<MachineId>>>
    suggestions = { }
    for datacenter_id in replica_affinities:
        suggestions[datacenter_id] = [x[datacenter_id] for x in sharding_scheme]

    print "setting shards to", proposed
    backend.set('/namespaces/%s/shards' % uuid, proposed)
    backend.set('/namespaces/%s/replica_affinities' % uuid,
                dict([(d, { 'desired_replication_count': replica_affinities[d]['desired_replication_count'],
                            'machines': suggestions[d] })
                      for d in replica_affinities]))

    return result_json({}, request.args['token'])

@app.route('/ajax/namespaces/<uuid>/add_secondary', methods=['POST'])
def namespace_add_secondary(uuid):
    "Adds a secondary datacenter to a namespace."

    key = '/namespaces/%s' % uuid
    affinities = backend.get(key + '/replica_affinities')
    affinities[request.form['datacenter']] = {
        'desired_replication_count': 0,
        'machines': [list([]) for x in xrange(1 + len(backend.get(key + '/shards')))]
    }

    backend.set(key + '/replica_affinities', affinities)

    return result_json({}, request.args['token'])

# TODO get rid of the add_shard and merge_shards method.
@app.route('/ajax/namespaces/<uuid>/add_shard', methods=['POST'])
def namespace_add_shard(uuid):
    "Adds a shard boundary to a namespace."

    key = '/namespaces/%s/shards' % uuid
    splitpoint = request.form['splitpoint']
    new_splitpoints = sorted(backend.get(key) + [request.form['splitpoint']])
    backend.set(key, new_splitpoints)
    shard_index = new_splitpoints.index(splitpoint)

    ret = {}
    ret['splitpoint'] = splitpoint
    if shard_index > 0:
        ret['lower'] = new_splitpoints[shard_index - 1]
    if shard_index < len(new_splitpoints) - 1:
        ret['upper'] = new_splitpoints[shard_index + 1]

    return result_json(ret, request.args['token'])

@app.route('/ajax/namespaces/<uuid>/merge_shards', methods=['POST'])
def namespace_merge_shards(uuid):
    "Merges two shards of a namespace."

    key = '/namespaces/%s/shards' % uuid
    shards = backend.get(key)
    shard_index = int(request.form['index'])

    ret = {}
    ret['middle'] = shards[shard_index]
    if shard_index > 0:
        ret['lower'] = shards[shard_index - 1]
    if shard_index < len(shards) - 1:
        ret['upper'] = shards[shard_index + 1]

    backend.set(key, shards[:shard_index] + shards[shard_index+1:])

    return result_json(ret, request.args['token'])

@app.route('/ajax/machines', methods=['GET', 'POST'])
def machines():
    "ajax machine api"

    if request.method == 'POST':
        backend.add('machines', {
            'name': request.form['name'],
            'status': 'Available',
            'ip': request.form['ip'],
            'datacenter_uuid': request.form['datacenter_uuid'],
            'uptime': '0 days, 0:00',
            'cpu': randrange(0,100),
            'used_ram': randrange(0,2048),
            'total_ram': '2048',
            'used_disk': randrange(0,1000000),
            'total_disk': '1000000',
        })
        return json.dumps(get_updates(0))
    else:
        return get_machines()

@app.route('/ajax/machines/<uuid>', methods=['GET', 'DELETE', 'POST'])
def machine_summary(uuid):
    "Gets or deletes a machine."
    if request.method == 'POST':
        try:
            backend.set('/machines/%s/datacenter_uuid' % uuid,
                        request.form['datacenter_uuid'])
            return result_json({ 'datacenter_uuid':
                                     request.form['datacenter_uuid'] },
                               request.args['token'])
        except KeyError:
            abort(404)

    elif request.method == 'DELETE':
        try:
            backend.delete('machines', uuid)
            return ''
        except KeyError:
            abort(404)
    else:
        try:
            # TODO: submit a uuid.  (??)
            machine = backend.get('/machines/'+uuid)
            return json.dumps(machine)
        except KeyError:
            abort(404)

@app.route('/ajax/alerts/')
def alerts():
    "Gets the alerts?  Maybe obsolete."
    ret = {}

    for key in backend.get('/alerts').keys():
        ret[key] = backend.get('/alerts/' + key)
    return jsonify(alerts = ret)

@app.route('/ajax/alerts/<uuid>')
def element_alerts(uuid):
    "Gets the given alert?"
    ret = {}

    for key in backend.get('/alerts').keys():
        alert = backend.get('/alerts/'+key)
        if alert['applies_to'] == uuid:
            ret[key] = alert
    return jsonify(alerts = ret)

# Start the Flask application
if __name__ == '__main__':
    app.wsgi_app = SessionMiddleware(app.wsgi_app, session_opts)
    app.run(debug=True, host='0.0.0.0', port=port)

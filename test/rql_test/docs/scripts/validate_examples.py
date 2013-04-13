import os
import sys
import yaml
import re
from subprocess import Popen, PIPE, call
from random import randrange
from threading import Thread
import SocketServer
import struct
import pdb

sys.path.insert(0, '../../../drivers/python/rethinkdb')
import ql2_pb2 as p

# tree of YAML documents defining documentation
src_dir = sys.argv[1]

commands = []

# Walk the src files to compile all sections and commands
for root, dirs, file_names in os.walk(src_dir):
    for file_name in file_names:
        docs = yaml.load(file(os.path.join(root, file_name)))

        if 'commands' in docs:
            commands.extend(docs['commands'])

def validate_for(lang, port):
    test_file_name = 'build/test.%s' % lang
    with open(test_file_name, 'w') as out:

        if lang == 'py':
            out.write("""
from sys import path
path.insert(0, '../../../drivers/python')
import rethinkdb as r
conn = r.connect(port=%d)
print 'Running python validation.'
""" % port)
        elif lang == 'js':
            out.write("""
var r = require("../../../../drivers/javascript/build/rethinkdb");
var callback = (function() { });
var cur = {next:(function(){}), hasNext:(function(){}), each:(function(){}), toArray:(function(){})};
r.connect({port:%d}, function(err, conn) {
console.log("Running Javascript validation.");
            """ % port)
        elif lang == 'rb':
            out.write("""
$LOAD_PATH.unshift('../../../drivers/ruby/lib')
require 'rethinkdb.rb'
include RethinkDB::Shortcuts
conn = r.connect('localhost', %d)
puts 'Running Ruby validation.'
""" % port)

        for command in commands:
            section_name = command['section']
            command_name = command['tag']

            for i,example in enumerate(command['examples']):
                test_tag = section_name+"-"+command_name+"-"+str(i)
                test_case = example['code']
                if isinstance(test_case, dict):
                    if lang in test_case:
                        test_case = test_case[lang]
                    else:
                        test_case = None

                if 'validate' in example and not example['validate']:
                    test_case = None
                    skip_validation = True
                else:
                    skip_validation = False

                # Check for an override of this test case
                if lang in command:
                    if isinstance(command[lang], bool) and not command[lang]:
                        test_case = None
                    elif isinstance(command[lang], dict):
                        override = command[lang]
                        if 'examples' in override:
                            if i in override['examples']:
                                example_override = override['examples'][i]

                                if len(example_override) == 0:
                                    test_case = None
                                elif 'code' in example_override:
                                    test_case = example_override['code']

                                if 'validate' in example_override:
                                    if not example_override['validate']:
                                        test_case = None
                                elif skip_validation:
                                    test_case = None

                comment = '#'
                if lang == 'js':
                    comment = '//'

                if test_case != None:
                    test_case = re.sub("\n", " %s %s\n" % (comment, test_tag), test_case)
                    out.write("%s %s %s\n" % (test_case, comment, test_tag))

        if lang == 'js':
            out.write("console.log('Javascript validation complete.');\n");
            out.write("conn.close()})")
        if lang == 'py':
            out.write("print 'Python validation complete.'");
        if lang == 'rb':
            out.write("puts 'Ruby validation complete.'");

    if lang == 'py':
        interpreter = 'python'
    elif lang == 'js':
        interpreter = 'node'
    elif lang == 'rb':
        interpreter = 'ruby'

    ret = call([interpreter, test_file_name])

    if ret is not 0:
        sys.exit(1)

class BlackHoleRDBHandler(SocketServer.BaseRequestHandler):
    def handle(self):
        magic = self.request.recv(4)

        while (True):
            header = self.request.recv(4)
            if len(header) == 0:
                break;

            (length,) = struct.unpack("<L", header)
            data = self.request.recv(length)

            query = p.Query()
            query.ParseFromString(data)

            response = p.Response()
            response.token = query.token
            response.type = p.Response.SUCCESS_ATOM
            datum = response.response.add()
            datum.type = p.Datum.R_NULL

            response_protobuf = response.SerializeToString()
            response_header = struct.pack("<L", len(response_protobuf))
            self.request.sendall(response_header + response_protobuf)

def validate():
    # Setup void server
    port = randrange(1025, 65535)
    server = SocketServer.TCPServer(('localhost', port), BlackHoleRDBHandler)

    t = Thread(target=server.serve_forever)
    t.start()

    try:
        validate_for('py', port)
        validate_for('js', port)
        validate_for('rb', port)
    finally:
        server.shutdown()

validate()

#!/usr/bin/python
# This script is used by run-tests.sh to build a list of all tests that can be run.
# The --test-dir argument should probalby be test/full_test
# A file is create in the --output-dir for each test

import os, traceback, optparse, itertools

parser = optparse.OptionParser()
parser.add_option("--test-dir", action="store", dest = "test_dir")
parser.add_option("--output-dir", action="store", dest="output_dir")
parser.set_defaults(output_dir = ".")
(options, args) = parser.parse_args()
assert not args

if options.test_dir is None:
    parser.error("You must specify --test-dir");

tests = { }

def generate_test(base_name):
    def gen(test_command, name):
        full_name = base_name + '-' + name
        if tests.has_key(full_name):
            for i in itertools.count():
                new_name = full_name + '-' + str(i)
                if not tests.has_key(new_name):
                    break
            full_name = new_name
        tests[full_name] = test_command
    return gen

for (dirpath, dirname, filenames) in os.walk(options.test_dir):
    filenames = [f for f in filenames if f.split('.')[-1] == 'test']
    for filename in filenames:
        base_name = filename.split('.')[0]
        full_path = os.path.join(dirpath, filename)
        print "Reading specification file %r" % full_path
        execfile(full_path, {"__builtins__": __builtins__, "generate_test": generate_test(base_name)})

for name, test_command in tests.items():
    f = open("%s/%s.param" % (options.output_dir, name), 'w')
    f.write("TEST_COMMAND=%s\n" % test_command)
    f.write("TEST_ID=%s" % name)
    f.close()


#!/usr/bin/python
import os, sys, subprocess, argparse
from termcolor import colored, cprint
import time

tests = [
    'add-a-namespace',
    'view-dashboard',
]

git_root = subprocess.Popen(['git','rev-parse','--show-toplevel'],  stdout=subprocess.PIPE).communicate()[0].rstrip('\r\n')
test_file_dir = os.path.join(git_root,'test/ui_test/')
cwd = os.getcwd()

# Define and parse command-line arguments
parser = argparse.ArgumentParser(description='Run a set of UI tests using CasperJS / PhantomJS.')
parser.add_argument('tests', nargs='*', 
        help='List of tests to run. Specify \'all\' to run all tests.')
parser.add_argument('-p','--rdb-port', nargs='?', 
        dest='rdb_port', default='6001',
        help='Port of the RethinkDB server to connect to (default is 6001).')
parser.add_argument('-i','--output-images', nargs='?', 
        dest='image_output_directory', const='./casper-results',
        help='Include if images should be scraped and saved. Optionally specify the output directory (default is ./casper-results/).')
parser.add_argument('-l','--list-tests', action='store_true',
        help='List available tests to run.')
parser.add_argument('-r','--output-results', nargs='?',
        dest='result_output_directory', const='./casper-results',
        help='Include if test results should be saved. Optionally specify the output directory (default is ./casper-results/).')
args = parser.parse_args()

def print_available_tests():
    print 'Available tests:'
    print '\t- all: run all of the following tests'
    for test in tests:
        print '\t- '+test[0]
    
if args.list_tests:
    print_available_tests()
    exit(0)

if len(args.tests) < 1:
    parser.print_usage()
    print '\nNo test specified.',
    print_available_tests()
    exit(1)

# Prepare the list of tests to process; if 'all' was one of the specified tests then process all tests
if 'all' in args.tests:
    test_list = tests
else:
    test_list = args.tests

# Process each test name specified on the command line
successful_tests = 0
os.chdir(test_file_dir)
for test_name in test_list:
    # Look for a matching test among known tests
    casper_script = os.path.join(test_file_dir,test_name+'.coffee')
    try:
        with open(casper_script) as f: pass
    except IOError as e:
        print "No test script found for CasperJS test '%s'." % test_name
        continue

    # Build command with arguments for casperjs test
    cl = ['casperjs', '--rdb-server=http://localhost:'+args.rdb_port+'/', casper_script]

    # If the option to scrape images was specified, add it to the casperjs argument list
    if args.image_output_directory:
        image_dir = os.path.abspath(os.path.join(args.image_output_directory,test_name))
        cl.extend(['--images='+image_dir])

    # Execute casperjs and pretty-print its output
    process = subprocess.Popen(cl,stdout=subprocess.PIPE)
    stdout = process.stdout.readlines()
    for i, line in enumerate(stdout):
        cprint('[%s]' % test_name, attrs=['bold'], end=' ')
        print line.rstrip('\n')

    # If the option to save results was specified, save stdout to a file
    if args.result_output_directory:
        result_dir = os.path.abspath(os.path.join(args.result_output_directory,test_name))
        result_filename = 'casper-result'
        result_file = open(os.path.join(result_dir,result_filename),'w')
        for line in stdout:
            result_file.write(line)
        result_file.close()

    # Check the exit code of the process
    #   0: casper test passed
    #   1: casper test failed
    process.poll()
    if process.returncode == 0:
        successful_tests+=1

    print

# Print test suite summary
cprint(" %d of %d tests ran successfully. " % (successful_tests, len(test_list)), attrs=['reverse'])

# Note: start time output in report is currently broken

# Please configure these:

testing_nodes_ec2_instance_type = "t1.micro" # TODO: Change to m1.large later?
testing_nodes_ec2_count = 4 # number of nodes to spin up
testing_nodes_ec2_image_name = "ami-d40e5e91"
testing_nodes_ec2_key_pair_name = "cloudtest_default"
testing_nodes_ec2_security_group_name = "cloudtest_default"
testing_nodes_ec2_region = "us-west-1"
testing_nodes_ec2_access_key = "AKIAJUKUVO6J45QRZQKA"
testing_nodes_ec2_private_key = "d9SiQpDD/YfGA2uC7CyqY7jmRoZg5utHM4pxTAhE" # TODO: Use environment variables instead?

private_ssh_key_filename = "/home/daniel/default_ec2_private_key.pem" # TODO: ?

round_robin_locking_timeout = 1
wrapper_script_filename = "cloud_retester_run_test_wrapper.py"

# END of configuration options


import subprocess, shlex, signal, os, time, shutil, tempfile, sys, traceback, types, gitroot, random, atexit
from stat import *
import paramiko # Using Paramiko for SSH2
import boto, boto.ec2 # Using Boto for AWS commands

base_directory = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0])) + "/../test"

from vcoptparse import *
from retester import *
import retester

reports = []
test_references = [] # contains pairs of the form (node, tmp-path)
testing_nodes_ec2_reservation = None
testing_nodes = []

next_node_to_issue_to = 0



class TestReference:
    def __init__(self, command):
        self.single_runs = []
        self.command = command

class TestingNode:
    def __init__(self, hostname, port, username, private_ssh_key_filename):  
        self.hostname = hostname
        self.port = port
        self.username = username # "ec2-user"
        
        #print "Created TestingNode with hostname %s, port %i, username %s" % (hostname, port, username)
        
        # read private key from file to get access to the node
        if True: # Always use RSA for now
            self.private_ssh_key = paramiko.RSAKey(filename=private_ssh_key_filename)
        else:
            self.private_ssh_key = paramiko.DSSKey(filename=private_ssh_key_filename)
            
        self.global_lock_file = "/tmp/cloudtest_lock"
    
        system_random = random.SystemRandom()    
        self.global_build_path = "/tmp/cloudtest_build_" + str(system_random.randint(10000000, 99999999));
        #print "Installing build into %s\n" % self.global_build_path
        
        self.basedata_installed = False
        
        self.ssh_transport = None
        
    def __del__(self):
        if self.ssh_transport != None:
            self.ssh_transport.close()

        
    def get_transport(self):
        if self.ssh_transport != None:
            return self.ssh_transport
    
        # open SSH transport
        print "Opening connection to " + self.hostname
        self.ssh_transport = paramiko.Transport((self.hostname, self.port))
        self.ssh_transport.use_compression()
        self.ssh_transport.set_keepalive(60)
        self.ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        print "Got connection"
        
        return self.ssh_transport
        
    
    # returns a tupel (return code, output)
    def run_command(self, command):    
        # open SSH channel
        #self.ssh_transport = paramiko.Transport((self.hostname, self.port))
        #self.ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        ssh_transport = self.get_transport()
        ssh_channel = ssh_transport.open_session()
        
        # issue the command to the node
        ssh_channel.exec_command(command)
        
        # read back command result:
        # do not timeout while reading (probably default anyway?)
        ssh_channel.settimeout(None)
        # read output until we get an EOF
        command_output = ""
        output_read = ssh_channel.recv(4096) # No do-while loops in Python? wth?
        while len(output_read) > 0:
            command_output += output_read
            output_read = ssh_channel.recv(4096)
            
        # retrieve exit code
        command_exit_status = ssh_channel.recv_exit_status() # side effect: waits until command has finished
        
        ssh_channel.close()
        #self.ssh_transport.close()
        
        return (command_exit_status, command_output)            
           
        
    def put_file(self, local_path, destination_path):
        # open SFTP session
        #ssh_transport = paramiko.Transport((self.hostname, self.port))
        #ssh_transport.use_compression()
        #ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        ssh_transport = self.get_transport()
        sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
        
        # do the operation
        sftp_session.put(local_path, destination_path)
        
        sftp_session.close()
        #ssh_transport.close()
        
        
    def get_file(self, remote_path, destination_path):
        # open SFTP session
        #ssh_transport = paramiko.Transport((self.hostname, self.port))
        #ssh_transport.use_compression()
        #ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        ssh_transport = self.get_transport()
        sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
        
        # do the operation
        sftp_session.get(remote_path, destination_path)
        
        sftp_session.close()
        #ssh_transport.close()
        
        
    def put_directory(self, local_path, destination_path):
        # open SFTP session
        #ssh_transport = paramiko.Transport((self.hostname, self.port))
        #ssh_transport.use_compression()
        #ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        ssh_transport = self.get_transport()
        sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
        
        # do the operation
        for root, dirs, files in os.walk(local_path):
            for name in files:
                sftp_session.put(os.path.join(root, name), os.path.join(destination_path + root[len(local_path):], name))
                sftp_session.chmod(os.path.join(destination_path + root[len(local_path):], name), os.stat(os.path.join(root, name))[ST_MODE])
            for name in dirs:
                #print "mk remote dir %s" % os.path.join(destination_path + root[len(local_path):], name)
                sftp_session.mkdir(os.path.join(destination_path + root[len(local_path):], name))
        
        sftp_session.close()
        #ssh_transport.close()
        
        
    def make_directory(self, remote_path):
        # open SFTP session
        #ssh_transport = paramiko.Transport((self.hostname, self.port))
        #ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        ssh_transport = self.get_transport()
        sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
        
        # do the operation
        sftp_session.mkdir(remote_path)
        
        sftp_session.close()
        #ssh_transport.close()
        
        
    def make_directory_recursively(self, remote_path):
        # rely on mkdir command to do the work...
        mkdir_result = self.run_command("mkdir -p %s" % remote_path.replace(" ", "\\ "))
        
        if mkdir_result[0] != 0:
            print ("Unable to create directory")
            # TODO: Throw exception or something,,,
            
            
    def acquire_lock(self, locking_timeout = 0):
        lock_sleeptime = 1
        lock_command = "lockfile -%i -r -1 %s" % (lock_sleeptime, self.global_lock_file.replace(" ", "\' ")) # TODO: Better not use special characters in the lock filename with this incomplete escaping scheme...
        if locking_timeout > 0:
            lock_command = "lockfile -%i -r %i %s" % (lock_sleeptime, locking_timeout / lock_sleeptime, self.global_lock_file.replace(" ", "\' ")) # TODO: Better not use special characters in the lock filename with this incomplete escaping scheme...
        locking_result = self.run_command(lock_command)
        
        return locking_result[0] == 0
        
    def get_release_lock_command(self):
        return "rm -f %s" % self.global_lock_file.replace(" ", "\' ") # TODO: Better not use special characters in the lock filename with this incomplete escaping scheme...
        
    def release_lock(self):
        command_result = self.run_command(self.get_release_lock_command())
        if command_result[0] != 0:
            print "Unable to release lock (maybe the node wasn't locked before?)"
            # TODO: Throw exception or something,,,
            
            
def create_testing_nodes_from_reservation():
    global testing_nodes
    global testing_nodes_ec2_reservation
    global private_ssh_key_filename
    
    for instance in testing_nodes_ec2_reservation.instances:
        new_testing_node = TestingNode(instance.public_dns_name, 22, "ec2-user", private_ssh_key_filename)
        
        testing_nodes.append(new_testing_node)



def setup_testing_nodes():
    global testing_nodes
    
    atexit.register(terminate_testing_nodes)

    start_testing_nodes()
    
    # Do this on demand, such that we can start running tests on the first node while others still have to be initilized...
    #for node in testing_nodes:
    #    copy_basedata_to_testing_node(node)

def start_testing_nodes():
    global testing_nodes
    global testing_nodes_ec2_reservation
    global testing_nodes_ec2_image_name
    global testing_nodes_ec2_instance_type
    global testing_nodes_ec2_count
    global testing_nodes_ec2_key_pair_name
    global testing_nodes_ec2_security_group_name
    global testing_nodes_ec2_region
    global testing_nodes_ec2_access_key
    global testing_nodes_ec2_private_key

    # Reserve nodes in EC2
    
    print "Spinning up %i testing nodes" % testing_nodes_ec2_count
    
    ec2_connection = boto.ec2.connect_to_region(testing_nodes_ec2_region, aws_access_key_id=testing_nodes_ec2_access_key, aws_secret_access_key=testing_nodes_ec2_private_key)
    
    # Query AWS to start all instances
    ec2_image = ec2_connection.get_image(testing_nodes_ec2_image_name)
    testing_nodes_ec2_reservation = ec2_image.run(min_count=testing_nodes_ec2_count, max_count=testing_nodes_ec2_count, key_name=testing_nodes_ec2_key_pair_name, security_groups=[testing_nodes_ec2_security_group_name], instance_type=testing_nodes_ec2_instance_type)
    # query AWS to wait for all instances to be available
    for instance in testing_nodes_ec2_reservation.instances:
        while instance.state != "running":
            time.sleep(5)
            instance.update()
    
    create_testing_nodes_from_reservation()
    
    # Give it another 70 seconds to start up...
    time.sleep(70)
    
    # Check that all testing nodes are up
    for node in testing_nodes:
        # send a testing command
        command_result = node.run_command("echo -n Are you up?")
        if command_result[1] != "Are you up?":
            print "Node %s is down!!" % node.hostname # TODO: Throw exception # TODO: This check fails with an exception anyway
        else:
            print "Node %s is up" % node.hostname
        # TODO: handle problems gracefully...
        
        
def terminate_testing_nodes():
    global testing_nodes
    global testing_nodes_ec2_reservation
    global testing_nodes_ec2_region
    global testing_nodes_ec2_access_key
    global testing_nodes_ec2_private_key

    if testing_nodes_ec2_reservation != None:
        print "Terminating EC2 nodes"
    
        ec2_connection = boto.ec2.connect_to_region(testing_nodes_ec2_region, aws_access_key_id=testing_nodes_ec2_access_key, aws_secret_access_key=testing_nodes_ec2_private_key)
    
        # Query AWS to stop all instances
        testing_nodes_ec2_reservation.stop_all()
        testing_nodes_ec2_reservation = None
    
    testing_nodes = None
    
    
def cleanup_testing_node(node):
    node.run_command("rm -rf " + node.global_build_path)
    
    
def copy_basedata_to_testing_node(node):
    print "Sending base data to node %s" % node.hostname
    
    # TODO: All these static (source) paths are really ugly. Is there a more elegant way?
    node.make_directory_recursively("/tmp/cloudtest_libs")
    node.put_file("/usr/local/lib/libtcmalloc_minimal.so.0", "/tmp/cloudtest_libs/libtcmalloc_minimal.so.0")
    node.put_file("/usr/local/lib/libmemcached.so.5", "/tmp/cloudtest_libs/libmemcached.so.5")
    
    node.make_directory_recursively("/tmp/cloudtest_bin")
    node.put_file("/usr/local/bin/netrecord", "/tmp/cloudtest_bin/netrecord")
    node.put_file("/usr/bin/valgrind", "/tmp/cloudtest_bin/valgrind")
    command_result = node.run_command("chmod +x /tmp/cloudtest_bin/*")
    if command_result[0] != 0:
        print "Unable to make cloudtest_bin files executable"
    
    node.make_directory_recursively("/tmp/cloudtest_python")
    node.put_file(base_directory + "/../lib/vcoptparse/vcoptparse.py", "/tmp/cloudtest_python/vcoptparse.py")
    node.put_file(base_directory + "/../lib/rethinkdb_memcache/rethinkdb_memcache.py", "/tmp/cloudtest_python/rethinkdb_memcache.py")
    node.put_file("/usr/local/lib/python2.6/dist-packages/pylibmc.py", "/tmp/cloudtest_python/pylibmc.py")
    node.put_file("/usr/local/lib/python2.6/dist-packages/_pylibmc.so", "/tmp/cloudtest_python/_pylibmc.so")
    
    # Recursively copy build hierarchy
    # TODO: Maybe not copy all of it, especially leave obj subdirs out...
    node.make_directory(node.global_build_path)
    node.put_directory(base_directory + "/../build", node.global_build_path)
    
    node.basedata_installed = True
    
    

def copy_per_test_data_to_testing_node(node, test_reference, test_script):
    # Create test directory
    node.make_directory_recursively("cloud_retest")
    node.make_directory("cloud_retest/%s" % test_reference) # TODO: This fails if the dir exists, which we should catch and handle gracefully (by assigning another reference)
    
    # Link buil hierarchy
    command_result = node.run_command("ln -s %s cloud_retest/%s/build" % (node.global_build_path, test_reference))
    if command_result[0] != 0:
        print "Unable to link build environment"
        # TODO: Throw an exception
    
    # Create test and test/integration subdirectory
    node.make_directory("cloud_retest/%s/test" % test_reference)
    node.make_directory("cloud_retest/%s/test/integration" % test_reference)
    
    # copy common python files to there
    node.put_file(base_directory + "/integration/test_common.py", "cloud_retest/%s/test/integration/test_common.py" % test_reference)
    node.put_file(base_directory + "/integration/corrupter.py", "cloud_retest/%s/test/integration/corrupter.py" % test_reference)
    
    # copy test_script to there
    # TODO: if test_script is not in test/integration, create the correct directory and copy into there
    node.put_file(base_directory + "/" + test_script, "cloud_retest/%s/test/%s" % (test_reference, test_script))
    command_result = node.run_command("chmod +x cloud_retest/%s/test/%s" % (test_reference, test_script))
    if command_result[0] != 0:
        print "Unable to make test script executable"
    
    # Install the wrapper script
    node.put_file(base_directory + "/" + wrapper_script_filename, "cloud_retest/%s/test/%s" % (test_reference, wrapper_script_filename));
    
def start_test_on_node(node, test_command, test_timeout = None, locking_timeout = 0):
    if locking_timeout == None:
        locking_timeout = 0

    #print ("trying to acquire lock with timeout %i" % locking_timeout)
    if node.acquire_lock(locking_timeout) == False:
        return False
    #print ("Got lock!")
    
    # Initialize node if not happened before...
    if node.basedata_installed == False:
        copy_basedata_to_testing_node(node)
    
    test_script = str.split(test_command)[0] # TODO: Does not allow for white spaces in script file name

    # Generate random reference
    system_random = random.SystemRandom()
    test_reference = "cloudtest_" + str(system_random.randint(10000000, 99999999))
    print "Starting test with test reference %s on node %s" % (test_reference, node.hostname)
    # TODO: Introduce safety check that this reference is not occupied, possibly generating another reference
    
    # Prepare for test...
    copy_per_test_data_to_testing_node(node, test_reference, test_script)
    # Store test_command and test_timeout into some files on the remote node for the wrapper script to pick it up
    command_result = node.run_command("echo -n %s > cloud_retest/%s/test/test_command" % (test_command, test_reference))
    if command_result[0] != 0:
        print "Unable to store command"
        # TODO: Throw an exception    
    if test_timeout == None:
        command_result = node.run_command("echo -n \"\" > cloud_retest/%s/test/test_timeout" % (test_reference))
    else:
        command_result = node.run_command("echo -n %i > cloud_retest/%s/test/test_timeout" % (test_timeout, test_reference))
    if command_result[0] != 0:
        print "Unable to store timeout"
        # TODO: Throw an exception
    
    # Run test and release lock after it has finished
    node.run_command("sh -c \"nohup sh -c \\\"(cd %s; LD_LIBRARY_PATH=/tmp/cloudtest_libs:$LD_LIBRARY_PATH PATH=/tmp/cloudtest_bin:$PATH PYTHONPATH=/tmp/cloudtest_python:$PYTHONPATH python %s; %s)&\\\"\"" % ("cloud_retest/%s/test" % test_reference, wrapper_script_filename.replace(" ", "\\ "), node.get_release_lock_command()))
    
    return (node, test_reference)

def get_report_for_test(test_reference):
    node = test_reference[0]
    result_start_time = float(node.run_command("cat cloud_retest/" + test_reference[1] + "/test/result_start_time")[1])
    result_result = node.run_command("cat cloud_retest/" + test_reference[1] + "/test/result_result")[1]
    result_description = node.run_command("cat cloud_retest/" + test_reference[1] + "/test/result_description")[1]
    if result_description == "":
        result_description = None
    
    # TODO: Collect additional results (maybe change wrapper not to use a different tmp dir for output?)
    
    return Result(result_start_time, result_result, result_description)
    
def issue_test_to_some_node(test_command, test_timeout = 0):
    global testing_nodes
    global next_node_to_issue_to
    global round_robin_locking_timeout
    
    test_successfully_issued = False
    while test_successfully_issued == False:
        # wait for a limited amount of time until that node is free to get work
        test_reference = start_test_on_node(testing_nodes[next_node_to_issue_to], test_command, test_timeout, round_robin_locking_timeout)
        if test_reference != False:
            test_successfully_issued = True
        
        # use next node for the next try
        next_node_to_issue_to = (next_node_to_issue_to + 1) % len(testing_nodes)
    
    # return the reference required to retrieve results later, contains node and report dir
    return test_reference
    


def wait_for_nodes_to_finish():
    global testing_nodes
    
    for node in testing_nodes:
        node.acquire_lock()
        node.release_lock()
        
def collect_reports_from_nodes():
    global testing_nodes
    global reports
    global test_references
    
    for test_reference in test_references:
        results = []
        for single_run in test_reference.single_runs:
            results.append(get_report_for_test(single_run))
            
            # Clean test (maybe preserve data instead?)
            node = single_run[0]
            node.run_command("rm -rf cloud_retest/%s" % test_reference)
        
        reports.append((test_reference.command, results))
    
    # Clean node
    for node in testing_nodes:
        cleanup_testing_node(node)
        
    terminate_testing_nodes()



# Safety stuff... (make sure that nodes get destroyed in EC2 eventually)
# This is not 100% fool-proof (id est does not catch all ways of killing the process), take care!
atexit.register(terminate_testing_nodes)



# modified variant of plain retester function...
# returns as soon as all repetitions of the test have been issued to some testing node
def do_test_cloud(cmd, cmd_args={}, cmd_format="gnu", repeat=1, timeout=60):
    global test_references
    
    # Build up the command line
    command = cmd
    for arg in cmd_args:
        command += " "
        # GNU cmd line builder
        if cmd_format == "gnu":
            if(isinstance(cmd_args[arg], types.BooleanType)):
                if cmd_args[arg]:
                    command += "--%s" % arg
            else:
                command += "--%s %s" % (arg, str(cmd_args[arg]))
        # Make cmd line builder
        elif cmd_format == "make":
            command += "%s=%s" % (arg, str(cmd_args[arg]))
        # Invalid cmd line builder
        else:
            print "Invalid command line formatter"
            raise NameError()
    
    # Run the test
    if repeat == 1: print "Running %r..." % command
    else: print "Running %r (repeating %d times)..." % (command, repeat)
    if timeout > 60: print "(This test may take up to %d seconds each time.)" % timeout
        
    test_reference = TestReference(command)
        
    for i in xrange(repeat):
        test_reference.single_runs.append(issue_test_to_some_node(command, timeout))
        
    test_references.append(test_reference)
        

# modified variant of plain retester function...
def report_cloud():
    # fill reports list
    collect_reports_from_nodes()

    # Invoke report() from plain retester to do the rest of the work...
    retester.reports.extend(reports)
    report()



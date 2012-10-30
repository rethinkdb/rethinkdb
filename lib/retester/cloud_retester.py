# Copyright 2010-2012 RethinkDB, all rights reserved.
import subprocess, shlex, signal, os, time, shutil, tempfile, sys, traceback, types, gitroot, random, atexit, stat
base_directory = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0])) + "/../test"
use_local_retester = os.getenv("USE_CLOUD", "false") == "false"

# The following functions are for external use: setup_testing_nodes(), terminate_testing_nodes(). do_test_cloud(), report_cloud()
#   + the following functions imported from retester are compatible and can be used in combination with cloud tests: do_test()

# In order to enable running tests in Amazon's EC2, set the USE_CLOUD environment variable

# Please configure in ec2_configuration.py!
from cloud_config import ec2_configuration
import cloud_node_data

testing_nodes_ec2_instance_type = ec2_configuration.testing_nodes_ec2_instance_type
testing_nodes_ec2_count = ec2_configuration.testing_nodes_ec2_count
testing_nodes_ec2_image_name = ec2_configuration.testing_nodes_ec2_image_name
testing_nodes_ec2_image_user_name = ec2_configuration.testing_nodes_ec2_image_user_name
testing_nodes_ec2_key_pair_name = ec2_configuration.testing_nodes_ec2_key_pair_name
testing_nodes_ec2_security_group_name = ec2_configuration.testing_nodes_ec2_security_group_name
testing_nodes_ec2_region = ec2_configuration.testing_nodes_ec2_region
testing_nodes_ec2_access_key = ec2_configuration.testing_nodes_ec2_access_key
testing_nodes_ec2_private_key = ec2_configuration.testing_nodes_ec2_private_key

private_ssh_key_filename = ec2_configuration.private_ssh_key_filename

round_robin_locking_timeout = 2
wrapper_script_filename = "cloud_retester_run_test_wrapper.py" # must be just the name of the file, no path!

# END of configuration options



from stat import *
import paramiko # Using Paramiko for SSH2
import boto, boto.ec2 # Using Boto for AWS commands

from vcoptparse import *
from retester import *
import retester

reports = []
test_references = []
testing_nodes_ec2_reservations = []
testing_nodes = []
remaining_nodes_to_allocate = testing_nodes_ec2_count

next_node_to_issue_to = 0
node_allocation_tries_count = 0

def put_file_compressed(ssh_transport, local_path, destination_path, retry = 3):
    gzip = None
    try:
        assert not not local_path
        assert not not destination_path
        session = ssh_transport.open_session()
        try:
            gzip = subprocess.Popen(["gzip", "-c", local_path], stdout=subprocess.PIPE)

            session = ssh_transport.open_session()
            session.exec_command('gzip -cd > "%s" && chmod %s "%s"\n' % (destination_path, oct(os.stat(local_path).st_mode)[-4:], destination_path))
            while True:
                buf = gzip.stdout.read(65536)
                if not buf:
                    break
                else:
                    session.sendall(buf)

            gzip.stdout.close()
        finally:
            session.close()
    except (IOError, EOFError, paramiko.SSHException, Exception) as e:
        if retry > 0:
            return put_file_compressed(ssh_transport, local_path, destination_path, retry-1)
        else:
            raise e
    finally:
        if gzip and gzip.poll() == None:
            gzip.kill()

def get_file_compressed(ssh_transport, remote_path, destination_path, retry = 3):
    gzip = None
    try:
        assert not not remote_path
        assert not not destination_path

        session = ssh_transport.open_session()
        try:
            with open(destination_path, "wb") as out:
                gzip = subprocess.Popen(["gzip", "-cd"], stdin=subprocess.PIPE, stdout=out)

                session = ssh_transport.open_session()
                session.exec_command('gzip -c "%s"\n' % remote_path)

                while True:
                    buf = session.recv(65536)
                    if not buf:
                        break
                    else:
                        gzip.stdin.write(buf)

                gzip.stdin.close()
                gzip.wait()
        finally:
            session.close()
    except (IOError, EOFError, paramiko.SSHException, Exception) as e:
        if retry > 0:
            return get_file_compressed(ssh_transport, remote_path, destination_path, retry-1)
        else:
            raise e
    finally:
        if gzip and gzip.poll() == None:
            gzip.kill()


class TestReference:
    def __init__(self, command):
        self.single_runs = []
        self.command = command
        self.results = []

class TestingNode:
    def __init__(self, hostname, port, username, private_ssh_key_filename):
        self.hostname = hostname
        self.port = port
        self.username = username
        
        #print "Created TestingNode with hostname %s, port %i, username %s" % (hostname, port, username)
        
        # read private key from file to get access to the node
        if True: # Always use RSA for now
            self.private_ssh_key = paramiko.RSAKey(filename=private_ssh_key_filename)
        else:
            self.private_ssh_key = paramiko.DSSKey(filename=private_ssh_key_filename)
            
        self.global_lock_file = "/tmp/cloudtest_lock"
    
        system_random = random.SystemRandom()    
        self.global_build_path = "/tmp/cloudtest_build_" + str(system_random.randint(10000000, 99999999));
        self.global_bench_path = "/tmp/cloudtest_bench_" + str(system_random.randint(10000000, 99999999));
        self.global_test_path = "/tmp/cloudtest_test_" + str(system_random.randint(10000000, 99999999));
        #print "Installing build into %s\n" % self.global_build_path
        
        self.basedata_installed = False
        
        self.ssh_transport = None
        
    def __del__(self):
        if self.ssh_transport != None:
            self.ssh_transport.close()

        
    def get_transport(self, retry = 3):
        if self.ssh_transport != None:
            return self.ssh_transport
    
        try:
            # open SSH transport
            self.ssh_transport = paramiko.Transport((self.hostname, self.port))
            self.ssh_transport.use_compression()
            self.ssh_transport.set_keepalive(60)
            self.ssh_transport.connect(username=self.username, pkey=self.private_ssh_key)
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            time.sleep(90) # Wait a bit in case the network needs time to recover
            if retry > 0:
                return self.get_transport(retry-1)
            else:
                raise e

        return self.ssh_transport
        
    
    # returns a tupel (return code, output)
    def run_command(self, command, retry = 3):
        ssh_transport = self.get_transport()
        
        try:
            # open SSH channel
            ssh_channel = ssh_transport.open_session()
            
            # issue the command to the node
            ssh_channel.exec_command(command)
            
            # read back command result:
            # do not timeout while reading (probably default anyway?)
            ssh_channel.settimeout(None)
            # read output until we get an EOF
            command_output = ""
            output_read = ssh_channel.recv(4096) # No do-while loops in Python?
            while len(output_read) > 0:
                command_output += output_read
                output_read = ssh_channel.recv(4096)
                
            # retrieve exit code
            command_exit_status = ssh_channel.recv_exit_status() # side effect: waits until command has finished
            
            ssh_channel.close()
            #self.ssh_transport.close()
            
            return (command_exit_status, command_output)
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            if retry > 0:
                return self.run_command(command, retry-1)
            else:
                raise e
           
        
    def put_file(self, local_path, destination_path, retry = 3):
        print "Sending file %r -> %r to cloud..." % (local_path, destination_path)
        put_file_compressed(self.get_transport(), local_path, destination_path, retry)
        #self.put_file_sftp(local_path, destination_path, retry)

    def put_file_sftp(self, local_path, destination_path, retry = 3):
        ssh_transport = self.get_transport()
        
        try:
            # open SFTP session
            sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
        
            # do the operation
            sftp_session.put(local_path, destination_path)
            sftp_session.chmod(destination_path, os.stat(local_path)[ST_MODE])
        
            sftp_session.close()
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            if retry > 0:
                return self.put_file(local_path, destination_path, retry-1)
            else:
                raise e
        
        
    def get_file(self, remote_path, destination_path, retry = 3):        
        print "Getting file %r -> %r from cloud..." % (remote_path, destination_path)
        get_file_compressed(self.get_transport(), remote_path, destination_path, retry)
        #self.get_file_sftp(remote_path, destination_path, retry)

    def get_file_sftp(self, remote_path, destination_path, retry = 3):
        ssh_transport = self.get_transport()
        
        try:
            # open SFTP session
            sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
            
            # do the operation
            sftp_session.get(remote_path, destination_path)
            
            sftp_session.close()
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            if retry > 0:
                return self.get_file(remote_path, destination_path, retry-1)
            else:
                raise e

    def get_file_gz(self, remote_path, destination_path, retry = 3):
        f = open(destination_path, "w")
        result = self.run_command("gzip -c \"%s\"" % remote_path) # TODO: Escape filename
        if not result[0] == 0:
            print "gzip returned an error"
            # TODO: Handle properly
            return
        f.write(result[1])
        f.close()


    def put_directory(self, local_path, destination_path, retry = 3):
        
        print "Sending directory %r to cloud..." % local_path
        
        ssh_transport = self.get_transport()
    
        try:
            # open SFTP session    
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
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            if retry > 0:
                return self.put_directory(local_path, destination_path, retry-1)
            else:
                raise e
    
    def list_directory(self, remote_path, retry = 3):
        
        ssh_transport = self.get_transport()
        try:
            sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
            list = sftp_session.listdir_attr(remote_path)
            sftp_session.close()
            return list
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            if retry > 0:
                return self.list_directory(remote_path, retry-1)
            else:
                raise e
        
    def make_directory(self, remote_path, retry = 3):
        ssh_transport = self.get_transport()

        try:
            # open SFTP session
            sftp_session = paramiko.SFTPClient.from_transport(ssh_transport)
            
            # do the operation
            sftp_session.mkdir(remote_path)
            
            sftp_session.close()
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            self.ssh_transport = None
            if retry > 0:
                return self.make_directory(remote_path, retry-1)
            else:
                raise e
        
        
    def make_directory_recursively(self, remote_path):
        # rely on mkdir command to do the work...
        mkdir_result = self.run_command("mkdir -p %s" % remote_path.replace(" ", "\\ "))
        
        if mkdir_result[0] != 0:
            print ("Unable to create directory")
            # TODO: Throw exception or something,,,
            
            
    def acquire_lock(self, locking_timeout = 0):
        lock_sleeptime = 1
        lock_command = "lockfile -%i -r -1 %s" % (lock_sleeptime, self.global_lock_file.replace(" ", "\' "))
        if locking_timeout > 0:
            lock_command = "lockfile -%i -r %i %s" % (lock_sleeptime, locking_timeout / lock_sleeptime, self.global_lock_file.replace(" ", "\' "))
        locking_result = self.run_command(lock_command)
        
        return locking_result[0] == 0
        
    def get_release_lock_command(self):
        return "rm -f %s" % self.global_lock_file.replace(" ", "\' ")
        
    def release_lock(self):
        command_result = self.run_command(self.get_release_lock_command())
        if command_result[0] != 0:
            print "Unable to release lock (maybe the node wasn't locked before?). Ignoring this."


def create_testing_nodes_from_reservation(ec2_reservation):
    global testing_nodes
    global testing_nodes_ec2_image_user_name
    global private_ssh_key_filename
    
    for instance in ec2_reservation.instances:
        if instance.state == "running":
            new_testing_node = TestingNode(instance.public_dns_name, 22, testing_nodes_ec2_image_user_name, private_ssh_key_filename)
            testing_nodes.append(new_testing_node)



def setup_testing_nodes():
    global testing_nodes
    global use_local_retester
    
    if use_local_retester:
        return

    start_testing_nodes()

def start_testing_nodes():
    global testing_nodes
    global testing_nodes_ec2_reservations
    global remaining_nodes_to_allocate
    global testing_nodes_ec2_image_name
    global testing_nodes_ec2_instance_type
    global testing_nodes_ec2_key_pair_name
    global testing_nodes_ec2_security_group_name
    global testing_nodes_ec2_region
    global testing_nodes_ec2_access_key
    global testing_nodes_ec2_private_key
    global node_allocation_tries_count
    
    if remaining_nodes_to_allocate == 0:
        return
    
    # Reserve nodes in EC2
    
    print "Trying to allocate %i testing nodes" % remaining_nodes_to_allocate
    
    try:
        ec2_connection = boto.ec2.connect_to_region(testing_nodes_ec2_region, aws_access_key_id=testing_nodes_ec2_access_key, aws_secret_access_key=testing_nodes_ec2_private_key)

        # Query AWS to start all instances
        ec2_image = ec2_connection.get_image(testing_nodes_ec2_image_name)
        ec2_reservation = ec2_image.run(min_count=1, max_count=remaining_nodes_to_allocate, key_name=testing_nodes_ec2_key_pair_name, security_groups=[testing_nodes_ec2_security_group_name], instance_type=testing_nodes_ec2_instance_type)
        testing_nodes_ec2_reservations.append(ec2_reservation)
        # query AWS to wait for all instances to be available
        for instance in ec2_reservation.instances:
            while instance.state != "running":
                time.sleep(5)
                instance.update()
                if instance.state == "terminated":
                    # Something went wrong :-(
                    print "Could not allocate the requested number of nodes. Retrying later..."
                    break
            # Got a node running
            remaining_nodes_to_allocate -= 1
        
        create_testing_nodes_from_reservation(ec2_reservation)
        
        # Give it another 120 seconds to start up...
        time.sleep(120)
    except (Exception) as e:
        print "An exception occured while trying to request a node from EC: \n%s" % e
        
    
    # Check that all testing nodes are up
    nodes_to_remove = []
    for node in testing_nodes:
        # send a testing command
        try:
            command_result = node.run_command("echo -n Are you up?")
            if command_result[1] != "Are you up?":
                print "Node %s is misfunctioning." % node.hostname
                nodes_to_remove.append(node)
            else:
                print "Node %s is up" % node.hostname
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            print "Node %s is not responding." % node.hostname
            nodes_to_remove.append(node)
    for node_to_remove in nodes_to_remove:
        testing_nodes.remove(node_to_remove)
        remaining_nodes_to_allocate += 1
        
    if len(testing_nodes) == 0:
        terminate_testing_nodes()
        node_allocation_tries_count += 1
        if node_allocation_tries_count > 5:
            raise Exception("Could not allocate any testing nodes after %d tries." % node_allocation_tries_count)
        else:
            print "Could not allocate any nodes, retrying..."
            time.sleep(120)
            start_testing_nodes()
            return


def terminate_testing_nodes():
    global testing_nodes
    global testing_nodes_ec2_reservations
    global testing_nodes_ec2_region
    global testing_nodes_ec2_access_key
    global testing_nodes_ec2_private_key

    for testing_nodes_ec2_reservation in testing_nodes_ec2_reservations:
        if testing_nodes_ec2_reservation:
            print "Terminating EC2 nodes"
        
            ec2_connection = boto.ec2.connect_to_region(testing_nodes_ec2_region, aws_access_key_id=testing_nodes_ec2_access_key, aws_secret_access_key=testing_nodes_ec2_private_key)
        
            # Query AWS to stop all instances   
            testing_nodes_ec2_reservation.stop_all()
    
    testing_nodes_ec2_reservations = []
    testing_nodes = None


def cleanup_testing_node(node):
    node.run_command("rm -rf " + node.global_build_path)
    node.run_command("rm -rf " + node.global_bench_path)
    node.run_command("rm -rf " + node.global_test_path)


def scp_basedata_to_testing_node(source_node, target_node):
    # Put private SSH key to source_node...
    source_node.run_command("rm -f private_ssh_key.pem")
    source_node.put_file(private_ssh_key_filename, "private_ssh_key.pem")
    command_result = source_node.run_command("chmod 500 private_ssh_key.pem")
    if command_result[0] != 0:
        print "Unable to change access mode of private SSH key on remote node"
        
    # Scp stuff to target node
    for path_to_copy in [("/tmp/cloudtest_libs", "/tmp/cloudtest_libs"), ("/tmp/cloudtest_bin", "/tmp/cloudtest_bin"), ("/tmp/cloudtest_python", "/tmp/cloudtest_python"), (source_node.global_build_path, target_node.global_build_path), (source_node.global_bench_path, target_node.global_bench_path), (source_node.global_test_path, target_node.global_test_path)]:
        command_result = source_node.run_command("scp -r -C -q -o stricthostkeychecking=no -P %i -i private_ssh_key.pem %s %s@%s:%s" % (target_node.port, path_to_copy[0], target_node.username, target_node.hostname, path_to_copy[1]))
        if command_result[0] != 0:
            print "Failed using scp to copy data from %s to %s: %s" % (source_node.hostname, target_node.hostname, command_result[1])
            return False
            
    target_node.basedata_installed = True
    return True


def copy_basedata_to_testing_node(node):
    global testing_nodes

    print "Sending base data to node %s" % node.hostname
   
    # Check if we can use scp_basedata_to_testing_node instead:
    for source_node in testing_nodes:
        if source_node.basedata_installed:
            print "Scp-ing base data from source node " + source_node.hostname
            if scp_basedata_to_testing_node(source_node, node):
                return

    node.basedata_installed = True
 
    # Copy dependencies as specified in ec2_configuration        
    node.make_directory_recursively("/tmp/cloudtest_libs")
    for (source_path, target_path) in ec2_configuration.cloudtest_lib_dependencies:
        node.make_directory_recursively("/tmp/cloudtest_libs/" + os.path.dirname(target_path))
        node.put_file(source_path, "/tmp/cloudtest_libs/" + target_path)
    
    node.make_directory_recursively("/tmp/cloudtest_bin")
    for (source_path, target_path) in ec2_configuration.cloudtest_bin_dependencies:
        node.make_directory_recursively("/tmp/cloudtest_bin/" + os.path.dirname(target_path))
        node.put_file(source_path, "/tmp/cloudtest_bin/" + target_path)
    command_result = node.run_command("chmod +x /tmp/cloudtest_bin/*")
    if command_result[0] != 0:
        print "Unable to make cloudtest_bin files executable"
    
    node.make_directory_recursively("/tmp/cloudtest_python")
    for (source_path, target_path) in ec2_configuration.cloudtest_python_dependencies:
        node.make_directory_recursively("/tmp/cloudtest_python/" + os.path.dirname(target_path))
        node.put_file(source_path, "/tmp/cloudtest_python/" + target_path)
    
    # Copy build hierarchy
    node.make_directory(node.global_build_path)
    #node.put_directory(base_directory + "/../build", node.global_build_path)
    # Just copy essential files to save time...
    for config in os.listdir(base_directory + "/../build"):
        if os.path.isdir(base_directory + "/../build/" + config):
            try:
                node.make_directory(node.global_build_path + "/" + config)
                node.put_file(base_directory + "/../build/" + config + "/rethinkdb", node.global_build_path + "/" + config + "/rethinkdb")
                #node.put_file(base_directory + "/../build/" + config + "/rethinkdb-extract", node.global_build_path + "/" + config + "/rethinkdb-extract")
                #node.put_file(base_directory + "/../build/" + config + "/rethinkdb-fsck", node.global_build_path + "/" + config + "/rethinkdb-fsck")
                command_result = node.run_command("chmod +x " + node.global_build_path + "/" + config + "/*")
                if command_result[0] != 0:
                    print "Unable to make rethinkdb executable"
            except:
                print "RethinkDB configuration %s could not be installed" % config
        
    # Copy benchmark stuff
    node.make_directory(node.global_bench_path)
    node.make_directory(node.global_bench_path + "/stress-client")
    node.put_file(base_directory + "/../bench/stress-client/stress", node.global_bench_path + "/stress-client/stress")
    command_result = node.run_command("chmod +x " + node.global_bench_path + "/*/*")
    if command_result[0] != 0:
        print "Unable to make bench files executable"
    try:
        node.put_file(base_directory + "/../bench/stress-client/libstress.so", node.global_bench_path + "/stress-client/libstress.so")
        node.put_file(base_directory + "/../bench/stress-client/stress.py", node.global_bench_path + "/stress-client/stress.py")
    except Exception as e:
        print "Failed copying stress auxiliary files: %s" % e
    
    # Copy test hierarchy
    node.make_directory(node.global_test_path)
    node.put_directory(base_directory, node.global_test_path)
    
    # Install the wrapper script
    node.put_file(os.path.dirname(cloud_node_data.__file__) + "/" + wrapper_script_filename, "%s/%s" % (node.global_test_path, wrapper_script_filename));



def copy_per_test_data_to_testing_node(node, test_reference):    
    # Link build hierarchy
    command_result = node.run_command("ln -s %s cloud_retest/%s/build" % (node.global_build_path, test_reference))
    if command_result[0] != 0:
        print "Unable to link build environment"
        raise Exception("Unable to link build environment")
        
    # Link bench hierarchy
    command_result = node.run_command("ln -s %s cloud_retest/%s/bench" % (node.global_bench_path, test_reference))
    if command_result[0] != 0:
        print "Unable to link bench environment"
        raise Exception("Unable to link bench environment")
    
    # copy over the global test hierarchy
    node.make_directory_recursively("cloud_retest/%s/test" % test_reference)    
    command_result = node.run_command("cp -af %s/* cloud_retest/%s/test" % (node.global_test_path, test_reference))
    if command_result[0] != 0:
        print "Unable to copy test environment"
        raise Exception("Unable to copy test environment")

def retrieve_results_from_node(node):
    global testing_nodes
    global reports
    global test_references

    for test_reference in test_references:
        single_runs_to_remove = []
        for single_run in test_reference.single_runs:
            run_node = single_run[0]
            if run_node == node:
                try:
                    test_reference.results.append(get_report_for_test(single_run))
                    single_runs_to_remove.append(single_run)
                    
                    # Clean test
                    run_node.run_command("rm -rf cloud_retest/%s" % single_run[1])
                except (IOError, EOFError, paramiko.SSHException, Exception) as e:
                    print "Unable to retrieve result for %s from node %s:" % (single_run[1], single_run[0].hostname)
                    traceback.print_exc()
                    
        for single_run_to_remove in single_runs_to_remove:
            test_reference.single_runs.remove(single_run_to_remove)


def start_test_on_node(node, test_command, test_timeout = None, locking_timeout = 0):
    global testing_nodes
    global remaining_nodes_to_allocate

    if locking_timeout == None:
        locking_timeout = 0

    #print ("trying to acquire lock with timeout %i" % locking_timeout)
    if node.acquire_lock(locking_timeout) == False:
        return False
    #print ("Got lock!")
    
    # Check if we can retrieve previous test results for this node now
    retrieve_results_from_node(node)
    
    try:
        # Initialize node if not happened before...
        if node.basedata_installed == False:
            copy_basedata_to_testing_node(node)

        # Generate random reference
        system_random = random.SystemRandom()
        test_reference = "cloudtest_" + str(system_random.randint(10000000, 99999999))
        
        # Create test directory and check that it isn't taken
        directory_created = False
        while not directory_created:
            node.make_directory_recursively("cloud_retest")
            try:
                node.make_directory("cloud_retest/%s" % test_reference)
                directory_created = True
            except IOError:
                directory_created = False
                test_reference = "cloudtest_" + str(system_random.randint(10000000, 99999999)) # Try another reference
        
        print "Starting test with test reference %s on node %s" % (test_reference, node.hostname)
        
        # Prepare for test...
        copy_per_test_data_to_testing_node(node, test_reference)
        # Store test_command and test_timeout into files on the remote node for the wrapper script to pick it up
        command_result = node.run_command("echo -n %s > cloud_retest/%s/test/test_command" % \
            (retester.shell_escape(test_command), test_reference))
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
        command_result = node.run_command("sh -c \"nohup sh -c \\\"(cd %s; LD_LIBRARY_PATH=/tmp/cloudtest_libs:$LD_LIBRARY_PATH PATH=/tmp/cloudtest_bin:$PATH PYTHONPATH=/tmp/cloudtest_python:$PYTHONPATH VALGRIND_LIB=/tmp/cloudtest_libs/valgrind python %s; %s)&\\\" > /dev/null 2> /dev/null\"" % ("cloud_retest/%s/test" % test_reference, wrapper_script_filename.replace(" ", "\\ "), node.get_release_lock_command()))
            
    except (IOError, EOFError, paramiko.SSHException, Exception) as e:
        print "Starting test failed: %s" % e
        test_reference = "Failed"
        
        try:
            node.release_lock()
        except (IOError, EOFError, paramiko.SSHException, Exception):
            print "Unable to release lock on node %s. Node is now defunct." % node.hostname
            testing_nodes.remove(node)
            remaining_nodes_to_allocate += 1
        
    return (node, test_reference)

# TODO: Move budget to configuration file
gz_budget_left = 512 * 1024 * 1024 # 512 MiB 
def get_report_for_test(test_reference):
    print "Downloading results for test %s" % test_reference[1]
    
    if test_reference[1] == "Failed":
        result = Result(0.0, "fail", "The test could not be started on EC2.")
        return result

    node = test_reference[0]
    result_result = node.run_command("cat cloud_retest/" + test_reference[1] + "/test/result_result")[1]
    result_description = node.run_command("cat cloud_retest/" + test_reference[1] + "/test/result_description")[1]
    if result_description == "":
        result_description = None
    
    result = Result(0.0, result_result, result_description)
    
    # Get running time
    try:
        result.running_time = float(node.run_command("cat cloud_retest/" + test_reference[1] + "/test/result_running_time")[1])
    except ValueError:
        print "Got invalid start_time for test %s" % test_reference[1]
        result.running_time = 0.0
    
    # Collect a few additional results into a temporary directory
    result.output_dir = SmartTemporaryDirectory("out_")
    
    def get_directory(remote_path, destination_path):
        global gz_budget_left
        assert os.path.isdir(destination_path)
        for file in node.list_directory(remote_path):
            max_file_size = 100000
            r_path = os.path.join(remote_path, file.filename)
            d_path = os.path.join(destination_path, file.filename)
            assert not os.path.exists(d_path)
            if stat.S_ISDIR(file.st_mode):
                os.mkdir(d_path)
                get_directory(r_path, d_path)
            elif file.st_size <= max_file_size:
                node.get_file(r_path, d_path)
            elif gz_budget_left > 0:
                # Retrieve complete gzipped file as long as we have some budget left for this
                node.get_file_gz(r_path, "%s.gz" % d_path)
                gz_budget_left -= (os.stat("%s.gz" % d_path))[ST_SIZE]
            else:
                f = open(d_path, "w")
                res = node.run_command("head -c %d \"%s\"" % (max_file_size / 2, r_path))
                if res[0] == 0: f.write(res[1])
                else: f.write("[cloud_retester failed to retrieve part of this file: %s]" % res[0])
                f.write("\n\n[cloud_retester omitted %d bytes of this file]\n\n" % (file.st_size - max_file_size + (max_file_size % 2)))
                res = node.run_command("tail -c %d \"%s\"" % (max_file_size / 2, r_path))
                if res[0] == 0: f.write(res[1])
                else: f.write("[cloud_retester failed to retrieve part of this file: %s]" % res[0])
                f.close()
    
    if result_result == "fail":
        get_directory(os.path.join("cloud_retest", test_reference[1], "test", "output_from_test"), result.output_dir.path)
    else:
        for file_name in ["server_output.txt", "creator_output.txt", "test_output.txt", "fsck_output.txt"]:
            command_result = node.run_command("cat 'cloud_retest/" + test_reference[1] + "/test/output_from_test/" + file_name + "'")
            if command_result[0] == 0:
                open(result.output_dir.path + "/" + file_name, 'w').write(command_result[1])
    
    return result


def issue_test_to_some_node(test_command, test_timeout = 0):
    global testing_nodes
    global next_node_to_issue_to
    global round_robin_locking_timeout
    
    # Start remaining nodes
    setup_testing_nodes()    
    
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
    
    print "Waiting for testing nodes to finish"
    
    for node in testing_nodes:
        try:
            node.acquire_lock()
            node.release_lock()
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            print "Node %s is broken" % node.hostname


def collect_reports_from_nodes():
    global testing_nodes
    global reports
    global test_references
    
    print "Collecting reports"
    
    for test_reference in test_references:
        single_runs_to_remove = []
        for single_run in test_reference.single_runs:
            try:
                test_reference.results.append(get_report_for_test(single_run))
                single_runs_to_remove.append(single_run)
            
                # Clean test
                node = single_run[0]
                node.run_command("rm -rf cloud_retest/%s" % single_run[1])
            except (IOError, EOFError, paramiko.SSHException, Exception) as e:
                print "Unable to retrieve result for %s from node %s:" % (single_run[1], single_run[0].hostname)
                traceback.print_exc()
                
        for single_run_to_remove in single_runs_to_remove:
            test_reference.single_runs.remove(single_run_to_remove)
        
        # Generate report object for all results of this test
        reports.append((test_reference.command, test_reference.results))
    
    # Clean node
    for node in testing_nodes:
        try:
            cleanup_testing_node(node)
        except (IOError, EOFError, paramiko.SSHException, Exception) as e:
            print "Unable to cleanup node %s:" % node.hostname
            traceback.print_exc()
        
    terminate_testing_nodes()



# Safety stuff... (make sure that nodes get destroyed in EC2 eventually)
# This is not 100% fool-proof (id est does not catch all ways of killing the process), take care!
atexit.register(terminate_testing_nodes)



# modified variant of plain retester function...
# returns as soon as all repetitions of the test have been issued to some testing node
def do_test_cloud(cmd, cmd_args={}, cmd_format="gnu", repeat=1, timeout=60):
    global test_references
    global use_local_retester
    
    if use_local_retester:
        return do_test(cmd, cmd_args, cmd_format, repeat, timeout)
    
    # Build up the command line
    command = cmd
    cmd_args_keys = [k for k in cmd_args]
    cmd_args_keys.sort()
    for arg in cmd_args_keys:
        command += " "
        # GNU cmd line builder
        if cmd_format == "gnu":
            if(isinstance(cmd_args[arg], types.BooleanType)):
                if cmd_args[arg]:
                    command += "--%s" % arg
            else:
                command += "--%s \"%s\"" % (arg, retester.shell_escape(str(cmd_args[arg])))
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
    global use_local_retester
    if use_local_retester:
        return report()

    wait_for_nodes_to_finish()

    # fill reports list
    collect_reports_from_nodes()

    # Invoke report() from plain retester to do the rest of the work...
    retester.reports.extend(reports)
    report()



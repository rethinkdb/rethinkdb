# Copyright 2010-2012 RethinkDB, all rights reserved.
import os, sys
import cloud_config

base_directory = os.path.dirname(os.path.join(os.getcwd(), sys.argv[0])) + "/../test"

cloud_config_path = "/usr/local/cloud_config"
#cloud_config_path = os.path.join (sys.prefix, "cloud_config") # sys.prefix does not match setup directory for some reason

# Please configure these:

testing_nodes_ec2_instance_type = os.getenv("EC2_INSTANCE_TYPE", "m1.large") # e.g. m1.large / t1.micro
testing_nodes_ec2_count = int(os.getenv("EC2_INSTANCE_COUNT", "5")) # number of nodes to spin up
#testing_nodes_ec2_image_name = "ami-2272864b" <-- ebs store
testing_nodes_ec2_image_name = "ami-827185eb" # <-- instance store
testing_nodes_ec2_image_user_name = "ec2-user"
testing_nodes_ec2_key_pair_name = "cloudtest_default"
testing_nodes_ec2_security_group_name = "cloudtest_default"
testing_nodes_ec2_region = "us-east-1"
testing_nodes_ec2_access_key = "AKIAJUKUVO6J45QRZQKA"
testing_nodes_ec2_private_key = "d9SiQpDD/YfGA2uC7CyqY7jmRoZg5utHM4pxTAhE"

private_ssh_key_filename = os.path.join (cloud_config_path, "ec2_private_key.pem")

# END of configuration options

# Dependencies to automatically install on the cloud nodes

cloudtest_lib_dependencies = [("/usr/local/lib/libtcmalloc_minimal.so.0", "libtcmalloc_minimal.so.0"),
                              ("/usr/local/lib/libmemcached.so.5", "libmemcached.so.5"),
                              ("/usr/local/lib/libmemcached.so.6", "libmemcached.so.6"),
                              ("/usr/local/lib/libgtest.so.0", "libgtest.so.0"),
                              ("/usr/lib/libstdc++.so.6", "libstdc++.so.6"),
                              ("/usr/lib/libgccpp.so.1", "libgccpp.so.1"),
                              ("/usr/lib/libQtCore.so.4", "libQtCore.so.4")]
for valgrind_file in ["vgpreload_memcheck-amd64-linux.so", "vgpreload_core-amd64-linux.so", "memcheck-amd64-linux", "drd-amd64-linux", "helgrind-amd64-linux"]:
    cloudtest_lib_dependencies.append(("/usr/lib/valgrind/" + valgrind_file, "valgrind/" + valgrind_file))
cloudtest_lib_dependencies.append((os.path.join (cloud_config_path, "valgrind-default.supp"), "valgrind/default.supp"))

cloudtest_bin_dependencies = [("/usr/local/bin/netrecord", "netrecord"),
        ("/usr/bin/valgrind", "valgrind"),
        ("/usr/bin/valgrind.bin", "valgrind.bin"),
        ("/usr/lib/libstdc++.so.6", "libstdc++.so.6"),
        ("/usr/lib/libgccpp.so.1", "libgccpp.so.1")]
        
cloudtest_python_dependencies = [(base_directory + "/../lib/vcoptparse/vcoptparse.py", "vcoptparse.py"),
        (base_directory + "/../lib/rethinkdb_memcache/rethinkdb_memcache.py", "rethinkdb_memcache.py"),
        ("/usr/local/lib/python2.6/dist-packages/pylibmc.py", "pylibmc.py"),
        ("/usr/local/lib/python2.6/dist-packages/_pylibmc.so", "_pylibmc.so")]

# END of dependencies

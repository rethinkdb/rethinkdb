#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.

from distutils.core import setup

setup(
    name = "retester",
    version = "1.0",
    description = "Automated test driver",
    py_modules=["retester", "gitroot"],
    scripts = ["git-test"]
    )
    
setup(
    name = "cloud retester",
    version = "1.0",
    description = "Automated test driver for EC2 clouds",
    py_modules=["retester", "gitroot", "cloud_retester"],
    packages=['cloud_config', 'cloud_node_data'],
    package_dir={'cloud_config': 'cloud_config', 'cloud_node_data': 'cloud_node_data'},
    package_data={'cloud_config': ['cloud_config/*.*'], 'cloud_node_data': ['cloud_node_data/*.*']},
    data_files=[("cloud_config", ["cloud_config/valgrind-default.supp", "cloud_config/ec2_private_key.pem"])],
    scripts = ["git-test"]
    )

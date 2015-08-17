from __future__ import print_function

from os.path import abspath, join
from subprocess import check_call, CalledProcessError
from os import environ
from sys import stderr

from test_framework import Test

class ShellCommandTest(Test):
    def __init__(self, command, env={}, **kwargs):
        Test.__init__(self, **kwargs)
        self.command = command
        self.env = env

    def configure(self, conf):
        env = self.env.copy()
        env.update({
            'RETHINKDB': abspath(join(conf['SRC_ROOT'])),
            'RETHINKDB_BUILD_DIR': abspath(conf['BUILD_DIR']),
            'PYTHONUNBUFFERED': 'true',
        })
        return ShellCommandTest(self.command, env)

    def run(self):
        print("Running shell command:", self.command)
        for k in self.env:
            print(k, "=", self.env[k])
        env = environ.copy()
        env.update(self.env)
        check_call(self.command, shell=True, env=env)

    def __str__(self):
        return self.command

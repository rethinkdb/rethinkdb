#!/usr/bin/env python
#
# This script helps to check the status of certain pre-release tasks

from __future__ import print_function

from os import path
from re import match, search, findall, MULTILINE, DOTALL
from subprocess import check_output
from sys import exit
from weakref import WeakKeyDictionary

class SmartGetitem(object):
    def __getitem__(self, name):
        if name == 'self':
            return self
        return getattr(self, name)

def cache(f):
    box = WeakKeyDictionary()
    def get(self):
        if self in box:
            return box[self]
        box[self] = None
        kwargs = { name: self[name] for name in f.func_code.co_varnames[0:f.func_code.co_argcount] }
        x = f(**kwargs)
        box[self] = x
        return x
    return property(get)

class Repo(SmartGetitem):
    def __init__(self, root):
        self.root = root

    @cache
    def head(self):
        return Ref(repo=self, ref='HEAD')

    @cache
    def prior(head):
        return head.prior

class Ref(SmartGetitem):

    def __init__(self, repo, ref):
        self.repo = repo
        self.ref = ref

    @cache
    def prior(repo, prior_tag):
        return Ref(repo=repo, ref=prior_tag)

    @cache
    def prior_tag(self, ref):
        return self.git("describe", "--tags", "--match", "v[0-9]*", "--abbrev=0", ref + "^")

    @cache
    def version(self, dirty, ref):
        tag = self.git("describe", "--tags", "--match", "v[0-9]*", "--abbrev=6", ref)
        version = tag.lstrip('v')
        if dirty:
            version = version + '-dirty'
        return version

    @cache
    def dirty(self, ref):
        if ref != 'HEAD':
            return False
        self.git("update-index", "-q", "--refresh")
        return "" != self.git("diff-index","--name-only","HEAD","--")

    def git(self, cmd, *args, **kwargs):
        if not kwargs.get('shell'):
            return check_output(["git", "--git-dir=" + self.repo.root + "/.git", cmd] + list(args)).rstrip('\n')
        else:
            assert args == []
            return check_output(["git --git-dir='" + self.repo.root + "/.git' " + cmd], shell=True).rstrip('\n')

    def git_show(self, path):
        return self.git("show", self.ref + ":" + path)

    def extract_version_macro(self, path, type):
        args_hpp = self.git_show(path)
        res = search('^#define %s_VERSION_STRING "(.*)"' % type.upper(), args_hpp, flags=MULTILINE)
        if not res:
            raise Exception("Missing %s version in %s:%s" % (path, self.ref, path))
        return res.groups()[0]

    @cache
    def serializer_version(self):
        return self.extract_version_macro("src/config/args.hpp", 'serializer')

    @cache
    def cluster_version(self, ref):
        return self.extract_version_macro("src/rpc/connectivity/cluster.cc", 'cluster')

    @cache
    def driver_magic_number(self, ref):
        path = 'src/rdb_protocol/ql2.proto'
        ql2_proto = self.git_show(path)
        res = search('enum Version {(.*?)}', ql2_proto, flags=DOTALL)
        if not res:
            raise Exception("Missing enum Version in %s:%s" % (ref, path))
        numbers = findall("(V.*?) += (.*?);", res.groups()[0])
        return numbers[-1][0]

    @cache
    def notes_version(self, ref):
        return match('# Release (.*?) ', self.git_show('NOTES.md')).groups()[0]

def info(description, *args):
    print(description + ":", *args)

def info_compare(description, name):
    cur = repo.head[name]
    prev = repo.prior[name]
    prior_version = repo.prior.version
    info(description, prev, '->', cur)

def report():
    info_compare("Refs", 'ref')
    info_compare("Version", 'version')
    info_compare("Serializer version", 'serializer_version')
    info_compare("Cluster version", 'cluster_version')
    info_compare("Driver magic number", 'driver_magic_number')
    info_compare("Notes version", 'notes_version')

repo = Repo(path.normpath(path.join(path.dirname(__file__), path.pardir)))

if __name__ == '__main__':
    report()

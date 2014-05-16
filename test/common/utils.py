#!/usr/bin/env python

import os

import test_exceptions

def module(module):
    __import__(module, level=0)
    return sys.modules[module]

# non-printable ascii characters and invalid utf8 bytes
non_text_bytes = \
  range(0x00, 0x09+1) + [0x0B, 0x0C] + range(0x0F, 0x1F+1) + \
  [0xC0, 0xC1] + range(0xF5, 0xFF+1)

def guess_is_text_file(name):
    with file(name, 'rb') as f:
        data = f.read(100)
    for byte in data:
        if ord(byte) in non_text_bytes:
            return False
    return True

def project_root_dir():
    '''Return the root directory for this project'''
    
    # warn: hard-coded both for location of this file and the name of the build dir
    masterBuildDir = os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir)
    if not os.path.isdir(masterBuildDir):
        raise Exception('The project build directory does not exist where expected: %s' % str(masterBuildDir))
    
    return os.path.realpath(masterBuildDir)

def latest_build_dir(check_executable=True):
    '''Look for the most recently built version of this project'''
    
    masterBuildDir = os.path.join(project_root_dir(), 'build')
    
    # -- find the build directory with the most recent mtime
    
    canidatePath    = None
    canidateMtime   = None
    for name in os.listdir(masterBuildDir):
        path = os.path.join(masterBuildDir, name)
        if os.path.isdir(path) and (name in ('release', 'debug') or name.startswith('debug_') or name.startswith('release_')):
            if check_executable == True:
                if not os.path.isfile(os.path.join(path, 'rethinkdb')):
                    continue
            
            mtime = os.path.getmtime(path)
            if canidateMtime is None or mtime > canidateMtime:
                canidateMtime = mtime
                canidatePath = path
    
    if canidatePath is None:
        raise test_exceptions.NotBuiltException(detail='no version of this project have yet been built')
    
    else:
        return canidatePath

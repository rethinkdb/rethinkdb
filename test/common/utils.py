#!/usr/bin/env python

import os, subprocess, sys, tempfile, threading, time

import test_exceptions

# -- constants

driverPaths = {
    'javascript': {'extension':'js', 'relDriverPath':'build/packages/js', 'relSourcePath':'drivers/javascript'},
    'python': {'extension':'py', 'relDriverPath':'build/drivers/python/rethinkdb', 'relSourcePath':'drivers/python'},
    'ruby': {'extension':'rb', 'relDriverPath':'build/drivers/ruby/lib', 'relSourcePath':'drivers/ruby'}
}

# --

# non-printable ascii characters and invalid utf8 bytes
non_text_bytes = \
  list(range(0x00, 0x09+1)) + [0x0B, 0x0C] + list(range(0x0F, 0x1F+1)) + \
  [0xC0, 0xC1] + list(range(0xF5, 0xFF+1))

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

def find_rethinkdb_executable(mode=None):
    return os.path.join(latest_build_dir(check_executable=True, mode=mode), 'rethinkdb')

def latest_build_dir(check_executable=True, mode=None):
    '''Look for the most recently built version of this project'''
    
    canidatePath = None
    
    if os.getenv('RETHINKDB_BUILD_DIR') is not None:
        canidatePath = os.path.realpath(os.getenv('RETHINKDB_BUILD_DIR'))
    
    else:
        masterBuildDir = os.path.join(project_root_dir(), 'build')
        if not os.path.isdir(masterBuildDir):
            raise test_exceptions.NotBuiltException(detail='no version of this project has yet been built')
        
        if mode in (None, ''):
            mode = ['release', 'debug']
        elif not hasattr(mode, '__iter__'):
            mode = [str(mode)]
        
        # -- find the build directory with the most recent mtime
        
        canidateMtime = None
        for name in os.listdir(masterBuildDir):
            path = os.path.join(masterBuildDir, name)
            if os.path.isdir(path) and any(map(lambda x: name.startswith(x + '_') or name.lower() == x, mode)):
                if check_executable == True:
                    if not os.path.isfile(os.path.join(path, 'rethinkdb')):
                        continue
                
                mtime = os.path.getmtime(path)
                if canidateMtime is None or mtime > canidateMtime:
                    canidateMtime = mtime
                    canidatePath = path
        
        if canidatePath is None:
            raise test_exceptions.NotBuiltException(detail='no built version of the server could be found')
    
    if canidatePath is None or (check_executable is True and not os.access(os.path.join(canidatePath, 'rethinkdb'), os.X_OK)):
        raise test_exceptions.NotBuiltException(detail='the rethinkdb server executable was not present/runable in: %s' % canidatePath)
    
    return canidatePath

def build_in_folder(targetFolder, waitNotification=None, notificationTimeout=2, buildOptions=None):
    '''Call `make -C` on a folder to build it. If waitNotification is given wait notificationTimeout seconds and then print the notificaiton'''
    
    outputFile = tempfile.NamedTemporaryFile()
    notificationDeadline = None
    if waitNotification not in ("", None):
        notificationDeadline = time.time() + notificationTimeout
    
    makeProcess = subprocess.Popen(['make', '-C', targetFolder], stdout=outputFile, stderr=subprocess.STDOUT)
    
    if notificationDeadline is not None:
        while makeProcess.poll() is None and time.time() < notificationDeadline:
            time.sleep(.1)
        if time.time() > notificationDeadline:
           print(waitNotification)
    
    if makeProcess.wait() != 0:
        raise test_exceptions.NotBuiltException(detail='Failed making: %s' % targetFolder, debugInfo=outputFile)

def import_python_driver(targetDir=None):
    '''import the latest built version of the python driver into the caller's namespace, ensuring that the drivers are built'''
    import imp # note: depreciated but not gone in 3.4, will have to add importlib at some point
    
    # TODO: modify this to allow for system-installled drivers
    
    # -- figure out what sort of path we got
    
    if targetDir is None:
        if 'PYTHON_DRIVER_DIR' in os.environ:
            targetDir = os.environ['PYTHON_DRIVER_DIR']
        elif 'PYTHON_DRIVER_SRC_DIR' in os.environ:
            targetDir = os.environ['PYTHON_DRIVER_SRC_DIR']
        else:
            targetDir = project_root_dir()
    
    driverDir = None
    srcDir = None
    
    if not os.path.isdir(targetDir):
        raise ValueError('import_python_driver got a non-directory path: %s' % str(targetDir))
    targetDir = os.path.realpath(targetDir)
    
    validSourceFolder = lambda path: os.path.basename(path) == 'rethinkdb' and all(map(lambda x: os.path.isfile(os.path.join(path, x)), ['__init__.py', 'ast.py', 'docs.py']))
    builtDriver = lambda path: validSourceFolder(path) and os.path.isfile(os.path.join(path, 'ql2_pb2.py'))
    
    # normalize path
    if not os.path.dirname(targetDir) == 'rethinkdb' and os.path.isdir(os.path.join(targetDir, 'rethinkdb')):
        targetDir = os.path.join(targetDir, 'rethinkdb')
    
    # - project directory
    if all(map(lambda x: os.path.isdir(os.path.join(targetDir, x)), ['src', 'drivers', 'admin'])):
        buildDriver = True
        driverDir = os.path.join(targetDir, driverPaths['python']['relDriverPath'])
        srcDir = os.path.join(targetDir, driverPaths['python']['relSourcePath'])
    
    # - built driver - it does not matter if this is source, build, or installed, it looks complete
    elif builtDriver(targetDir):
        buildDriver = False
        driverDir = targetDir
        srcDir = None
    
    # - source folder
    elif validSourceFolder(targetDir) and os.path.isfile(os.path.join(os.path.dirname(targetDir), 'Makefile')):
        buildDriver = True
        driverDir = os.path.join(targetDir, os.path.pardir, os.path.relpath(driverPaths['python']['relDriverPath'], driverPaths['python']['relSourcePath']))
        srcDir = os.path.dirname(targetDir)
    
    else:
        raise ValueError('import_python_driver was unable to determine the locations from: %s' % targetDir)
    
    # -- build if needed
    
    if buildDriver == True:
        try:
            build_in_folder(srcDir, waitNotification='Building the python drivers. This make take a few moments.')
        except test_exceptions.NotBuiltException as e:
            raise test_exceptions.NotBuiltException(detail='Failed making Python driver from: %s' % srcDir, debugInfo=e.debugInfo)
    
    # --
    
    if not os.path.isdir(driverDir) or not os.path.basename(driverDir) == 'rethinkdb' or not os.path.isfile(os.path.join(driverDir, '__init__.py')): # ToDo: handle ziped egg case
        raise ValueError('import_python_driver got an invalid driverDir: %s' % driverDir)
    
    # - return the imported module
    
    keptPaths = sys.path[:]
    try:
        moduleFile, pathname, desc = imp.find_module('rethinkdb', [os.path.dirname(driverDir)])
        driverModule = imp.load_module('rethinkdb', moduleFile, pathname, desc)
        if moduleFile is not None:
            moduleFile.close()
        loadedFrom = os.path.dirname(os.path.realpath(driverModule.__file__))
        assert loadedFrom.startswith(driverDir), "The wrong version or the rethinkdb Python driver got imported. It should have been in %s but was from %s" % (driverDir, loadedFrom)
        return driverModule
    finally:
        sys.path = keptPaths

class PerformContinuousAction(threading.Thread):
    '''Use to continuously perform an action on a table. Either provide an action (reql command without run) on instantiation, or subclass and override runAction'''
    
    action = None
    delay = None
    kwargs = None
    
    connection = None
    database = None
    
    startTime = None
    durration = 0
    sucessCount = 0
    errorCount = 0
    recordedErrors = None # error string => count
    
    daemon = True
    stopSignal = False
    
    def __init__(self, connection, database=None, action=None, autoStart=True, delay=.01, **kwargs):
        super(PerformContinuousAction, self).__init__()
        
        self.connection = connection
        self.database = database
        self.action = action
        self.delay = delay
        self.kwargs = kwargs
        
        self.recordedErrors = {}
        
        if self.database is not None:
            connection.use(database)
        
        self.startTime = time.time()
        if autoStart is True:
            self.start()
    
    def runAction(self):
        self.action.run(self.connection)
    
    def recordError(self, error):
        errorString = None
        if isinstance(error, Exception):
            errorString = error.__class__.__name__ + " " + str(error)
        else:
            errorString = str(error)
        
        if errorString not in self.recordedErrors:
            self.recordedErrors[errorString] = 1
        else:
            self.recordedErrors[errorString] += 1
        self.errorCount += 1
    
    def run(self):
        while self.stopSignal is False:
            try:
                self.runAction()
                self.sucessCount += 1
            except Exception as e:
                self.recordError(e)
                errorString = str(e)
            time.sleep(self.delay)
        self.durration = time.time() - self.startTime
    
    def stop(self):
        self.stopSignal = True
        self.join(timeout=.5)
        if self.isAlive():
          raise Warning('performContinuousAction failed to stop when asked to, results might not be trustable')
    
    def errorSummary(self):
        if self.isAlive():
            self.stop()
        
        return self.recordedErrors

def supportsTerminalColors():
    '''Return True if both stdout and stderr are tty's and seem to support color, otherwise return False'''
    import curses
    
    if not all([sys.stderr.isatty(), sys.stdout.isatty()]):
        return False
    
    try:
        curses.setupterm()
        assert curses.tigetstr('setaf') is not None
        assert curses.tparm(curses.tigetstr('setaf'), 1) is not None
    except Exception:
        return False
    return True


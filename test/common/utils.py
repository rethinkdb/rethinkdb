#!/usr/bin/env python
# Copyright 2014-2016 RethinkDB, all rights reserved.

from __future__ import print_function

import atexit, collections, fcntl, os, pprint, random, re, shutil, signal
import socket, string, subprocess, sys, tempfile, threading, time, warnings

import test_exceptions

try:
    unicode
except NameError:
    unicode = str

# -- constants

project_root_dir = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir))

driverPaths = {
    'JavaScript': {
        'envName':'JAVASCRIPT_DRIVER',
        'extension':'js',
        'driverPath':None,
        'driverRelSourcePath':os.path.join(os.pardir, os.pardir, 'build', 'packages', 'js'),
        'sourcePath':os.path.join(project_root_dir, 'drivers', 'javascript')
    },
    'Python': {
        'envName':'PYTHON_DRIVER',
        'extension':'py',
        'driverPath':None,
        'driverRelSourcePath':os.path.join(os.pardir, os.pardir, 'build', 'drivers', 'python', 'rethinkdb'),
        'sourcePath':os.path.join(project_root_dir, 'drivers', 'python')
    },
    'Ruby': {
        'envName':'RUBY_DRIVER',
        'extension':'rb',
        'driverPath':None,
        'driverRelSourcePath':os.path.join(os.pardir, os.pardir, 'build', 'drivers', 'ruby', 'lib'),
        'sourcePath':os.path.join(project_root_dir, 'drivers', 'ruby')
    },
    'JRuby': {
        'envName':'RUBY_DRIVER',
        'extension':'rb',
        'driverPath':None,
        'driverRelSourcePath':os.path.join(os.pardir, os.pardir, 'build', 'drivers', 'ruby', 'lib'),
        'sourcePath':os.path.join(project_root_dir, 'drivers', 'ruby')
    }
}

# -- reset driverPaths based on os.environ

def resetDriverPaths():
    for name, info in driverPaths.items():
        if info['envName'] in os.environ:
            if os.environ[info['envName']].lower() == '--installed--':
                info['driverPath'] = '--installed--'
                info['sourcePath'] = None
            else:
                target = os.path.realpath(os.environ[info['envName']])
                if not os.path.exists(target):
                    warnings.warn('Supplied %s driver path (%s) was not valid: %s' % (name, info['envName'], os.environ[info['envName']]))
                    continue
                
                if os.path.isfile(target) and os.path.basename(target) in ('__init__.py',):
                    target = os.path.dirname(target)
                
                if os.path.isdir(target) and 'Makefile' in os.listdir(target):
                    # source folder
                    info['driverPath'] = os.path.normpath(os.path.join(target, info['driverRelSourcePath']))
                    info['sourcePath'] = target
                else:
                    # assume it is a useable driver
                    info['driverPath'] = target
                    info['sourcePath'] = None
                os.environ[info['envName']] = info['driverPath']
        elif info['sourcePath']:
            info['driverPath'] = os.path.normpath(os.path.join(info['sourcePath'], info['driverRelSourcePath']))
            os.environ[info['envName']] = info['driverPath']

resetDriverPaths() # pickup inital values

# --

# non-printable ascii characters and invalid utf8 bytes
non_text_bytes = \
  list(range(0x00, 0x09+1)) + [0x0B, 0x0C] + list(range(0x0F, 0x1F+1)) + \
  [0xC0, 0xC1] + list(range(0xF5, 0xFF+1))

startTime = time.time()
def print_with_time(*args, **kwargs): # add timing information to print statements
    args += ('(T+ %.2fs)' % (time.time() - startTime), )
    print(*args, **kwargs)
    sys.stdout.flush()

def guess_is_text_file(name):
    with file(name, 'rb') as f:
        data = f.read(100)
    for byte in data:
        if ord(byte) in non_text_bytes:
            return False
    return True

def find_rethinkdb_executable(mode=None):
    result_path = os.environ.get('RDB_EXE_PATH') or os.path.join(latest_build_dir(check_executable=True, mode=mode), 'rethinkdb')
    
    if not os.access(result_path, os.X_OK):
        raise test_exceptions.NotBuiltException(detail='The rethinkdb server executable is not available: %s' % str(result_path))
    
    return result_path

def latest_build_dir(check_executable=True, mode=None):
    '''Look for the most recently built version of this project'''
    
    candidatePath = None
    
    if os.getenv('RETHINKDB_BUILD_DIR') is not None:
        candidatePath = os.path.realpath(os.getenv('RETHINKDB_BUILD_DIR'))
    
    else:
        masterBuildDir = os.path.join(project_root_dir, 'build')
        if not os.path.isdir(masterBuildDir):
            raise test_exceptions.NotBuiltException(detail='no version of this project has yet been built')
        
        if mode in (None, ''):
            mode = ['release', 'debug']
        elif not hasattr(mode, '__iter__'):
            mode = [str(mode)]
        
        # -- find the build directory with the most recent mtime
        
        candidateMtime = None
        for name in os.listdir(masterBuildDir):
            path = os.path.join(masterBuildDir, name)
            if os.path.isdir(path) and any(map(lambda x: name.lower().startswith(x + '_') or name.lower() == x, mode)):
                if check_executable == True:
                    if not os.path.isfile(os.path.join(path, 'rethinkdb')):
                        continue
                
                mtime = os.path.getmtime(path)
                if candidateMtime is None or mtime > candidateMtime:
                    candidateMtime = mtime
                    candidatePath = path
        
        if candidatePath is None:
            raise test_exceptions.NotBuiltException(detail='no built version of the server could be found')
    
    if candidatePath is None or (check_executable is True and not os.access(os.path.join(candidatePath, 'rethinkdb'), os.X_OK)):
        raise test_exceptions.NotBuiltException(detail='the rethinkdb server executable was not present/runnable in: %s' % candidatePath)
    
    return candidatePath

def build_in_folder(targetFolder, waitNotification=None, notificationTimeout=2, buildOptions=None):
    '''Call `make -C` on a folder to build it. If waitNotification is given wait notificationTimeout seconds and then print the notification'''
    
    outputFile = tempfile.NamedTemporaryFile('w+')
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

__loadedPythonDriver = None
def import_python_driver():
    '''return the rethinkdb Python driver, defaulting (and building) the in-source driver, but following PYTHON_DRIVER'''
    global __loadedPythonDriver
    
    # -- short-circut if we already have it loaded
    
    if __loadedPythonDriver:
        return __loadedPythonDriver
    
    # --
    
    loadedDriver = None
    driverPath =  driverPaths['Python']['driverPath']
    sourcePath =  driverPaths['Python']['sourcePath']
    
    # -- short-circut if the installed driver is called for
    
    if driverPath == '--installed--':
        # - load the driver
        try:
            loadedDriver = __import__('rethinkdb')
            driverPaths['Python']['driverPath'] = os.path.dirname(loadedDriver.__file__)
            os.environ['PYTHON_DRIVER'] = driverPaths['Python']['driverPath']
            return loadedDriver
        except ImportError as e:
            raise ImportError('Unable to load system-installed `rethinkdb` module - %s' % str(e))
    
    # -- build the driver if that is called for
    
    if sourcePath:
        try:
            build_in_folder(sourcePath, waitNotification='Building the python drivers. This make take a few moments.')
        except test_exceptions.NotBuiltException as e:
            raise test_exceptions.NotBuiltException(detail='Failed making Python driver from: %s' % sourcePath, debugInfo=e.debugInfo)
    
    # -- validate the built driver
    
    if not all(map(lambda x: os.path.isfile(os.path.join(driverPath, x)), ['__init__.py', 'ast.py', 'docs.py'])):
        raise ValueError('Invalid Python driver: %s' % driverPath)
    
    # -- load the driver
    
    keptPaths = sys.path[:]
    driverName = os.path.basename(driverPath)
    try:
        sys.path.insert(0, os.path.dirname(driverPath))
        loadedDriver = __import__(driverName)
    finally:
        sys.path = keptPaths

    
    # -- check that it is from where we assert it to be
    
    if not loadedDriver.__file__.startswith(driverPath):
        raise ImportError('Loaded Python driver was %s, rather than the expected one from %s' % (loadedDriver.__file__, driverPath))
    
    # -- return the loaded module
    
    __loadedPythonDriver = loadedDriver
    return __loadedPythonDriver

class PerformContinuousAction(threading.Thread):
    '''Use to continuously perform an action on a table. Either provide an action (reql command without run) on instantiation, or subclass and override runAction'''
    
    action = None
    delay = None
    kwargs = None
    
    connection = None
    database = None
    
    startTime = None
    duration = 0
    successCount = 0
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
                self.successCount += 1
            except Exception as e:
                self.recordError(e)
                errorString = str(e)
            time.sleep(self.delay)
        self.duration = time.time() - self.startTime
    
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

def get_test_db_table(tableName=None, dbName='test', index=0):
    '''Get the standard name for the table for this test'''
    
    # - name of __main__ module or a random name
    
    if tableName is None:
        if hasattr(sys.modules['__main__'], '__file__'):
            tableName = os.path.basename(sys.modules['__main__'].__file__).split('.')[0]
        else:
            tableName = 'random_' + ''.join(random.choice(string.ascii_uppercase for _ in range(4)))
        
        # - interpreter version
        
        tableName += '_py' + '_'.join([str(x) for x in sys.version_info[:3]])
    
    else:
        tableName = tableName.replace(".","_").replace("/","_")
    
    if index != 0:
        tableName += '_tbl%s' % str(index)
    
    # -
    
    return (dbName, tableName)
    

def get_avalible_port(interface='localhost'):
    testSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    testSocket.bind((interface, 0))
    freePort = testSocket.getsockname()[1]
    testSocket.close()
    return freePort

def is_port_open(port, host='localhost'):
    try:
        port = int(port)
        assert port > 0
    except Exception:
        raise ValueError('port must be a valid port, got: %s' % repr(port))
    testSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        testSocket.connect((str(host), port))
        testSocket.close()
        return True
    except Exception:
        return False

def wait_for_port(port, host='localhost', timeout=5):
    try:
        port = int(port)
        assert port > 0
    except Exception:
        raise ValueError('port must be a valid port, got: %s' % repr(port))
    deadline = timeout + time.time()
    while deadline > time.time():
        if is_port_open(port, host):
            return
        time.sleep(.1)
    raise Exception('Timed out after %d seconds waiting for port %d on %s to be open' % (timeout, port, host))

class RunningProcesses:
    
    class Process:
        pid = None
        ppid = None
        pgid = None
        status = None
        command = None
        
        def __init__(self, pid, ppid, pgid, status, command):
            self.pid = pid
            self.ppid = ppid
            self.pgid = pgid
            self.status = status
            self.command = command
        
        def __hash__(self):
            return hash((self.pid, self.command))
        
        def __eq__(self, other):
            if not isinstance(other, self.__class__):
                return False
            return self.__hash__() == other.__hash__()
        
        def __str__(self):
            return 'pid: %d ppid: %d pgid: %d status: %r command: %s' % (self.pid, self.ppid, self.pgid, self.status, self.command)
        
        def __repr__(self):
            return 'Process<%s>' % self.__str__()
    
    psCommand = ['ps', '-u', str(os.getuid()), '-o', 'pid=,ppid=,pgid=,state=,command=', '-www']
    
    parentPid         = None
    parentProcess     = None # process
    processes         = None # (pid, command) -> process
    processesByParent = None # pid -> set([process,...]}
    processesByGroup  = None # pgid -> set([process,...]}
    
    def __init__(self, parentPid):
        
        self.processes = {}
        self.processesByParent = {}
        self.processesByGroup = {}
        
        # -- validate input
    
        try:
            self.parentPid = int(parentPid)
            if parentPid < 0:
                raise Exception()
        except Exception:
            raise ValueError('invalid parentPid: %r' % parentPid)
        
        # -- get the inital list
        
        self.list()
    
    def list(self):
        # - reset known processes status to ''
        for process in self.processes.values():
            process.status = ''
        
        # - get ouptut from `ps`
        psProcess = subprocess.Popen(self.psCommand, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        psOutput = psProcess.communicate()[0].decode('utf-8')
        #assert psProcess.returncode == 0, 'Bad output from ps process (%d): %s\n%s' % (psProcess.returncode, ' '.join(self.psCommand), psOutput)
        
        # - parse the list
        for line in psOutput.splitlines():
            try:
                pid, ppid, pgid, status, command = line.split(None, 4)
                pid = int(pid)
                ppid = int(ppid)
                pgid = int(pgid)
            except ValueError: continue
            
            if (pid, command) in self.processes:
                thisProcess = self.processes[(pid, command)]
                thisProcess.pgid = pgid
                thisProcess.status = status
            else:
                thisProcess = self.Process(pid, ppid, pgid, status, command)
                self.processes[(pid, command)] = thisProcess
            
            # check for our parent process
            if self.parentProcess is None and thisProcess.pid == self.parentPid:
                self.parentProcess = thisProcess
            
            # catalog by parent
            if thisProcess.ppid not in self.processesByParent:
                self.processesByParent[thisProcess.ppid] = set()
            self.processesByParent[thisProcess.ppid].add(thisProcess)
            
            # catalog by pgid
            if thisProcess.pgid not in self.processesByGroup:
                self.processesByGroup[thisProcess.pgid] = set()
            self.processesByGroup[thisProcess.pgid].add(thisProcess)
        
        # - create the target list of running processes decended from our parent process
        targetProcesses = []
        candidates = set()
        candidateGroups = set()
        if self.parentProcess:
            candidates.add(self.parentProcess)
            if self.parentProcess.status and self.parentProcess.status[0] in ('D', 'I', 'R', 'S'):
                targetProcesses.append(self.parentProcess)
            if self.parentProcess.pid == self.parentProcess.pgid:
                candidateGroups.add(self.parentProcess.pgid)
        
        visited = set()
        while candidates or candidateGroups:
            for candidate in candidates.copy():
                candidates.remove(candidate)
                
                # shortcut if we have seen this node
                if candidate in visited: continue
                visited.add(candidate)
                
                # add this to the target list if it is running
                if candidate.status and candidate.status[0] in ('D', 'I', 'R', 'S') and candidate not in targetProcesses:
                    targetProcesses.append(candidate)
                
                # add any children to the list
                if candidate.pid in self.processesByParent:
                    candidates.update(self.processesByParent[candidate.pid])
                
                # add the group if it is safe
                if self.parentProcess and candidate.pgid != self.parentProcess.pgid:
                    candidateGroups.add(candidate.pgid)
            
            # add items from the groups found above
            for candidateGroupId in candidateGroups:
                candidates.update(self.processesByGroup[candidateGroupId])
            candidateGroups = set()
            
        targetProcesses.reverse() # so children go before their parents
        return targetProcesses

def kill_process_group(parent, timeout=20, sigkill_grace=2, only_warn=True):
    '''make sure that the given process group id is not running'''
    
    # -- validate input
    
    parentPopen = None
    parentPid = None
    
    if isinstance(parent, subprocess.Popen):
        parentPopen = parent
        parentPid = parent.pid
    elif hasattr(parent, 'process') and hasattr(parent.process, 'pid'): # presumably driver.Process
        parentPopen = parent.process
        parentPid = parent.process.pid
    else:
        try:
            parentPid = int(parent)
            if parentPid <= 0:
                raise Exception()
        except Exception:
            raise ValueError('invalid parent: %r' % parent)
    
    try:
        timeout = float(timeout)
        if timeout < 0:
            raise Exception()
    except Exception:
        raise ValueError('invalid timeout: %r' % timeout)
    
    try:
        sigkill_grace = float(sigkill_grace)
        if sigkill_grace < 0:
            raise Exception()
    except Exception:
        raise ValueError('invalid sigkill_grace: %r' % sigkill_grace)
    
    # -- Deadlines
    
    startTime = time.time()
    
    # deadline for a clean shutdown after SIGTERM'ing parent
    cleanDeadline = startTime + (timeout * 0.5)
    
    # deadline for all processes to respond to SIGINTs
    softDeadline = cleanDeadline + (timeout * 0.5) # deadline before resporting to SIGKILL
    
    # deadline before giving up altogether
    hardDeadline = softDeadline + sigkill_grace
    
    # - collect the running processes
    processes = RunningProcesses(parentPid)
    
    # -- Terminate the parent process
    if parentPopen:
        if parentPopen.poll() is None:
            if timeout > 0:
                parentPopen.terminate()
            else:
                parentPopen.kill()
        else:
            cleanDeadline = 0 # nothing to wait for
    else:
        try:
            if timeout > 0:
                os.kill(parentPid, signal.SIGTERM)
            else:
                os.kill(parentPid, signal.SIGKILL)
        except OSError as e:
            if e.errno == 3: # No such process
                cleanDeadline = 0 # nothing to wait for
            elif e.errno == 1: # Operation not permitted: not our process
                raise Exception('Asked to kill a process that was not ours: %d' % parentPid)
            else:
                raise

            
    
    # - wait for the processes to gracefully terminate
    while time.time() < cleanDeadline:
        if parentPopen:
            if parentPopen.poll() is not None:
                break
        else:
            try:
                result = os.waitpid(parentPid, os.WNOHANG)
                if result != (0, 0):
                    break
            except OSError as e:
                if e.errno == 10: # No such child
                    break
        time.sleep(0.1)
    
    # -- SIGINT all of the running processes
    while time.time() < softDeadline:
        time.sleep(0.1)
        runningProcesses = processes.list()
        if len(runningProcesses) == 0:
            return # everything is done
        for runner in runningProcesses:
            try:
                os.kill(runner.pid, signal.SIGINT)
            except OSError: pass # ToDo: figure out what to do here
    
    # -- SIGKILL whatever is left - multiple SIGKILLs should not make a difference, but sometimes they do
    while True:
        runningProcesses = processes.list()
        if len(runningProcesses) == 0:
            return # everything is done
        for runner in runningProcesses:
            try:
                os.kill(runner.pid, signal.SIGKILL)
            except OSError: pass # ToDo: figure out what to do here
    
        if time.time() < hardDeadline:
            time.sleep(0.2)
        else:
            break
    
    # -- try to collect the return code/process
    if parentPopen:
        parentPopen.poll()
    else:
        try:
            os.waitpid(parentPid, os.WNOHANG)
        except Exception: pass
    
    # -- return failure if anything still remains
    runningProcesses = processes.list()
    if runningProcesses:
        timeElapesed = time.time() - startTime
        message = 'Unable to kill all of the processes under pid %d after %.2f seconds:\n\t%s\n' % (parentPid, timeElapesed, '\n\t'.join([str(x) for x in runningProcesses]))
    else:
        return

def nonblocking_readline(source, seek=0):
    
    # - ensure we have a file
    
    if isinstance(source, (str, unicode)):
        if not os.path.isfile(source):
            raise ValueError('can not find the source file: %s' % str(source))
        try:
            source = open(source, 'rU')
        except Exception as e:
            raise ValueError('bad source file: %s got error: %s' % (str(source), str(e)))
    
    elif hasattr(source, 'read'):
        try:
            int(source.fileno())
        except:
            raise ValueError('bad source file, it does not have a fileno: %s' % repr(source))
    else:
        raise ValueError('bad source: %s' % repr(source))
    
    # - seek to the specified location
    source.seek(seek)
    assert source.tell() == seek
    
    # - set non-blocking IO    
    fcntl.fcntl(source, fcntl.F_SETFL, fcntl.fcntl(source, fcntl.F_GETFL) | os.O_NONBLOCK)
    
    # -
    
    waitingLines = collections.deque()
    unprocessed = ''
    lastRead = seek
    
    while True:
        
        # - return an already-processed line
        try:
            yield waitingLines.popleft()
            continue
        except IndexError: pass
        
        # - try to read in a new chunk and split it
        source.seek(lastRead)
        chunk = source.read(1024)
        
        if len(chunk) == 0:
            yield None
            continue
        lastRead = source.tell()
        
        unprocessed += chunk
        
        # - process the block into lines
        endsWithNewline = unprocessed[-1] == '\n'
        waitingLines.extend(unprocessed.splitlines())
        
        if endsWithNewline:
            unprocessed = ''
        else:
            unprocessed = waitingLines.pop()
        
        # wrap around to pass the data back

pathsToClean = []
@atexit.register # Note that this needs to be registerd with atexit before driver.endRunningServers
def atExitCleanup():
    '''Delete all of the paths that registered using cleanupPathAtExit.'''
    
    for path in pathsToClean:
        if not os.path.exists(path):
            continue
        
        elif os.path.isdir(path):
            try:
                shutil.rmtree(path)
            except Exception as e:
                sys.stdout.write('Warning: unable to cleanup folder: %s - got error: %s\n' % (str(path), str(e)))
        elif os.path.isfile(path):
            try:
                os.unlink(path)
            except Exception as e:
                sys.stdout.write('Warning: unable to cleanup file: %s - got error: %s\n' % (str(path), str(e)))

def cleanupPathAtExit(path):
    '''method to register with atExitCleanup'''
    
    path = str(path)
    assert os.path.exists(path), 'cleanupPathAtExit given a non-existent path: %s' % path
    
    global pathsToClean
    if path not in pathsToClean:
        pathsToClean.append(path)

def populateTable(conn, table, db=None, records=100, fieldName='id'):
    '''Given a table (name or object) insert a number of records into it'''
    
    # -- input validation/defaulting
    
    if hasattr(table, 'index_list'):
        pass # nothing to do here
    else:
        db = str(db) or 'test'
        table = conn._r.db(db).table(str(table))
    
    try:
        records = int(records)
        assert records >= 0
    except Exception as e:
        raise ValueError('bad value for records: %r (%s)' % (records, str(e)))
    
    fieldName = str(fieldName)
    
    # --
    
    result = table.insert(conn._r.range(1, records + 1).map({fieldName:conn._r.row})).run(conn)
    assert result['inserted'] == records, result
    assert result['errors'] == 0, result
    return result

def getShardRanges(conn, table, db='test'):
    '''Given a table and a connection return a list of tuples'''
    
    # -- input validation/defaulting
    
    # - conn
    
    if conn is None:
        raise ValueError('conn must be supplied, got None')
    elif not hasattr(conn, '_r') or not isinstance(conn, conn._r.Connection):
        raise ValueError('conn must be a RethinkDB connection object, got %s' % repr(conn))
    
    # - table/db
    
    tableName = str(table)
    dbName = str(db)
    
    # -- get split points
    
    splitPointsRaw = conn._r.db('rethinkdb').table('_debug_table_status').get(conn._r.db(dbName).table(tableName).config()['id'])['shard_scheme']['split_points'].run(conn)
    
    # -- translate split points into ranges
    
    ranges = []
    lastPoint = conn._r.minval
    for splitPoint in splitPointsRaw:
        newPoint = None
        if splitPoint.startswith('N'):
            # - numbers
            newPoint = float(splitPoint.split('#', 1)[1])
        elif splitPoint.startswith('S'):
            # - strings
            newPoint = str(splitPoint[1:])
        elif splitPoint.startswith('P'):
            raise NotImplementedError('Object split points are not currently supported')
        else:
            raise NotImplementedError('Got a type of range that is not known: %s' % repr(splitPoint))
        
        ranges.append((lastPoint, newPoint))
        lastPoint = newPoint
    
    # - default if no split points
    
    if not ranges:
        ranges.append((conn._r.minval, conn._r.maxval))
    else:
        ranges.append((lastPoint, conn._r.maxval))
    
    # -- return value
    
    return ranges

class NextWithTimeout(threading.Thread):
    '''Constantly tries to fetch the next item on an changefeed'''
    
    daemon = True
    
    feed = None
    timeout = None
    
    keepRunning = True
    latestResult = None
    stopOnEmpty = None
    
    def __enter__(self):
        return self
    
    def __exit__(self, exitType, value, traceback):
        self.keepRunning = False
    
    def __init__(self, feed, timeout=5, stopOnEmpty=True):
        self.feed = iter(feed)
        self.timeout = timeout
        self.stopOnEmpty = stopOnEmpty
        super(NextWithTimeout, self).__init__()
        self.start()
    
    def __iter__(self):
        return self
    
    def next(self):
        deadline = time.time() + self.timeout
        while time.time() < deadline:
            if self.latestResult is not None:
                if isinstance(self.latestResult, Exception):
                    raise self.latestResult
                result = self.latestResult
                self.latestResult = None
                return result
            time.sleep(.05)
        else:
            raise Exception('Timed out waiting %d seconds for next item' % self.timeout)
    
    def __next__(self):
        return self.next()
    
    def run(self):
        while self.keepRunning:
            if self.latestResult is not None:
                time.sleep(.1)
                continue
            try:
                self.latestResult = next(self.feed)
                time.sleep(.05)
            except Exception as e:
                self.latestResult = e
                if not self.stopOnEmpty:
                    self.keepRunning = False

class RePrint(pprint.PrettyPrinter, object):
    defaultPrinter = None
    
    @classmethod
    def pprint(cls, item, hangindent=0):
        print(cls.pformat(item, hangindent=hangindent))
    
    @classmethod
    def pformat(cls, item, hangindent=0):
        if cls.defaultPrinter is None:
            cls.defaultPrinter = cls(width=120)
        formated = super(RePrint, cls.defaultPrinter).pformat(item)
        if len(formated) > 70:
            padding = ' ' * hangindent
            if '\n' in formated:
                formated = ('\n' + padding).join(formated.splitlines())
        return formated
    
    def format(self, item, context, maxlevels, level):
        if str != unicode and isinstance(item, unicode):
            # remove the leading `u` from unicode objects
            return (('%r' % item)[1:], True, False) # string, readable, recursed
        else:
            return super(RePrint, self).format(item, context, maxlevels, level)
pprint  = RePrint.pprint
pformat = RePrint.pformat

import tempfile, shlex, subprocess, shutil, signal, time, os, socket, sys, traceback, threading, random

used_port = 11220
def find_unused_port():
    global used_port
    used_port += 1
    while True:
        s = socket.socket()
        try: s.bind(("", used_port))
        except socket.error: used_port += 1
        else:
            s.close()
            break
    return used_port

from retest2.utils import SmartTemporaryFile

valgrind_error_code = 100
server_quit_time = 10   # Server should shut down within 10 seconds of SIGINT

class output_file(object):
    def __init__(self):
        self.src_file = SmartTemporaryFile()
    def __del__(self):
        self.src_file.close()
    
class data_file(object):
    def __init__(self):
        tf = tempfile.NamedTemporaryFile(prefix = "rdb_")
        self.file_name = tf.name
        self.delete = True
        tf.close()

    def dont_delete_data_files(self):
        self.delete = False
            
    def __del__(self):     
        if self.delete:
            num = 0
            while os.path.exists("%s%d" % (self.file_name, num)):
                os.remove("%s%d" % (self.file_name, num))
                num += 1


class test_server(object):

    def __init__(self, rethinkdb_path, _data_file, _output_file, flags = "", valgrind = False):

        print "Finding port..."
        self.port = find_unused_port()
        print "Chose port %d." % self.port
        flags += "-p %d" % self.port
        self.output = _output_file
        self.data_file_root = _data_file
        print "temp file name: " + self.data_file_root.file_name
        print "temp output: " + self.output.src_file.name
        
        self.valgrind = valgrind
        
        self.command_line = [rethinkdb_path] + shlex.split(flags) + [self.data_file_root.file_name]
        if self.valgrind:
            self.command_line = \
                ["valgrind", "--leak-check=full", "--error-exitcode=%d" % valgrind_error_code] + \
                self.command_line

        self.server = subprocess.Popen(self.command_line,
            stdout=self.output.src_file, stderr=subprocess.STDOUT)
        
        if valgrind: time.sleep(1)   # Valgrind needs extra time to start up
        time.sleep(0.1)
    
    def report(self, what):
        raise Exception(what)
    
    def end(self):
        time.sleep(0.1)
        
        # Make sure server didn't shut down before we told it to
        if self.server.poll() != None:
            self.report("Server terminated unexpectedly with exit code %d" % self.server.poll())
        
        # Shut down the server
        self.server.send_signal(signal.SIGINT)
        
        # Wait for the server to quit
        for i in xrange(server_quit_time * 4):
            time.sleep(0.25)
            if self.server.poll() != None:
                break
        else:
            self.server.send_signal(signal.SIGKILL)
            self.report("Server did not shut down %d seconds after getting SIGINT." % \
                server_quit_time)
        
        # Make sure the server didn't encounter any problems
        if self.server.poll() != 0:
            if self.server.poll() == valgrind_error_code and self.valgrind:
                self.report("Valgrind reported errors.")
            else:
                self.report("Server shut down with exit code %d" % self.server.poll())                

    def __del__(self):
        if hasattr(self, "server") and self.server.poll() == None:
            self.server.send_signal(signal.SIGKILL)
    
import retest2

def start_server(_data_file, _output_file, _mode, _valgrind):
        
    print "Starting server..."
    server_executable = "../../build/%s%s/rethinkdb" % (
        _mode,
        "" if not _valgrind else "-valgrind")
    server = test_server(server_executable, _data_file, _output_file, valgrind = _valgrind)
    print "Started server."
    return server

def shutdown_server(server, test_failure):
    print "Shutting down server..."
    try:
        server.end()
    except Exception, f:
        server_failure = str(f) or repr(f)
    else:
        server_failure = None
    print "Shut down server."
    
    if test_failure or server_failure:
        
        msg = ""
        if test_failure is not None: msg += "Test script failed:\n    %s" % test_failure
        else: msg += "Test script succeeded."
        msg += "\n"
        if server_failure is not None: msg += "Server failed:\n    %s" % server_failure
        else: msg += "Server succeeded."
        
        temp_files = [
            retest2.ResultTempFile(
                server.output.src_file.take_file(),
                "Output from server",
                friendly_name = "server_output.txt",
                important = False)
            ]
        
        server.data_file_root.dont_delete_data_files()
        num = 0
        while os.path.exists("%s%d" % (server.data_file_root.file_name, num)):
            temp_files.append(retest2.ResultTempFile(
                "%s%d" % (server.data_file_root.file_name, num),
                "Server data file %d" % num,
                friendly_name = "data.file%d" % num,
                important = False))
            num += 1

        return retest2.Result("fail",
            description = msg,
            temp_files = temp_files)
    
    else:
        # Server's destructor will remove leftover data files            
        return retest2.Result("pass")


class RethinkDBTester(object):
        
    def __init__(self, test_function, mode, valgrind = False, timeout = 60):
    
        self.test_function = test_function
        self.mode = mode
        self.valgrind = valgrind
        
        # We will abort ourselves when 'self.own_timeout' is hit. retest2 will abort us when
        # 'self.timeout' is hit. 
        self.own_timeout = timeout
        self.timeout = timeout + 15
    
    def run_test_function(self, test_function, port, mutual_list=None):
    
        try:
            if mutual_list is not None: test_function(port, mutual_list)
            else: test_function(port)
        except Exception, f:
            self.test_failure = traceback.format_exc()
        else:
            self.test_failure = None

    def test(self):
        
        d = data_file()
        o = output_file()    
        server = start_server(d, o, self.mode, self.valgrind)
        print "Running test..."
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function, server.port,))
        test_thread.start()
        test_thread.join(self.own_timeout)
        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."

        print "Finished test."
        
        return shutdown_server(server, self.test_failure)


class RethinkDBCorruptionTester(RethinkDBTester):
        
    def __init__(self, test_function1, test_function2, mode, valgrind = False, timeout = 60):
    
        self.test_function1 = test_function1
        self.test_function2 = test_function2
        self.mode = mode
        self.valgrind = valgrind
        
        # We will abort ourselves when 'self.own_timeout' is hit. retest2 will abort us when
        # 'self.timeout' is hit. 
        self.own_timeout = timeout
        self.timeout = timeout + 15
    
    def test(self):
        
        d = data_file()
        o = output_file()
        server = start_server(d, o, self.mode, self.valgrind)
        print "Running first test..."
        mutual_list = []
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function1, server.port, mutual_list))
        test_thread.start()
        time.sleep(.001)
        
        if not test_thread.is_alive():
            raise RuntimeError, "first test finished before we could kill the server."
        else:
            server.server.send_signal(signal.SIGINT)                
            test_thread.join(self.own_timeout)
        
        last_written_key = mutual_list[-1]

        print "Server killed."
        print "Starting another server..."        
        server = start_server(d, o, self.mode, self.valgrind)
        print "Running second test..."
        mutual_list2 = []
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function2, server.port, mutual_list2))
        test_thread.start()
        test_thread.join(self.own_timeout)
        
        last_read_key = mutual_list2[-1]

        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."
        elif last_read_key < last_written_key - 1:
            self.test_failure = "The last written key was %d, but the last read key was %d" % (last_written_key, last_read_key)
        else:
            self.test_failure = None
        return shutdown_server(server, self.test_failure)
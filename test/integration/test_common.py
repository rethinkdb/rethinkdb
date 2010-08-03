import tempfile, shlex, subprocess, shutil, signal, time, os, socket, sys, traceback, threading, random

def find_unused_port():
    port = 11220
    while True:
        s = socket.socket()
        try: s.bind(("", port))
        except socket.error: port += 1
        else:
            s.close()
            break
    return port

from retest2.utils import SmartTemporaryFile

valgrind_error_code = 100
server_quit_time = 10   # Server should shut down within 10 seconds of SIGINT

class data_file(object):
    def __init__(self):
        tf = tempfile.NamedTemporaryFile(prefix = "rdb_")
        self.file_name = tf.name
        tf.close()


class test_server(object):

    def __init__(self, rethinkdb_path, data_file_path, flags = "-p 11211", valgrind = False):
    
        self.output = SmartTemporaryFile()        
        self.data_file_root = data_file_path.file_name
        
        self.valgrind = valgrind
        
        self.command_line = [rethinkdb_path] + shlex.split(flags) + [self.data_file_root]
        if self.valgrind:
            self.command_line = \
                ["valgrind", "--leak-check=full", "--error-exitcode=%d" % valgrind_error_code] + \
                self.command_line
        self.server = subprocess.Popen(self.command_line,
            stdout=self.output, stderr=subprocess.STDOUT)
        
        if valgrind: time.sleep(1)   # Valgrind needs extra time to start up
        time.sleep(0.1)
    
    def report(self, what):
        raise Exception(what)
    
    def end(self):
        try:
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
                
        finally:
            # No matter how server dies, make sure output file gets closed
            self.output.close()
    
    def dont_delete_data_files(self):
        assert self.data_file_root
        self.data_file_root = None
    
    def __del__(self):
        if hasattr(self, "server") and self.server.poll() == None:
            self.server.send_signal(signal.SIGKILL)
        
        if self.data_file_root is not None:
            num = 0
            while os.path.exists("%s%d" % (self.data_file_root, num)):
                os.remove("%s%d" % (self.data_file_root, num))
                num += 1

import retest2

class RethinkDBTester(object):
        
    def __init__(self, test_function, mode, valgrind = False, timeout = 60):
    
        self.test_function = test_function
        self.mode = mode
        self.valgrind = valgrind
        
        # We will abort ourselves when 'self.own_timeout' is hit. retest2 will abort us when
        # 'self.timeout' is hit. 
        self.own_timeout = timeout
        self.timeout = timeout + 15
    
    def run_test_function(self, test_function, port):
    
        try:
            test_function(port)
        except Exception, f:
            self.test_failure = traceback.format_exc()
        else:
            self.test_failure = None
    
    def start_server(self, data_file_path):
    
        print "Finding port..."
        port = find_unused_port()
        print "Chose port %d." % port
        
        print "Starting server..."
        server_executable = "../../build/%s%s/rethinkdb" % (
            self.mode,
            "" if not self.valgrind else "-valgrind")
        server = test_server(server_executable, data_file_path, "-p %d" % port, valgrind = self.valgrind)
        self.port = port
        self.server = server
        print "Started server."

    def shutdown_server(self):
        print "Shutting down server..."
        try:
            self.server.end()
        except Exception, f:
            self.server_failure = str(f) or repr(f)
        else:
            self.server_failure = None
        print "Shut down server."
        
        if self.test_failure or self.server_failure:
            
            msg = ""
            if self.test_failure is not None: msg += "Test script failed:\n    %s" % self.test_failure
            else: msg += "Test script succeeded."
            msg += "\n"
            if self.server_failure is not None: msg += "Server failed:\n    %s" % self.server_failure
            else: msg += "Server succeeded."
            
            temp_files = [
                retest2.ResultTempFile(
                    self.server.output.take_file(),
                    "Output from server",
                    friendly_name = "server_output.txt",
                    important = False)
                ]
            
            self.server.dont_delete_data_files()
            num = 0
            while os.path.exists("%s%d" % (self.server.data_file_root, num)):
                temp_files.append(retest2.ResultTempFile(
                    "%s%d" % (self.server.data_file_root, num),
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


    def test(self):
        
        d = data_file()
        self.start_server(d)
        port = self.port
        server = self.server
        print "Running test..."
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function, port,))
        test_thread.start()
        test_thread.join(self.own_timeout)
        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."

        print "Finished test."
        
        return self.shutdown_server()

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
        self.start_server(d)
        port = self.port
        server = self.server
        print "Running first test..."
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function1, port,))
        test_thread.start()
        test_thread.join(random.random()/100)
        if not test_thread.is_alive():
            raise RuntimeError, "first test finished before we could kill the server."
        else:
            server.server.send_signal(signal.SIGINT)

        print "Server killed."
        print "Starting another server..."        
        self.start_server(d)
        port = self.port
        server = self.server
        print "Running second test..."

        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function2, port,))
        test_thread.start()
        test_thread.join(self.own_timeout)

        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."
        
        return self.shutdown_server()
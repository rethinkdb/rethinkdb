import tempfile, subprocess, shutil, signal, time, os, sys, traceback, threading, random, retest2, socket, shlex

valgrind_error_code = 100
server_quit_time = 10   # Server should shut down within 10 seconds of SIGINT

def find_unused_port():
    used_port = 11220
    while True:
        s = socket.socket()
        try: s.bind(("", used_port))
        except socket.error: used_port += 1
        else:
            s.close()
            break
    return used_port


class test_server(object):

    def __init__(self, _data_file, _output_file, _mode, flags = "", valgrind = False):
        print "Starting server..."
        rethinkdb_path = "../../build/%s%s/rethinkdb" % (_mode, "" if not valgrind else "-valgrind")

        print "Finding port..."
        self.port = find_unused_port()
        print "Chose port %d." % self.port

        flags += "-p %d" % self.port
        self.output = _output_file
        self.data_file_root = _data_file
        print "temp file name: " + self.data_file_root.file_name
        print "temp output: " + self.output.name        

        self.valgrind = valgrind        
        self.command_line = [rethinkdb_path] + shlex.split(flags) + [self.data_file_root.file_name]
        if self.valgrind:
            self.command_line = \
                ["valgrind", "--leak-check=full", "--error-exitcode=%d" % valgrind_error_code] + \
                self.command_line

        self.server = subprocess.Popen(self.command_line,
            stdout=self.output, stderr=subprocess.STDOUT)
        
        if valgrind: time.sleep(1)   # Valgrind needs extra time to start up
        time.sleep(0.1)
        print "Started server."

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
        
    def shutdown(self, test_failure):
        print "Shutting down server..."
        try:
            self.end()
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
                    self.output.take_file(),
                    "Output from server",
                    friendly_name = "server_output.txt",
                    important = False)
                ]
            
            self.data_file_root.dont_delete_data_files()
            num = 0
            while os.path.exists("%s%d" % (self.data_file_root.file_name, num)):
                temp_files.append(retest2.ResultTempFile(
                    "%s%d" % (self.data_file_root.file_name, num),
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

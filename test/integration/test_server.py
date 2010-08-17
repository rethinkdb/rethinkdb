import tempfile, subprocess, shutil, signal, time, os, sys, traceback, threading, random, retest2, socket, shlex

valgrind_error_code = 100
server_quit_time = 3   # Server should shut down within %(server_quit_time)d seconds of SIGINT

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
        
        time.sleep(0.1)   # Give server time to start up
        if valgrind: time.sleep(1)   # Valgrind needs extra time to start up
        
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
        if self.valgrind:
            # Sometimes valgrind loses track of the first SIGINT, especially if it receives it right
            # as it is starting up. Send a second SIGINT to make sure the server gets it. This is 
            # stupid and we shouldn't have to do this.
            time.sleep(0.5)
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
            
            # Uncomment these lines if you want the server data files included in the report
            #
            # self.data_file_root.dont_delete_data_files()
            # for path in self.data_file_root.list_all_files():
            #     temp_files.append(retest2.ResultTempFile(
            #         path,
            #         "Server data file %s" % os.path.basename(path),
            #         friendly_name = os.path.basename(path),
            #         important = False))
    
            return retest2.Result("fail",
                description = msg,
                temp_files = temp_files)
                    
        else:
            # Server's destructor will remove leftover data files            
            return retest2.Result("pass")

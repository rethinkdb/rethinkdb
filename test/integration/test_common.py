import tempfile, subprocess, shutil, signal, time, os, sys, traceback, threading, random, retest2
from retest2.utils import SmartTemporaryFile
from test_server import test_server

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
                    
class GenericTester(object):

    def __init__(self):
        pass

    def run_test_function(self, test_function, port, mutual_list=None):
    
        try:
            if mutual_list is not None: test_function(port, mutual_list)
            else: test_function(port)
        except Exception, f:
            self.test_failure = traceback.format_exc()
        else:
            self.test_failure = None
   
        
class RethinkDBTester(GenericTester):
        
    def __init__(self, test_function, mode, valgrind = False, timeout = 60):
    
        self.test_function = test_function
        self.mode = mode
        self.valgrind = valgrind
        
        # We will abort ourselves when 'self.own_timeout' is hit. retest2 will abort us when
        # 'self.timeout' is hit. 
        self.own_timeout = timeout
        self.timeout = timeout + 15
    
    def test(self):
    
        d = data_file()
        o = SmartTemporaryFile()
        server = test_server(d, o, self.mode, valgrind = self.valgrind)
        print "Running test..."
        test_thread = threading.Thread(target = self.run_test_function, args = (self.test_function, server.port,))
        test_thread.start()
        test_thread.join(self.own_timeout)
        if test_thread.is_alive():
            self.test_failure = "The integration test didn't finish in time, probably because the " \
                "server wasn't responding to its queries fast enough."

        print "Finished test."        
        return server.shutdown(self.test_failure)
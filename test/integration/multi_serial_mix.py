#!/usr/bin/python
import multiprocessing, sys, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

def child(opts, log_path, port):
    # This is run in a separate process
    import sys
    sys.stdout = sys.stderr = file(log_path, "w")
    print "Starting test against server at port %d..." % port
    import serial_mix
    mc = connect_to_port(opts, port)
    serial_mix.test(opts, mc)
    mc.disconnect_all()
    print "Done with test."

shutdown_grace_period = 0.5

def test(opts, port):
    
    tester_log_dir = os.path.join(make_test_dir(), "tester_logs")
    if not os.path.isdir(tester_log_dir): os.mkdir(tester_log_dir)
    
    serial_mix_path = os.path.join(os.path.dirname(__file__), "serial_mix.py")
    
    processes = []
    try:
        
        print "Starting %d child processes..." % opts["num_testers"]
        print "Writing output from child processes to %r" % tester_log_dir
        
        for id in xrange(opts["num_testers"]):
        
            log_path = os.path.join(tester_log_dir, "%d.txt" % id)
            
            opts2 = dict(opts)
            opts2["keysuffix"] = "_%d" % id   # Prevent collisions between tests
            
            process = multiprocessing.Process(target = child, args = (opts2, log_path, port))
            process.start()
            
            processes.append((process, id))
        
        print "Waiting for child processes..."
        
        start_time = time.time()
        def time_remaining():
            time_elapsed = time.time() - start_time
            # Give subprocesses lots of extra time
            return opts["duration"] * 2 - time_elapsed + 1
        
        for (process, id) in processes:
            tr = time_remaining()
            if tr <= 0: tr = shutdown_grace_period
            process.join(tr)
        
        stuck = sorted(id for (process, id) in processes
            if process.is_alive())
        failed = sorted(id for (process, id) in processes
            if not process.is_alive() and process.exitcode != 0)
        
        if stuck or failed:
            if len(stuck) == opts["num_testers"]:
                raise ValueError("All %d processes did not finish in time." % opts["num_testers"])
            elif len(failed) == opts["num_testers"]:
                raise ValueError("All %d processes failed." % opts["num_testers"])
            else:
                raise ValueError("Of processes [1 ... %d], the following did not finish in time: " \
                    "%s and the following failed: %s" % (opts["num_testers"], stuck, failed))
    
    finally:
        for (process, id) in processes:
            if process.is_alive():
                process.terminate()
    
    print "Done."

if __name__ == "__main__":
    op = make_option_parser()
    op["num_testers"] = IntFlag("--num-testers", 16)
    op["keysize"] = IntFlag("--keysize", 250)
    op["valuesize"] = IntFlag("--valuesize", 200)
    op["thorough"] = BoolFlag("--thorough")
    opts = op.parse(sys.argv)
    auto_server_test_main(test, opts,
        timeout = opts["duration"] + opts["num_testers"] * shutdown_grace_period)

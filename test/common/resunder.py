# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/python

import sys, os, time, atexit
import subprocess
import socket
import copy
import re
from signal import SIGTERM
 
class Daemon:
    """
    A generic daemon class. Taken from http://www.jejik.com/articles/2007/02/a_simple_unix_linux_daemon_in_python/
     written by Sander Marechal

    Usage: subclass the Daemon class and override the run() method
    """
    def __init__(self, pidfile, stdin='/dev/null', stdout='/dev/null', stderr='/dev/null'):
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr
        self.pidfile = pidfile

    def daemonize(self):
        """
        do the UNIX double-fork magic, see Stevens' "Advanced
        Programming in the UNIX Environment" for details (ISBN 0201563177)
        http://www.erlenstar.demon.co.uk/unix/faq_2.html#SEC16
        """
        try:
            pid = os.fork()
            if pid > 0:
                # exit first parent
                sys.exit(0)
        except OSError, e:
            sys.stderr.write("fork #1 failed: %d (%s)\n" % (e.errno, e.strerror))
            sys.exit(1)

        # decouple from parent environment
        os.chdir("/")
        os.setsid()
        os.umask(0)

        # do second fork
        try:
            pid = os.fork()
            if pid > 0:
                # exit from second parent
                sys.exit(0)
        except OSError, e:
            sys.stderr.write("fork #2 failed: %d (%s)\n" % (e.errno, e.strerror))
            sys.exit(1)

        # redirect standard file descriptors
        sys.stdout.flush()
        sys.stderr.flush()
        si = file(self.stdin, 'r')
        so = file(self.stdout, 'a+')
        se = file(self.stderr, 'a+', 0)
        os.dup2(si.fileno(), sys.stdin.fileno())
        os.dup2(so.fileno(), sys.stdout.fileno())
        os.dup2(se.fileno(), sys.stderr.fileno())

        # write pidfile
        atexit.register(self.delpid)
        pid = str(os.getpid())
        file(self.pidfile,'w+').write("%s\n" % pid)

    def delpid(self):
        os.remove(self.pidfile)
 
    def start(self):
        """
        Start the daemon
        """
        # Check for a pidfile to see if the daemon already runs
        try:
            pf = file(self.pidfile,'r')
            pid = int(pf.read().strip())
            pf.close()
        except IOError:
            pid = None

        if pid:
            message = "pidfile %s already exist. Daemon already running?\n"
            sys.stderr.write(message % self.pidfile)
            sys.exit(1)

        # Start the daemon
        self.daemonize()
        self.run()
 
    def stop(self):
        """
        Stop the daemon
        """
        # Get the pid from the pidfile
        try:
            pf = file(self.pidfile,'r')
            pid = int(pf.read().strip())
            pf.close()
        except IOError:
            pid = None

        if not pid:
            message = "pidfile %s does not exist. Daemon not running?\n"
            sys.stderr.write(message % self.pidfile)
            return # not an error in a restart
 
        # Try killing the daemon process
        try:
            # first attempt to shutdown the daemon by issuing a shutdown command
            conn = socket.create_connection(("localhost", 46594))
            conn.sendall("shutdown\n")
            conn.close()
            time.sleep(0.2)
            while 1:
                os.kill(pid, SIGTERM)
                time.sleep(0.1)
        except OSError, err:
            err = str(err)
            if err.find("No such process") > 0:
                if os.path.exists(self.pidfile):
                    os.remove(self.pidfile)
            else:
                print str(err)
                sys.exit(1)
 
    def restart(self):
        """
        Restart the daemon
        """
        self.stop()
        self.start()

    def run(self):
        """
        You should override this method when you subclass Daemon. It will be called after the process has been
        daemonized by start() or restart().
        """
 
class ResunderDaemon(Daemon):
    def run(self):
        # TODO: keep a dict of blocked sockets and unblock them on shutdown
        listener = socket.socket()
        listener.bind(("localhost", 46594))
        listener.listen(3)
        listener.settimeout(3600) # Wake up every hour to clear out old rules
        self.blocked_ports = { }
        while True:
            try:
                client, addr = listener.accept()
                data = str(client.recv(1024))
                matches = re.match("(block|unblock) (\d+) (\d+)$", data)
                if matches is not None:
                    operation = matches.group(1)
                    source_port = matches.group(2)
                    dest_port = matches.group(3)
                    if int(source_port) < 10000 or int(source_port) > 65535 or int(dest_port) < 10000 or int(dest_port) > 65535:
                        print "invalid port specified: " + data
                    elif operation == "block":
                        self.block_port(source_port, dest_port)
                    elif operation == "unblock":
                        self.unblock_port(source_port, dest_port)
                elif data == "shutdown":
                    while len(self.blocked_ports) > 0:
                        source_port = next(self.blocked_ports.iterkeys())
                        ports_to_unblock = copy.deepcopy(self.blocked_ports[source_port])
                        for dest_port in ports_to_unblock:
                            self.unblock_port(source_port, dest_port)
                    return
            except socket.timeout:
                unblock = [ ]
                for source_port, dest_map in self.blocked_ports.iteritems():
                    for dest_port, creation_time in dest_map.iteritems():
                        if creation_time < time.time() - 864000: # find and remove rules older than 10 days
                            unblock.append((source_port, dest_port))
                for block in unblock:
                    self.unblock_port(block[0], block[1])

    def block_port(self, source_port, dest_port):
        if source_port not in self.blocked_ports:
            self.blocked_ports[source_port] = { }

        if dest_port not in self.blocked_ports[source_port]:
            self.blocked_ports[source_port][dest_port] = time.time()
            subprocess.check_output(["iptables", "-AOUTPUT", "-ptcp", "--sport", source_port, "--dport", dest_port, "-jDROP"])
            subprocess.check_output(["iptables", "-AINPUT", "-ptcp", "--sport", dest_port, "--dport", source_port, "-jDROP"])

    def unblock_port(self, source_port, dest_port):
        if source_port in self.blocked_ports and dest_port in self.blocked_ports[source_port]:
            del self.blocked_ports[source_port][dest_port]
            subprocess.check_output(["iptables", "-DOUTPUT", "-ptcp", "--sport", source_port, "--dport", dest_port, "-jDROP"])
            subprocess.check_output(["iptables", "-DINPUT", "-ptcp", "--sport", dest_port, "--dport", source_port, "-jDROP"])

            if len(self.blocked_ports[source_port]) == 0:
                del self.blocked_ports[source_port]
 
if __name__ == "__main__":
    daemon = ResunderDaemon('/tmp/resunder-daemon.pid')
    if len(sys.argv) == 2:
        if 'start' == sys.argv[1]:
            if os.geteuid() != 0:
                print "Cannot start daemon without root access"
            else:
                daemon.start()
        elif 'stop' == sys.argv[1]:
            daemon.stop()
        elif 'restart' == sys.argv[1]:
            daemon.restart()
        else:
            print "Unknown command"
            sys.exit(2)
        sys.exit(0)
    else:
        print "usage: %s start|stop|restart" % sys.argv[0]
        sys.exit(2)


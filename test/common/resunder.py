#!/usr/bin/env python
# Copyright 2010-2015 RethinkDB, all rights reserved.

import atexit, copy, logging, os, re, socket, subprocess, sys, time
from signal import SIGTERM

pidFilePath = '/tmp/resunder-daemon.pid'
resunderPort = 46594

# setup basic logging

logger = logging.getLogger('resunder')
logger.setLevel(logging.INFO)

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
        except OSError as e:
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
        except OSError as e:
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
        file(self.pidfile, 'w+').write("%s\n" % pid)

    def delpid(self):
        os.remove(self.pidfile)
    
    def start(self):
        """
        Start the daemon
        """
        # Check for a pidfile to see if the daemon already runs
        try:
            pf = file(self.pidfile, 'r')
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
            pf = file(self.pidfile, 'r')
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
            conn = socket.create_connection(("localhost", resunderPort))
            conn.sendall("shutdown\n")
            conn.close()
            time.sleep(0.2)
            while 1:
                os.kill(pid, SIGTERM)
                time.sleep(0.1)
        except OSError as err:
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
    
    blocked_ports = None
    
    def run(self):
        logger.info('Starting resunder')
        # TODO: keep a dict of blocked sockets and unblock them on shutdown
        listener = socket.socket()
        listener.bind(("localhost", 46594))
        listener.listen(3)
        listener.settimeout(3600) # Wake up every hour to clear out old rules
        self.blocked_ports = {}
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
                        logger.warning("invalid port specified: %r" % data)
                    elif operation == "block":
                        self.block_port(source_port, dest_port)
                    elif operation == "unblock":
                        self.unblock_port(source_port, dest_port)
                elif data.strip() == "shutdown":
                    logger.info('Shutting down resunder')
                    return # atexit handler will take care of cleanup
                else:
                    logger.warning('got bad input: %r' % data)
            except socket.timeout:
                unblock = []
                for ports, creation_time in self.blocked_ports:
                    if creation_time < time.time() - (60 * 60 * 24): # find and remove rules older than 1 day
                        unblock.append(ports)
                for source_port, dest_port in unblock:
                    self.unblock_port(source_port, dest_port)
                    
            except Exception as e:
                logger.error(str(e))

    def block_port(self, source_port, dest_port):
        if (source_port, dest_port) in self.blocked_ports:
            logger.debug('asked to block ports that were already on the blocked list: %s <-> %s' % (source_port, dest_port))
            return

        self.blocked_ports[tuple([source_port, dest_port])] = time.time()
        args_out = ["-AOUTPUT", "-ptcp", "--sport", source_port, "--dport", dest_port, "-jDROP", "-m", "comment", "--comment", "resunder"]
        args_in = ["-AINPUT", "-ptcp", "--sport", dest_port, "--dport", source_port, "-jDROP", "-m", "comment", "--comment", "resunder"]
        subprocess.check_output(["iptables"] + args_out)
        subprocess.check_output(["iptables"] + args_in)
        subprocess.check_output(["ip6tables"] + args_out)
        subprocess.check_output(["ip6tables"] + args_in)
        logger.info('blocked %s <-> %s' % (source_port, dest_port))

    def unblock_port(self, source_port, dest_port):
        if not (source_port, dest_port) in self.blocked_ports:
            logger.debug('asked to unblock ports that were not on the blocked list: %s <-> %s' % (source_port, dest_port))
            return
        
        args_out = ["-DOUTPUT", "-ptcp", "--sport", source_port, "--dport", dest_port, "-jDROP", "-m", "comment", "--comment", "resunder"]
        args_in = ["-DINPUT", "-ptcp", "--sport", dest_port, "--dport", source_port, "-jDROP", "-m", "comment", "--comment", "resunder"]
        subprocess.check_output(["iptables"] + args_out)
        subprocess.check_output(["iptables"] + args_in)
        subprocess.check_output(["ip6tables"] + args_out)
        subprocess.check_output(["ip6tables"] + args_in)
        
        del self.blocked_ports[tuple([source_port, dest_port])]
        logger.info('unblocked %s <-> %s' % (source_port, dest_port))
    
    def unblock_all(self):
        # ToDo: unblock all of the iptables entries with the comment "resunder"
        if not self.blocked_ports:
            return
        for source_port, dest_port in self.blocked_ports.keys():
            self.unblock_port(source_port, dest_port)

if __name__ == "__main__":
    
    # connect the logger to the log file
    fileLogger = logging.FileHandler('/var/log/resunder.log')
    fileLogger.setFormatter(logging.Formatter('%(asctime)s %(levelname)s %(message)s'))
    fileLogger.setLevel(logging.INFO)
    logger.addHandler(fileLogger) 
    
    daemon = ResunderDaemon(pidFilePath)
    atexit.register(daemon.unblock_all) # ensure cleanup
    
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
        elif 'inline' == sys.argv[1]:
            daemon.run()
        else:
            print "Unknown command"
            sys.exit(2)
        sys.exit(0)
    else:
        print "usage: %s start|stop|restart" % sys.argv[0]
        sys.exit(2)

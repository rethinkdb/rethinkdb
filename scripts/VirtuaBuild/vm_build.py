import os
import socket
import time
import socket

def remove_local(string):
    if (string[len(string) - len('.local'):] == '.local'):
        return string[:len(string) - len('.local')]
    else:
        return string

def rpm_install(path):
    return "rpm -i %s" % path

def rpm_uninstall(cmd_name): 
    return "which %s | xargs readlink -f | xargs rpm -qf | xargs rpm -e" % cmd_name

def deb_install(path):
    return "dpkg -i %s" % path

def deb_uninstall(cmd_name): 
    return "which %s | xargs readlink -f | xargs dpkg -S | sed 's/^\(.*\):.*$/\1/' | xargs dpkg -r" % cmd_name

class VM():
    def __init__(self, uuid, hostname, username = 'rethinkdb', rootname = 'root'):
        self.uuid = uuid
        self.hostname = hostname
        self.username = username
        self.rootname = rootname
        os.system("VBoxManage startvm %s --type headless" % self.uuid)
        time.sleep(100)
        while (self.command("true") != 0):
            time.sleep(3)

    def __del__(self):
        os.system("VBoxManage controlvm %s poweroff" % self.uuid)

    def command(self, cmd_str, root = False, bg = False):
        #print "Got command:\n", cmd_str
        str = "ssh -o ConnectTimeout=1000 %s@%s \"%s\"" % ((self.rootname if root else self.username), self.hostname, cmd_str) + ("&" if bg else "")
        #print str
        return os.system(str)

    def popen(self, cmd_str, mode):
        #print cmd_str
        return os.popen("ssh %s@%s \"%s\"" % (self.username, self.hostname, cmd_str), mode)

class target():
    def __init__(self, build_uuid, build_hostname, username, build_cl, res_ext, install_cl_f, uninstall_cl_f):
        self.build_uuid = build_uuid 
        self.build_hostname = build_hostname 
        self.username = username
        self.build_cl = build_cl
        self.res_ext = res_ext 
        self.install_cl_f = install_cl_f # path -> install cmd
        self.uninstall_cl_f = uninstall_cl_f

    class RunError(Exception):
        def __init__(self, str):
            self.str = str
        def __str__(self):
            return repr(self.str)

    def run(self, branch, short_name):
        if (not os.path.exists("Built_Packages")): 
            os.mkdir("Built_Packages")
        build_vm = VM(self.build_uuid, self.build_hostname, self.username)

        def run_checked(cmd, root = False, bg = False):
            res = build_vm.command(cmd, root, bg)
            if res != 0:
                raise self.RunError(cmd + " returned on %d exit." % res)

        run_checked("cd rethinkdb && git fetch -f origin {b}:refs/remotes/origin/{b} && git checkout -f origin/{b}".format(b=branch))
        run_checked("cd rethinkdb/src &&"+self.build_cl)

        dir = build_vm.popen("pwd", 'r').readline().strip('\n')
        p = build_vm.popen("find rethinkdb/build/packages -regex .*\\\\\\\\.%s" % self.res_ext, 'r')
        raw = p.readlines()
        res_paths = map((lambda x: os.path.join(dir, x.strip('\n'))), raw)
        print res_paths
        dest = os.path.abspath("Built_Packages")

        for path in res_paths:
            if (not os.path.exists(os.path.join(dest, short_name))): 
                os.mkdir(os.path.join(dest, short_name))

            build_vm.command(self.uninstall_cl_f("rethinkdb"), True)
            run_checked(self.install_cl_f(path), True)
            build_vm.command("rm test_data")
            run_checked("rethinkdb -p 11211 -f test_data &", bg = True)
            print "Starting tests..."
            while (1):
                try:
                    s = socket.create_connection((build_vm.hostname, 11211))
                    break
                except:
                    pass

            from smoke_install_test import test_against
            if (not test_against(build_vm.hostname, 11211)):
                raise self.RunError("Tests failed")
            s.send("rdb shutdown\r\n")
            run_checked(self.uninstall_cl_f("rethinkdb"), True)
            scp_string = "scp %s@%s:%s %s" % (self.username, self.build_hostname, path, os.path.join(dest, short_name))
            print scp_string
            os.system(scp_string)

def build(targets):
    os.mkdir("Built_Packages")
    map((lambda x: x.run()), targets)

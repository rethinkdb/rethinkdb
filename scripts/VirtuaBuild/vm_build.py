import os
import socket

def remove_local(string):
    if (string[len(string) - len('.local'):] == '.local'):
        return string[:len(string) - len('.local')]
    else:
        return string

def rpm_install(path):
    return "rpm -i %s" % path

def rpm_uninstall(pkg_name): 
    return "rpm -e %s" % pkg_name

def deb_install(path):
    return "dpkg -i %s" % path

def deb_uninstall(pkg_name): 
    return "dpkg -r %s" % pkg_name

class VM():
    def __init__(self, uuid, hostname, username = 'rethinkdb', rootname = 'root'):
        self.uuid = uuid
        self.hostname = hostname
        self.username = username
        self.rootname = rootname
        os.system("VBoxManage startvm %s --type headless" % self.uuid)
        import time
        time.sleep(100)
        while (self.command("true") != 0):
            time.sleep(3)

    def __del__(self):
        os.system("VBoxManage controlvm %s poweroff" % self.uuid)

    def command(self, cmd_str, root = False):
        #print "Got command:\n", cmd_str
        str = "ssh -o ConnectTimeout=1000 %s@%s \"%s\"" % ((self.rootname if root else self.username), self.hostname, cmd_str)
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
        def __init__(self, cmd, ret_val):
            self.cmd = cmd
            self.ret_val = ret_val
        def __str__(self):
            return repr(self.cmd) + " return with value: " + repr(self.ret_val)

    def run(self, branch, short_name):
        if (not os.path.exists("Built_Packages")): 
            os.mkdir("Built_Packages")
        build_vm = VM(self.build_uuid, self.build_hostname, self.username)

        def run_checked(cmd, root = False):
            res = build_vm.command(cmd, root)
            if res != 0:
                raise self.RunError(cmd, res)

        run_checked("cd rethinkdb && git checkout %s && git pull" % branch)
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
            run_checked(self.install_cl_f(path), True)
            run_checked(self.uninstall_cl_f("rethinkdb"), True)
            scp_string = "scp %s@%s:~/%s %s" % (self.username, self.build_hostname, path, os.path.join(dest, short_name))
            print scp_string
            os.system(scp_string)

def build(targets):
    os.mkdir("Built_Packages")
    map((lambda x: x.run()), targets)

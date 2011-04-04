import os
import socket

def remove_local(string):
    if (string[len(string) - len('.local'):] == '.local'):
        return string[:len(string) - len('.local')]
    else:
        return string

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
        print str
        return os.system(str)

    def popen(self, cmd_str, mode):
        print cmd_str
        return os.popen("ssh %s@%s \"%s\"" % (self.username, self.hostname, cmd_str), mode)

class target():
    def __init__(self, build_uuid, build_hostname, username, build_cl, res_ext, install_cl_f, uninstall_cl):
        self.build_uuid = build_uuid 
        self.build_hostname = build_hostname 
        self.username = username
        self.build_cl = build_cl
        self.res_ext = res_ext 
        self.install_cl_f = install_cl_f # path -> install cmd
        self.uninstall_cl = uninstall_cl

    def run(self, branch, short_name):
        if (not os.path.exists("Built_Packages")): 
            os.mkdir("Built_Packages")
        build_vm = VM(self.build_uuid, self.build_hostname, self.username)

        build_vm.command("cd rethinkdb && git pull && git checkout %s" % branch)
        build_vm.command("cd rethinkdb/src &&"+self.build_cl)

        p = build_vm.popen("find rethinkdb/build/packages -regex .*\\\\\\\\.%s" % self.res_ext, 'r')
        res_paths = map((lambda x: x.strip('\n')), p.readlines())
        print res_paths
        dest = os.path.abspath("Built_Packages")

        for path in res_paths:
            if (not os.path.exists(os.path.join(dest, short_name))): 
                os.mkdir(os.path.join(dest, short_name))
            #build_vm.command(self.install_cl_f("~rethinkdb/"+path), True)
            #build_vm.command(self.uninstall_cl, True)
            scp_string = "scp %s@%s:~/%s %s" % (self.username, self.build_hostname, path, os.path.join(dest, short_name))
            print scp_string
            os.system(scp_string)

def build(targets):
    os.mkdir("Built_Packages")
    map((lambda x: x.run()), targets)

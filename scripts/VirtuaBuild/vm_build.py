import os
import socket
import time
import socket

# pythonic discriminated union I guess, this is kind of retarded.

#actually 0 need for a base class it's really more like a comment
#that happens to be runable code
def ensure_socket(host, port):
    while (1):
        try:
            s = socket.create_connection((host, port))
            break
        except:
            time.sleep(20)
            pass
    return s

class Refspec():
    pass

class Tag(Refspec):
    def __init__(self, tagname):
        self.val = tagname

class Branch(Refspec):
    def __init__(self, branchname):
        self.val = branchname

def remove_local(string):
    if (string[len(string) - len('.local'):] == '.local'):
        return string[:len(string) - len('.local')]
    else:
        return string

def rpm_install(path):
    return "rpm -i %s" % path

def rpm_get_binary(path):
    return "rpm -qpil %s | grep /usr/bin" % path

def rpm_uninstall(cmd_name): 
    return "which %s | xargs readlink -f | xargs rpm -qf | xargs rpm -e" % cmd_name

def deb_install(path):
    return "dpkg -i %s" % path

def deb_get_binary(path):
    return "dpkg -c %s | grep /usr/bin/rethinkdb-.* | sed 's/^.*\(\/usr.*\)$/\\1/'" % path

def deb_uninstall(cmd_name): 
    return "which %s | xargs readlink -f | xargs dpkg -S | sed 's/^\(.*\):.*$/\\1/' | xargs dpkg -r" % cmd_name

class VM():
    def __init__(self, uuid, hostname, username = 'rethinkdb', rootname = 'root', vbox_username = 'rethinkdb', vbox_hostname = 'deadshot', startup = True):
        self.uuid = uuid
        self.hostname = hostname
        self.username = username
        self.rootname = rootname
        self.vbox_username = vbox_username
        self.vbox_hostname = vbox_hostname
        if (startup):
            os.system("ssh %s@%s VBoxManage startvm %s --type headless" % (self.vbox_username, self.vbox_hostname, self.uuid))
            time.sleep(80)
            while (self.command("true") != 0):
                time.sleep(3)

    def __del__(self):
        os.system("ssh %s@%s VBoxManage controlvm %s poweroff" % (self.vbox_username, self.vbox_hostname, self.uuid))

    def command(self, cmd_str, root = False, bg = False):
        str = "ssh -o ConnectTimeout=1000 %s@%s \"%s\"" % ((self.rootname if root else self.username), self.hostname, (cmd_str + ("&" if bg else ""))) + ("&" if bg else "")
        print str
        return os.system(str)

    #send a file into the tmp directory of the vm
    def copy_to_tmp(self, path):
        str = "scp %s %s@%s:/tmp/" % (path, self.username, self.hostname)
        assert(os.system(str) == 0)


    def popen(self, cmd_str, mode):
        #print cmd_str
        return os.popen("ssh %s@%s \"%s\"" % (self.username, self.hostname, cmd_str), mode)

class target():
    def __init__(self, build_uuid, build_hostname, username, build_cl, res_ext, install_cl_f, uninstall_cl_f, get_binary_f, vbox_username, vbox_hostname):
        self.build_uuid = build_uuid 
        self.build_hostname = build_hostname 
        self.username = username
        self.build_cl = build_cl
        self.res_ext = res_ext 
        self.install_cl_f = install_cl_f # path -> install cmd
        self.uninstall_cl_f = uninstall_cl_f
        self.get_binary_f = get_binary_f
	self.vbox_username = vbox_username # username and hostname for running VirtualBox through ssh
        self.vbox_hostname = vbox_hostname

    class RunError(Exception):
        def __init__(self, str):
            self.str = str
        def __str__(self):
            return repr(self.str)

    def start_vm(self):
        return VM(self.build_uuid, self.build_hostname, self.username, self.vbox_username, self.vbox_hostname) # startup = True

    def get_vm(self):
        return VM(self.build_uuid, self.build_hostname, self.username, self.vbox_username, self.vbox_hostname, startup = False)

    def interact(self, short_name):
        build_vm = self.start_vm()

        print "%s is now accessible via ssh at %s@%s" % (short_name, self.username, self.build_hostname)
        print "Leave this process running in the background and when you're done interrupt it to clean up the virtual machine."
        while True:
            time.sleep(1)

    def run(self, refspec, short_name):
        def purge_installed_packages():
            old_binaries_raw = build_vm.popen("ls /usr/bin/rethinkdb*", "r").readlines()
            old_binaries = map(lambda x: x.strip('\n'), old_binaries_raw)
            print "Binaries scheduled for removal: ", old_binaries

            for old_binary in old_binaries:
                build_vm.command(self.uninstall_cl_f(old_binary), True)

        if (not os.path.exists("Built_Packages")): 
            os.mkdir("Built_Packages")

        build_vm = self.start_vm()

        def run_checked(cmd, root = False, bg = False):
            res = build_vm.command(cmd, root, bg)
            if res != 0:
                raise self.RunError(cmd + " returned on %d exit." % res)

        def run_unchecked(cmd, root = False, bg = False):
            res = build_vm.command(cmd, root, bg)

        if isinstance(refspec, Tag):
            run_checked("cd rethinkdb && git fetch && git fetch origin tag %s && git checkout -f %s" % (refspec.val, refspec.val))
        elif isinstance(refspec, Branch):
            run_checked("cd rethinkdb && git fetch && git checkout -f %s && git pull" % refspec.val)

        else:
            raise self.RunError("Invalid refspec type, must be branch or tag.")

        run_checked("cd rethinkdb/src &&"+self.build_cl)

        dir = build_vm.popen("pwd", 'r').readline().strip('\n')
        p = build_vm.popen("find rethinkdb/build/packages -regex .*\\\\\\\\.%s" % self.res_ext, 'r')
        raw = p.readlines()
        res_paths = map((lambda x: os.path.join(dir, x.strip('\n'))), raw)
        print res_paths
        dest = os.path.abspath("Built_Packages")

        for path in res_paths:
            purge_installed_packages()

            if (not os.path.exists(os.path.join(dest, short_name))): 
                os.mkdir(os.path.join(dest, short_name))

            #install antiquated packages here
            for old_version in os.listdir('old_versions'):
                pkg = os.listdir(os.path.join('old_versions', old_version, short_name))[0]
                build_vm.copy_to_tmp(os.path.join('old_versions', old_version, short_name, pkg))
                run_checked(self.install_cl_f(os.path.join('/tmp', pkg)), True)
                print "Installed: ", old_version

            #install current versions
            target_binary_name = build_vm.popen(self.get_binary_f(path), "r").readlines()[0].strip('\n')
            print "Target binary name: ", target_binary_name
            run_checked(self.install_cl_f(path), True)

            # run smoke test
            run_unchecked("rm test_data")
            run_checked("rethinkdb -p 11211 -f test_data", bg = True)
            print "Starting tests..."
            s = ensure_socket(build_vm.hostname, 11211)
            from smoke_install_test import test_against
            if (not test_against(build_vm.hostname, 11211)):
                raise self.RunError("Tests failed")
            s.send("rethinkdb shutdown\r\n")
            scp_string = "scp %s@%s:%s %s" % (self.username, self.build_hostname, path, os.path.join(dest, short_name))
            print scp_string
            os.system(scp_string)

            # find legacy binaries
            leg_binaries_raw = build_vm.popen("ls /usr/bin/rethinkdb*", "r").readlines()
            leg_binaries = map(lambda x: x.strip('\n'), leg_binaries_raw)
            leg_binaries.remove('/usr/bin/rethinkdb') #remove the symbolic link
            leg_binaries.remove(target_binary_name)

            for leg_binary in leg_binaries:
                print "Testing migration %s --> %s..." % (leg_binary, target_binary_name)
                file_name = leg_binary.replace('/', '_').replace('-','_').replace('.', '_')

                #create the old data
                run_unchecked("rm %s_1 %s_2" % (file_name, file_name))
                run_checked("%s -p 11211 -f %s_1 -f %s_2" % (leg_binary, file_name, file_name), bg = True)
                s = ensure_socket(build_vm.hostname, 11211)
                from smoke_install_test import throw_migration_data
                throw_migration_data(build_vm.hostname, 11211)
                s.send("rethinkdb shutdown\r\n")

                #run migration
                run_unchecked("rm %s_mig_1 %s_mig_2 %s_intermediate" % ((file_name, ) * 3))
                run_checked("%s migrate --in -f %s_1 -f %s_2 --out -f %s_mig_1 -f %s_mig_2 --intermediate %s_intermediate" % ((target_binary_name,) + ((file_name,) * 5)))

                #check to see if the data is there
                run_checked("%s -p 11211 -f %s_mig_1 -f %s_mig_2" % (target_binary_name, file_name, file_name), bg = True)
                s = ensure_socket(build_vm.hostname, 11211)
                from smoke_install_test import check_migration_data
                check_migration_data(build_vm.hostname, 11211)
                s.send("rethinkdb shutdown\r\n")
                print "Done"

            purge_installed_packages()

        #clean up is used to just shutdown the machine, kind of a hack but oh well
    def clean_up(self):
        build_vm = get_vm()
        return #this calls the build_vms __del__ method which shutsdown the machine

def build(targets):
    os.mkdir("Built_Packages")
    map((lambda x: x.run()), targets)

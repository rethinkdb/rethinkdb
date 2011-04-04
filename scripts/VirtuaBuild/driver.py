import vm_build

suse = vm_build.target('12c1cf78-5dc5-4baa-8e93-ac6fdd1ebf1f', '192.168.0.173', 'rethinkdb', 'make LEGACY_LINUX=1 NO_EVENTFD=1 DEBUG=0 rpm-suse10', 'rpm', (lambda x: "rpm -i %s" % x), "rpm -e rethinkdb")
redhat5_1 = vm_build.target('5eaf8089-9ae4-4493-81fc-a885dc8e08ff', '192.168.0.159', 'rethinkdb', 'make rpm DEBUG=0 LEGACY_LINUX=1 NO_EVENTFD=1', 'rpm', (lambda x: "rpm -i %s" % x), "rpm -e rethinkdb")
ubuntu = vm_build.target('b555d9f6-441f-4b00-986f-b94286d122e9', '192.168.0.172', 'rethinkdb', 'make deb DEBUG=0', 'deb', (lambda x: "rpm -i %s" % x), "rpm -e rethinkdb")
debian = vm_build.target('3ba1350e-eda8-4166-90c1-714be0960724', '192.168.0.176', 'root', 'make deb DEBUG=0 NO_EVENTFD=1 LEGACY_LINUX=1', 'deb', None, None)
centos5_5 = vm_build.target('46c6b842-b4ac-4cd6-9eae-fe98a7246ca9', '192.168.0.177', 'root', 'make rpm DEBUG=0 LEGACY_LINUX=1', 'rpm', None, None)

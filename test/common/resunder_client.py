import sys, socket

def block_path(source_port, dest_port):
    if not ("resunder" in subprocess.check_output(["ps", "-A"])):
        print >> sys.stderr, '\nPlease start resunder process in test/common/resunder.py (as root)\n'
        assert False
    conn = socket.create_connection(("localhost", 46594))
    conn.sendall("block %s %s\n" % (str(source_port), str(dest_port)))
    # TODO: Wait for ack?
    conn.close()

def unblock_path(source_port, dest_port):
    assert "resunder" in subprocess.check_output(["ps", "-A"])
    conn = socket.create_connection(("localhost", 46594))
    conn.sendall("unblock %s %s\n" % (str(source_port), str(dest_port)))
    conn.close()

import gdb
import string
import subprocess

class CoroCommand(gdb.Command):
    "Prefix command for rethinkdb coroutines."

    def __init__(self):
        super(CoroCommand, self).__init__("coro", gdb.COMMAND_SUPPORT, gdb.COMPLETE_NONE, True)

CoroCommand()

# shamelessly copied from libstdcxx v6 python pretty-printers for gdb
class RbtreeIterator:
    def __init__(self, rbtree):
        self.size = rbtree['_M_t']['_M_impl']['_M_node_count']
        self.node = rbtree['_M_t']['_M_impl']['_M_header']['_M_left']
        self.count = 0

    def __iter__(self):
        return self

    def __len__(self):
        return int (self.size)

    def next(self):
        if self.count == self.size:
            raise StopIteration
        result = self.node
        self.count = self.count + 1
        if self.count < self.size:
            # Compute the next node.
            node = self.node
            if node.dereference()['_M_right']:
                node = node.dereference()['_M_right']
                while node.dereference()['_M_left']:
                    node = node.dereference()['_M_left']
            else:
                parent = node.dereference()['_M_parent']
                while node == parent.dereference()['_M_right']:
                    node = parent
                    parent = parent.dereference()['_M_parent']
                if node.dereference()['_M_right'] != parent:
                    node = parent
            self.node = node
        return result

class StdSetPrinter:
    "Print a std::set or std::multiset"

    # Turn an RbtreeIterator into a pretty-print iterator.
    class _iter:
        def __init__(self, rbiter, type):
            self.rbiter = rbiter
            self.count = 0
            self.type = type

        def __iter__(self):
            return self

        def next(self):
            item = self.rbiter.next()
            item = item.cast(self.type).dereference()['_M_value_field']
            # FIXME: this is weird ... what to do?
            # Maybe a 'set' display hint?
            result = (self.count, item)
            self.count = self.count + 1
            return result

    def __init__ (self, val):
        self.val = val

    def children (self):
        keytype = self.val.type.template_argument(0)
        nodetype = gdb.lookup_type('std::_Rb_tree_node< %s >' % keytype).pointer()
        return self._iter (RbtreeIterator (self.val), nodetype)

def get_coro_backtrace(gdb, coro_ptr, exe_file):
    max_frames = 50
    result = []
    coro_context = gdb.parse_and_eval("((coro_t*)%s)->stack->context" % coro_ptr)
    current_frame_ptr = gdb.execute("x /xg %s" % coro_context["pointer"], True, True).split()[-1]
    next_frame_ptr = gdb.execute("x /xg %s" % current_frame_ptr, True, True).split()[-1]

    while (next_frame_ptr != "0x0000000000000000" and max_frames > 0):
        program_counter_ptr = gdb.execute("p /x %s + 8" % current_frame_ptr, True, True).split()[-1]
        program_counter = gdb.execute("x /xg %s" % program_counter_ptr, True, True).split()[-1]

        if (program_counter == "0x0000000000000000"):
            break

        function_name = gdb.execute("info symbol %s" % program_counter, True, True)

        if (exe_file is not None):
            addr2line = subprocess.Popen(["addr2line", "-e", exe_file, program_counter], stdout=subprocess.PIPE)
            file_line = addr2line.communicate()[0].rstrip()
        else:
            file_line = "<ERROR>"
        
        result.append((file_line, function_name))

        # TODO: use a version of gdb that supports this
        # line_object = gdb.find_pc_line(program_counter)
        # if not line_object.valid:
        #     gdb.write("line info not valid\n")
        # else:
        #     gdb.write("line info: %s:%d\n" % (line_object.symtab.filename, line_object.line))
        current_frame_ptr = next_frame_ptr
        next_frame_ptr = gdb.execute("x /xg %s" % current_frame_ptr, True, True).split()[-1]
        max_frames = max_frames - 1
    return result

class coro_tree_node(object):
    def __init__(self, coro, index):
        self.coro = coro
        self.purpose = coro.dereference()["purpose_"]
        self.index = index
        self.children = []

def insert_into_tree(list_of_coro_nodes, new_node):
    for node in list_of_coro_nodes:
        if new_node.coro.dereference()["parent_"] == node.coro:
            node.children += [new_node]
            return True
        elif insert_into_tree(node.children, new_node):
            return True
    return False

def make_coro_tree(list_of_coros):
    coro_nodes = []
    for index, coro in list_of_coros:
        node = coro_tree_node(coro, index)
        if not insert_into_tree(coro_nodes, node):
            coro_nodes += [node]
    return coro_nodes

def print_coro_nodes(list_of_coro_nodes, tabs):
    for node in list_of_coro_nodes:
        gdb.write("\t" * tabs + " " + "[%d] %s - %s" % (node.index, node.coro, node.purpose) + "\n")
        print_coro_nodes(node.children, tabs + 1)


class CoroListCommand(gdb.Command):
    "List of all coroutines running in the current thread."

    def __init__(self):
        super(CoroListCommand, self).__init__("coro list", gdb.COMMAND_SUPPORT, gdb.COMPLETE_NONE, True)
        # TODO: get this from gdb, maybe using info inferiors
        self.exe_file = "/home/jdoliner/rethinkdb/build/debug/rethinkdb"

    def invoke(self, arg, from_tty):
        coro_globals_ptr = gdb.parse_and_eval("cglobals")
        if (coro_globals_ptr):
            active_coros = StdSetPrinter(coro_globals_ptr.dereference()["active_coroutines"])
            current_coro = coro_globals_ptr.dereference()["current_coro"]

            gdb.write("active coroutine count: %d\n" % len (active_coros.children().rbiter))
            print_coro_nodes(make_coro_tree(list(active_coros.children())), 0)
        else:
            gdb.write("no coroutine structure found, switch to a thread with coroutines\n")

CoroListCommand()

class CoroShowCommand(gdb.Command):
    "Show the backtrace of a coroutine in the current thread."

    def __init__(self):
        super(CoroShowCommand, self).__init__("coro show", gdb.COMMAND_SUPPORT, gdb.COMPLETE_NONE, True)
        # TODO: get this from gdb, maybe using info inferiors
        self.exe_file = "/home/jdoliner/rethinkdb/build/debug/rethinkdb"

    def invoke(self, arg, from_tty):
        coro_globals_ptr = gdb.parse_and_eval("cglobals")
        try:
            i = int(arg)
        except ValueError:
            gdb.write("invalid index, usage: coro show [index]\n")
            return
        if (coro_globals_ptr):
            current_coro = coro_globals_ptr.dereference()["current_coro"]
            active_coros = StdSetPrinter(coro_globals_ptr.dereference()["active_coroutines"])
            if (i > len(active_coros.children().rbiter)):
                gdb.write("index is out of bounds\n")
                return

            for coro in active_coros.children():
                if (i != 0):
                    i = i - 1
                    continue

                if (current_coro == coro[1]):
                    continue

                # coro[0] contains the index we're in
                # coro[1] contains a string of the pointer to the coro_t
                # TODO: do this in a more dependable fashion

                backtrace = get_coro_backtrace(gdb, coro[1], self.exe_file)
                for line in backtrace:
                    gdb.write("%s - %s\n" % line)

                break

CoroShowCommand()

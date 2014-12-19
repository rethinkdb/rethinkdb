# Copyright 2010-2012 RethinkDB, all rights reserved.
import gdb
import itertools, re

class StdPointerPrinter:
    "Print a smart pointer of some kind"

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def to_string(self):
        if self.val['_M_refcount']['_M_pi'] == 0:
            return '%s (empty) %s' % (self.typename, self.val['_M_ptr'])
        return '%s (count %d) %s' % (self.typename,
                                     self.val['_M_refcount']['_M_pi']['_M_use_count'],
                                     self.val['_M_ptr'])

class UniquePointerPrinter:
    "Print a unique_ptr"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['_M_t']

class StdListPrinter:
    "Print a std::list"

    class _iterator:
        def __init__(self, nodetype, head):
            self.nodetype = nodetype
            self.base = head['_M_next']
            self.head = head.address
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            if self.base == self.head:
                raise StopIteration
            elt = self.base.cast(self.nodetype).dereference()
            self.base = elt['_M_next']
            count = self.count
            self.count = self.count + 1
            return ('[%d]' % count, elt['_M_data'])

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def children(self):
        itype = self.val.type.template_argument(0)
        # If the inferior program is compiled with -D_GLIBCXX_DEBUG
        # some of the internal implementation details change.
        if self.typename == "std::list":
            nodetype = gdb.lookup_type('std::_List_node<%s>' % itype).pointer()
        elif self.typename == "std::__debug::list":
            nodetype = gdb.lookup_type('std::__norm::_List_node<%s>' % itype).pointer()
        else:
            raise ValueError("Cannot cast list node for list printer.")
        return self._iterator(nodetype, self.val['_M_impl']['_M_node'])

    def to_string(self):
        if self.val['_M_impl']['_M_node'].address == self.val['_M_impl']['_M_node']['_M_next']:
            return 'empty %s' % (self.typename)
        return '%s' % (self.typename)

class StdListIteratorPrinter:
    "Print std::list::iterator"

    def __init__(self, typename, val):
        self.val = val
        self.typename = typename

    def to_string(self):
        itype = self.val.type.template_argument(0)
        # If the inferior program is compiled with -D_GLIBCXX_DEBUG
        # some of the internal implementation details change.
        if self.typename == "std::_List_iterator" or self.typename == "std::_List_const_iterator":
            nodetype = gdb.lookup_type('std::_List_node<%s>' % itype).pointer()
        elif self.typename == "std::__norm::_List_iterator" or self.typename == "std::__norm::_List_const_iterator":
            nodetype = gdb.lookup_type('std::__norm::_List_node<%s>' % itype).pointer()
        else:
            raise ValueError("Cannot cast list node for list iterator printer.")
        return self.val['_M_node'].cast(nodetype).dereference()['_M_data']

class StdSlistPrinter:
    "Print a __gnu_cxx::slist"

    class _iterator:
        def __init__(self, nodetype, head):
            self.nodetype = nodetype
            self.base = head['_M_head']['_M_next']
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            if self.base == 0:
                raise StopIteration
            elt = self.base.cast(self.nodetype).dereference()
            self.base = elt['_M_next']
            count = self.count
            self.count = self.count + 1
            return ('[%d]' % count, elt['_M_data'])

    def __init__(self, val):
        self.val = val

    def children(self):
        itype = self.val.type.template_argument(0)
        nodetype = gdb.lookup_type('__gnu_cxx::_Slist_node<%s>' % itype).pointer()
        return self._iterator(nodetype, self.val)

    def to_string(self):
        if self.val['_M_head']['_M_next'] == 0:
            return 'empty __gnu_cxx::slist'
        return '__gnu_cxx::slist'

class StdSlistIteratorPrinter:
    "Print __gnu_cxx::slist::iterator"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        itype = self.val.type.template_argument(0)
        nodetype = gdb.lookup_type('__gnu_cxx::_Slist_node<%s>' % itype).pointer()
        return self.val['_M_node'].cast(nodetype).dereference()['_M_data']

class StdVectorPrinter:
    "Print a std::vector"

    class _iterator:
        def __init__(self, start, finish):
            self.item = start
            self.finish = finish
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            if self.item == self.finish:
                raise StopIteration
            count = self.count
            self.count = self.count + 1
            elt = self.item.dereference()
            self.item = self.item + 1
            return ('[%d]' % count, elt)

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def children(self):
        return self._iterator(self.val['_M_impl']['_M_start'],
                              self.val['_M_impl']['_M_finish'])

    def to_string(self):
        start = self.val['_M_impl']['_M_start']
        finish = self.val['_M_impl']['_M_finish']
        end = self.val['_M_impl']['_M_end_of_storage']
        return ('%s of length %d, capacity %d' % (self.typename, int(finish - start), int(end - start)))

    def display_hint(self):
        return 'array'

class StdVectorIteratorPrinter:
    "Print std::vector::iterator"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['_M_current'].dereference()

class StdTuplePrinter:
    "Print a std::tuple"

    class _iterator:
        def __init__(self, head):
            self.head = head

            # Set the base class as the initial head of the
            # tuple.
            nodes = self.head.type.fields()
            if len(nodes) != 1:
                raise ValueError("Top of tuple tree does not consist of a single node.")

            # Set the actual head to the first pair.
            self.head = self.head.cast(nodes[0].type)
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            nodes = self.head.type.fields()
            # Check for further recursions in the inheritance tree.
            if len(nodes) == 0:
                raise StopIteration
            # Check that this iteration has an expected structure.
            if len(nodes) != 2:
                raise ValueError("Cannot parse more than 2 nodes in a tuple tree.")

            # - Left node is the next recursion parent.
            # - Right node is the actual class contained in the tuple.

            # Process right node.
            impl = self.head.cast(nodes[1].type)

            # Process left node and set it as head.
            self.head = self.head.cast(nodes[0].type)
            self.count = self.count + 1

            # Finally, check the implementation.  If it is
            # wrapped in _M_head_impl return that, otherwise return
            # the value "as is".
            fields = impl.type.fields()
            if len(fields) < 1 or fields[0].name != "_M_head_impl":
                return ('[%d]' % self.count, impl)
            else:
                return ('[%d]' % self.count, impl['_M_head_impl'])

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val;

    def children(self):
        return self._iterator(self.val)

    def to_string(self):
        return '%s containing' % (self.typename)

class StdStackOrQueuePrinter:
    "Print a std::stack or std::queue"

    def __init__(self, typename, val):
        self.typename = typename
        self.visualizer = gdb.default_visualizer(val['c'])

    def children(self):
        return self.visualizer.children()

    def to_string(self):
        return '%s wrapping: %s' % (self.typename,
                                    self.visualizer.to_string())

    def display_hint(self):
        if hasattr(self.visualizer, 'display_hint'):
            return self.visualizer.display_hint()
        return None

class RbtreeIterator:
    def __init__(self, rbtree):
        self.size = rbtree['_M_t']['_M_impl']['_M_node_count']
        self.node = rbtree['_M_t']['_M_impl']['_M_header']['_M_left']
        self.count = 0

    def __iter__(self):
        return self

    def __len__(self):
        return int(self.size)

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

# This is a pretty printer for std::_Rb_tree_iterator (which is
# std::map::iterator), and has nothing to do with the RbtreeIterator
# class above.
class StdRbtreeIteratorPrinter:
    "Print std::map::iterator"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        valuetype = self.val.type.template_argument(0)
        nodetype = gdb.lookup_type('std::_Rb_tree_node < %s >' % valuetype)
        nodetype = nodetype.pointer()
        return self.val.cast(nodetype).dereference()['_M_value_field']

class StdDebugIteratorPrinter:
    "Print a debug enabled version of an iterator"

    def __init__(self, val):
        self.val = val

    # Just strip away the encapsulating __gnu_debug::_Safe_iterator
    # and return the wrapped iterator value.
    def to_string(self):
        itype = self.val.type.template_argument(0)
        return self.val['_M_current'].cast(itype)

class StdMapPrinter:
    "Print a std::map or std::multimap"

    # Turn an RbtreeIterator into a pretty-print iterator.
    class _iter:
        def __init__(self, rbiter, type):
            self.rbiter = rbiter
            self.count = 0
            self.type = type

        def __iter__(self):
            return self

        def next(self):
            if self.count % 2 == 0:
                n = self.rbiter.next()
                n = n.cast(self.type).dereference()['_M_value_field']
                self.pair = n
                item = n['first']
            else:
                item = self.pair['second']
            result = ('[%d]' % self.count, item)
            self.count = self.count + 1
            return result

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def to_string(self):
        return '%s with %d elements' % (self.typename, len(RbtreeIterator(self.val)))

    def children(self):
        keytype = self.val.type.template_argument(0).const()
        valuetype = self.val.type.template_argument(1)
        nodetype = gdb.lookup_type('std::_Rb_tree_node< std::pair< %s, %s > >' % (keytype, valuetype))
        nodetype = nodetype.pointer()
        return self._iter(RbtreeIterator(self.val), nodetype)

    def display_hint(self):
        return 'map'

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
            result = ('[%d]' % self.count, item)
            self.count = self.count + 1
            return result

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def to_string(self):
        return '%s with %d elements' % (self.typename, len(RbtreeIterator(self.val)))

    def children(self):
        keytype = self.val.type.template_argument(0)
        nodetype = gdb.lookup_type('std::_Rb_tree_node< %s >' % keytype).pointer()
        return self._iter(RbtreeIterator(self.val), nodetype)

class StdBitsetPrinter:
    "Print a std::bitset"

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def to_string(self):
        # If template_argument handled values, we could print the
        # size.  Or we could use a regexp on the type.
        return '%s' % (self.typename)

    def children(self):
        words = self.val['_M_w']
        wtype = words.type

        # The _M_w member can be either an unsigned long, or an
        # array.  This depends on the template specialization used.
        # If it is a single long, convert to a single element list.
        if wtype.code == gdb.TYPE_CODE_ARRAY:
            tsize = wtype.target().sizeof
        else:
            words = [words]
            tsize = wtype.sizeof

        nwords = wtype.sizeof / tsize
        result = []
        byte = 0
        while byte < nwords:
            w = words[byte]
            bit = 0
            while w != 0:
                if (w & 1) != 0:
                    # Another spot where we could use 'set'?
                    result.append(('[%d]' % (byte * tsize * 8 + bit), 1))
                bit = bit + 1
                w = w >> 1
            byte = byte + 1
        return result

class StdDequePrinter:
    "Print a std::deque"

    class _iter:
        def __init__(self, node, start, end, last, buffer_size):
            self.node = node
            self.p = start
            self.end = end
            self.last = last
            self.buffer_size = buffer_size
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            if self.p == self.last:
                raise StopIteration

            result = ('[%d]' % self.count, self.p.dereference())
            self.count = self.count + 1

            # Advance the 'cur' pointer.
            self.p = self.p + 1
            if self.p == self.end:
                # If we got to the end of this bucket, move to the
                # next bucket.
                self.node = self.node + 1
                self.p = self.node[0]
                self.end = self.p + self.buffer_size

            return result

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val
        self.elttype = val.type.template_argument(0)
        size = self.elttype.sizeof
        if size < 512:
            self.buffer_size = int(512 / size)
        else:
            self.buffer_size = 1

    def to_string(self):
        start = self.val['_M_impl']['_M_start']
        end = self.val['_M_impl']['_M_finish']

        delta_n = end['_M_node'] - start['_M_node'] - 1
        delta_s = start['_M_last'] - start['_M_cur']
        delta_e = end['_M_cur'] - end['_M_first']

        size = self.buffer_size * delta_n + delta_s + delta_e

        return '%s with %d elements' % (self.typename, long(size))

    def children(self):
        start = self.val['_M_impl']['_M_start']
        end = self.val['_M_impl']['_M_finish']
        return self._iter(start['_M_node'], start['_M_cur'], start['_M_last'],
                          end['_M_cur'], self.buffer_size)

    def display_hint(self):
        return 'array'

class StdDequeIteratorPrinter:
    "Print std::deque::iterator"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['_M_cur'].dereference()

class StdStringPrinter:
    "Print a std::basic_string of some kind"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        # Make sure &string works, too.
        type = self.val.type
        if type.code == gdb.TYPE_CODE_REF:
            type = type.target()

        # Calculate the length of the string so that to_string returns
        # the string according to length, not according to first null
        # encountered.
        ptr = self.val['_M_dataplus']['_M_p']
        realtype = type.unqualified().strip_typedefs()
        try:                    # clang thinks std::string is std::basic_string<char>
            realtype.template_argument(1)
        except RuntimeError:
            basetype = str(realtype.template_argument(0))
            realtype = gdb.lookup_type(("std::basic_string<%s, std::char_traits" +
                                        "<%s>, std::allocator<%s> >")
                                       % (basetype, basetype, basetype))
        reptype = gdb.lookup_type(str(realtype) + '::_Rep').pointer()
        header = ptr.cast(reptype) - 1
        len = header.dereference()['_M_length']
        return self.val['_M_dataplus']['_M_p'].lazy_string(length=len)

    def display_hint(self):
        return 'string'

class Tr1HashtableIterator:
    def __init__(self, hash):
        self.count = 0
        self.n_buckets = hash['_M_element_count']
        if self.n_buckets == 0:
            self.node = False
        else:
            self.bucket = hash['_M_buckets']
            self.node = self.bucket[0]
            self.update()

    def __iter__(self):
        return self

    def update(self):
        # If we advanced off the end of the chain, move to the next
        # bucket.
        while self.node == 0:
            self.bucket = self.bucket + 1
            self.node = self.bucket[0]

        # If we advanced off the end of the bucket array, then
        # we're done.
        if self.count == self.n_buckets:
            self.node = False
        else:
            self.count = self.count + 1

    def next(self):
        if not self.node:
            raise StopIteration
        result = self.node.dereference()['_M_v']
        self.node = self.node.dereference()['_M_next']
        self.update()
        return result

class Tr1UnorderedSetPrinter:
    "Print a tr1::unordered_set"

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def to_string(self):
        return '%s with %d elements' % (self.typename, self.val['_M_element_count'])

    @staticmethod
    def format_count(i):
        return '[%d]' % i

    def children(self):
        counter = itertools.imap(self.format_count, itertools.count())
        return itertools.izip(counter, Tr1HashtableIterator(self.val))

class Tr1UnorderedMapPrinter:
    "Print a tr1::unordered_map"

    def __init__(self, typename, val):
        self.typename = typename
        self.val = val

    def to_string(self):
        return '%s with %d elements' % (self.typename, self.val['_M_element_count'])

    @staticmethod
    def flatten(list):
        for elt in list:
            for i in elt:
                yield i

    @staticmethod
    def format_one(elt):
        return (elt['first'], elt['second'])

    @staticmethod
    def format_count(i):
        return '[%d]' % i

    def children(self):
        counter = itertools.imap(self.format_count, itertools.count())
        # Map over the hash table and flatten the result.
        data = self.flatten(itertools.imap(self.format_one, Tr1HashtableIterator(self.val)))
        # Zip the two iterators together.
        return itertools.izip(counter, data)

    def display_hint(self):
        return 'map'

def lookup_function (val):
    "Look-up and return a pretty-printer that can print val."
    global pretty_printers_dict

    # Get the type.
    type = val.type

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified ().strip_typedefs ()

    # Get the type name.
    typename = type.tag

    if typename == None:
        return None

    # Iterate over local dictionary of types to determine
    # if a printer is registered for that type.  Return an
    # instantiation of the printer if found.
    for function in pretty_printers_dict:
        if function.match (typename):
            return pretty_printers_dict[function] (val)

    # Cannot find a pretty printer.  Return None.

    return None

def disable_lookup_function ():
    lookup_function.enabled = False

def enable_lookup_function ():
    lookup_function.enabled = True

pretty_printers_dict = {}

def register_libstdcxx_printers(obj):
    "Register libstdc++ pretty-printers with objfile Obj."

    if obj == None:
        obj = gdb

    obj.pretty_printers.append(lookup_function)

# libstdc++ objects requiring pretty-printing.
# In order from:
# http://gcc.gnu.org/onlinedocs/libstdc++/latest-doxygen/a01847.html
pretty_printers_dict[re.compile('^std::basic_string<.*>$')] = lambda val: StdStringPrinter(val)
pretty_printers_dict[re.compile('^std::bitset<.*>$')] = lambda val: StdBitsetPrinter("std::bitset", val)
pretty_printers_dict[re.compile('^std::deque<.*>$')] = lambda val: StdDequePrinter("std::deque", val)
pretty_printers_dict[re.compile('^std::list<.*>$')] = lambda val: StdListPrinter("std::list", val)
pretty_printers_dict[re.compile('^std::map<.*>$')] = lambda val: StdMapPrinter("std::map", val)
pretty_printers_dict[re.compile('^std::multimap<.*>$')] = lambda val: StdMapPrinter("std::multimap", val)
pretty_printers_dict[re.compile('^std::multiset<.*>$')] = lambda val: StdSetPrinter("std::multiset", val)
pretty_printers_dict[re.compile('^std::priority_queue<.*>$')] = lambda val: StdStackOrQueuePrinter("std::priority_queue", val)
pretty_printers_dict[re.compile('^std::queue<.*>$')] = lambda val: StdStackOrQueuePrinter("std::queue", val)
pretty_printers_dict[re.compile('^std::tuple<.*>$')] = lambda val: StdTuplePrinter("std::tuple", val)
pretty_printers_dict[re.compile('^std::set<.*>$')] = lambda val: StdSetPrinter("std::set", val)
pretty_printers_dict[re.compile('^std::stack<.*>$')] = lambda val: StdStackOrQueuePrinter("std::stack", val)
pretty_printers_dict[re.compile('^std::unique_ptr<.*>$')] = UniquePointerPrinter
pretty_printers_dict[re.compile('^std::vector<.*>$')] = lambda val: StdVectorPrinter("std::vector", val)
# vector<bool>

# Printer registrations for classes compiled with -D_GLIBCXX_DEBUG.
pretty_printers_dict[re.compile('^std::__debug::bitset<.*>$')] = lambda val: StdBitsetPrinter("std::__debug::bitset", val)
pretty_printers_dict[re.compile('^std::__debug::deque<.*>$')] = lambda val: StdDequePrinter("std::__debug::deque", val)
pretty_printers_dict[re.compile('^std::__debug::list<.*>$')] = lambda val: StdListPrinter("std::__debug::list", val)
pretty_printers_dict[re.compile('^std::__debug::map<.*>$')] = lambda val: StdMapPrinter("std::__debug::map", val)
pretty_printers_dict[re.compile('^std::__debug::multimap<.*>$')] = lambda val: StdMapPrinter("std::__debug::multimap", val)
pretty_printers_dict[re.compile('^std::__debug::multiset<.*>$')] = lambda val: StdSetPrinter("std::__debug::multiset", val)
pretty_printers_dict[re.compile('^std::__debug::priority_queue<.*>$')] = lambda val: StdStackOrQueuePrinter("std::__debug::priority_queue", val)
pretty_printers_dict[re.compile('^std::__debug::queue<.*>$')] = lambda val: StdStackOrQueuePrinter("std::__debug::queue", val)
pretty_printers_dict[re.compile('^std::__debug::set<.*>$')] = lambda val: StdSetPrinter("std::__debug::set", val)
pretty_printers_dict[re.compile('^std::__debug::stack<.*>$')] = lambda val: StdStackOrQueuePrinter("std::__debug::stack", val)
pretty_printers_dict[re.compile('^std::__debug::unique_ptr<.*>$')] = UniquePointerPrinter
pretty_printers_dict[re.compile('^std::__debug::vector<.*>$')] = lambda val: StdVectorPrinter("std::__debug::vector", val)

# These are the TR1 and C++0x printers.
# For array - the default GDB pretty-printer seems reasonable.
pretty_printers_dict[re.compile('^std::shared_ptr<.*>$')] = lambda val: StdPointerPrinter('std::shared_ptr', val)
pretty_printers_dict[re.compile('^std::weak_ptr<.*>$')] = lambda val: StdPointerPrinter('std::weak_ptr', val)
pretty_printers_dict[re.compile('^std::unordered_map<.*>$')] = lambda val: Tr1UnorderedMapPrinter('std::unordered_map', val)
pretty_printers_dict[re.compile('^std::unordered_set<.*>$')] = lambda val: Tr1UnorderedSetPrinter('std::unordered_set', val)
pretty_printers_dict[re.compile('^std::unordered_multimap<.*>$')] = lambda val: Tr1UnorderedMapPrinter('std::unordered_multimap', val)
pretty_printers_dict[re.compile('^std::unordered_multiset<.*>$')] = lambda val: Tr1UnorderedSetPrinter('std::unordered_multiset', val)

pretty_printers_dict[re.compile('^std::tr1::shared_ptr<.*>$')] = lambda val: StdPointerPrinter('std::tr1::shared_ptr', val)
pretty_printers_dict[re.compile('^std::tr1::weak_ptr<.*>$')] = lambda val: StdPointerPrinter('std::tr1::weak_ptr', val)
pretty_printers_dict[re.compile('^std::tr1::unordered_map<.*>$')] = lambda val: Tr1UnorderedMapPrinter('std::tr1::unordered_map', val)
pretty_printers_dict[re.compile('^std::tr1::unordered_set<.*>$')] = lambda val: Tr1UnorderedSetPrinter('std::tr1::unordered_set', val)
pretty_printers_dict[re.compile('^std::tr1::unordered_multimap<.*>$')] = lambda val: Tr1UnorderedMapPrinter('std::tr1::unordered_multimap', val)
pretty_printers_dict[re.compile('^std::tr1::unordered_multiset<.*>$')] = lambda val: Tr1UnorderedSetPrinter('std::tr1::unordered_multiset', val)

# These are the C++0x printer registrations for -D_GLIBCXX_DEBUG cases.
# The tr1 namespace printers do not seem to have any debug
# equivalents, so do no register them.
pretty_printers_dict[re.compile('^std::__debug::unordered_map<.*>$')] = lambda val: Tr1UnorderedMapPrinter('std::__debug::unordered_map', val)
pretty_printers_dict[re.compile('^std::__debug::unordered_set<.*>$')] = lambda val: Tr1UnorderedSetPrinter('std::__debug::unordered_set', val)
pretty_printers_dict[re.compile('^std::__debug::unordered_multimap<.*>$')] = lambda val: Tr1UnorderedMapPrinter('std::__debug::unordered_multimap', val)
pretty_printers_dict[re.compile('^std::__debug::unordered_multiset<.*>$')] = lambda val: Tr1UnorderedSetPrinter('std::__debug:unordered_multiset', val)


# Extensions.
pretty_printers_dict[re.compile('^__gnu_cxx::slist<.*>$')] = StdSlistPrinter

if True:
    # These shouldn't be necessary, if GDB "print *i" worked.
    # But it often doesn't, so here they are.
    pretty_printers_dict[re.compile('^std::_List_iterator<.*>$')] = lambda val: StdListIteratorPrinter("std::_List_iterator", val)
    pretty_printers_dict[re.compile('^std::_List_const_iterator<.*>$')] = lambda val: StdListIteratorPrinter("std::_List_const_iterator", val)
    pretty_printers_dict[re.compile('^std::_Rb_tree_iterator<.*>$')] = lambda val: StdRbtreeIteratorPrinter(val)
    pretty_printers_dict[re.compile('^std::_Rb_tree_const_iterator<.*>$')] = lambda val: StdRbtreeIteratorPrinter(val)
    pretty_printers_dict[re.compile('^std::_Deque_iterator<.*>$')] = lambda val: StdDequeIteratorPrinter(val)
    pretty_printers_dict[re.compile('^std::_Deque_const_iterator<.*>$')] = lambda val: StdDequeIteratorPrinter(val)
    pretty_printers_dict[re.compile('^__gnu_cxx::__normal_iterator<.*>$')] = lambda val: StdVectorIteratorPrinter(val)
    pretty_printers_dict[re.compile('^__gnu_cxx::_Slist_iterator<.*>$')] = lambda val: StdSlistIteratorPrinter(val)

    # Debug (compiled with -D_GLIBCXX_DEBUG) printer registrations.
    # The Rb_tree debug iterator when unwrapped from the encapsulating __gnu_debug::_Safe_iterator
    # does not have the __norm namespace. Just use the existing printer registration for that.
    pretty_printers_dict[re.compile('^__gnu_debug::_Safe_iterator<.*>$')] = lambda val: StdDebugIteratorPrinter(val)
    pretty_printers_dict[re.compile('^std::__norm::_List_iterator<.*>$')] = lambda val: StdListIteratorPrinter("std::__norm::_List_iterator", val)
    pretty_printers_dict[re.compile('^std::__norm::_List_const_iterator<.*>$')] = lambda val: StdListIteratorPrinter("std::__norm::_List_const_iterator", val)
    pretty_printers_dict[re.compile('^std::__norm::_Deque_const_iterator<.*>$')] = lambda val: StdDequeIteratorPrinter(val)
    pretty_printers_dict[re.compile('^std::__norm::_Deque_iterator<.*>$')] = lambda val: StdDequeIteratorPrinter(val)

gdb.pretty_printers.append(lookup_function)

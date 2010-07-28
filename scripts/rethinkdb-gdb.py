import re


def strip_end_spaces(s):
    end = s.find(r"\275")
    count = s.count(r"\275")
    return s[:end] + r" (...plus \275 * %d)" % count


def lookup_function (val):
    "Look-up and return a pretty-printer that can print val."

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
       if function.search (typename):
           return pretty_printers_dict[function] (val)
           
    # Cannot find a pretty printer.  Return None.
    return None
    

class Btree_FsmPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "========================================\nbtree object with the following members:"

    def children(self):
        yield 'fsm_type', Fsm_TypePrinter(int(self.val['fsm_type'])).to_string()
        yield 'noreply', "false" if self.val['noreply']==False else "true"

        try: yield 'request', "a pointer to a " + str(self.val['request'].dereference())
        except: yield "request", "no request object"

        try: yield 'cache', "a pointer to a " + str(self.val['cache'].dereference())
        except RuntimeError: yield "cache", "no cache object"

        try: yield 'on_complete', "a pointer to a " + str(self.val['on_complete'].dereference()) 
        except RuntimeError: yield "on_complete", "no on_complete object"
        
        try: yield "transaction", "a pointer to a " + str(self.val['transaction'].dereference())
        except RuntimeError: yield "transaction", "no transaction object"

        try: yield 'prev', "a pointer to a " + str(self.val['prev'].dereference())
        except: yield "prev", "no prev object"

        try: yield 'next', "a pointer to a " + str(self.val['next'].dereference())
        except: yield "next", "no next object"

        yield 'type', self.val['type']
        yield 'return_cpu', self.val['return_cpu']
        yield 'key_memory', strip_end_spaces(str(self.val['key_memory']))
        yield 'value_memory', strip_end_spaces(str(self.val['value_memory']))
        
        fsm_type = Fsm_TypePrinter(int(self.val['fsm_type'])).to_string()
        if fsm_type == "btree_set_fsm":
            yield "state", self.val['state']
            try: yield "key", "a pointer to a " + str(self.val['key'].dereference())
            except RuntimeError: yield "key", "no key object"

            try: yield "value", "a pointer to a " + str(self.val['value'].dereference())
            except RuntimeError: yield "value", "no value object"

            try: yield "sb_buf", "a pointer to a " + str(self.val['sb_buf'].dereference())
            except RuntimeError: yield "sb_buf", "no sb_buf object"

            try: yield "buf", "a pointer to a " + str(self.val['buf'].dereference())
            except RuntimeError: yield "buf", "no buf object"

            try: yield "last_buf", "a pointer to a " + str(self.val['last_buf'].dereference())
            except RuntimeError: yield "last_buf", "no last_buf object"

            yield "node_id", self.val['node_id']
            yield "last_node_id", self.val['last_node_id']
            yield "set_type", self.val['set_type']
            yield "set_was_successful", self.val['set_was_successful']
        elif fsm_type == "btree_get_fsm":
            yield "state", self.val['state']
            yield "key_memory", strip_end_spaces(str(self.val['key_memory']))
            yield "value_memory", strip_end_spaces(str(self.val['value_memory']))

            try: yield "buf", "a pointer to a " + str(self.val['buf'].dereference())
            except RuntimeError: yield "buf", "no buf object"

            try: yield "last_buf", "a pointer to a " + str(self.val['last_buf'].dereference())
            except RuntimeError: yield "last_buf", "no last_buf object"

            yield "node_id", self.val['node_id']
        elif fsm_type == "btree_delete_fsm":
            yield "state", self.val['state']
            yield "key_memory", strip_end_spaces(str(self.val['key_memory']))
            yield "node_id", self.val['node_id']
            yield "last_node_id", self.val['last_node_id']
            yield "sib_node_id", self.val['sib_node_id']

            try: yield "key", "a pointer to a " + str(self.val['key'].dereference())
            except RuntimeError: yield "key", "no key object"

            try: yield "sb_buf", "a pointer to a " + str(self.val['sb_buf'].dereference())
            except RuntimeError: yield "sb_buf", "no sb_buf object"

            try: yield "sib_buf", "a pointer to a " + str(self.val['sib_buf'].dereference())
            except RuntimeError: yield "sib_buf", "no sib_buf object"

            try: yield "last_buf", "a pointer to a " + str(self.val['last_buf'].dereference())
            except RuntimeError: yield "last_buf", "no last_buf object"


class Fsm_TypePrinter:
    def __init__(self, val):
        self.val = int(val)

    def to_string(self):
        a = ["btree_mock_fsm", "btree_get_fsm", "btree_set_fsm", "btree_delete_fsm"]
        return a[int(self.val)]


class RequestPrinter:
    def __init__(self, val=None):
        self.val = val

    def to_string(self):
        return "request object with the following members:"
    
    def children(self):
        yield 'nstarted', 1# int(self.val['nstarted'])
        yield 'ncompleted',2# int(self.val['ncompleted'])
    
    
class CachePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "cache object with the following members:"
    
    def children(self):
        yield 'n_trans_created', int(self.val['n_trans_created'])
        yield 'n_trans_freed', int(self.val['n_trans_freed'])
        yield 'n_blocks_acquired', int(self.val['n_blocks_acquired'])
        yield 'n_blocks_released', int(self.val['n_blocks_released'])


class TransactionPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "transaction object with the following members:"
    
    def children(self):
        try: yield 'cache', "a pointer to a " + str(self.val['cache'].dereference())
        except RuntimeError: yield "cache", "no cache object"

        try: yield 'access', "a pointer to a " + str(self.val['access'].dereference())
        except RuntimeError: yield "access", "no access object"
        
        try: yield "begin_callback", "a pointer to a" + str(self.val['begin_callback'].dereference())
        except RuntimeError: yield "begin_callback", "no transaction_begin_callback object"

        try: yield "commit_callback", "a pointer to a" + str(self.val['commit_callback'].dereference())
        except RuntimeError: yield "commit_callback", "no transaction_commit_callback object"


class AccessPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "access object with the following members:"
    
    def children(self):
        pass


class Transaction_Begin_CallbackPrinter:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "transaction_begin_callback object"


class Transaction_Commit_CallbackPrinter:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "transaction_commit_callback object"


class Cpu_MessagePrinter:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "cpu_message object"


class Btree_KeyPrinter:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "btree_key object with the following members:"
    def children(self):
        yield "size", self.val['size']
        yield "contents", strip_end_spaces(str(self.val['contents']))


class Btree_ValuePrinter:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "btree_value object with the following members:"
    def children(self):
        yield "size", self.val['size']
        yield "contents", strip_end_spaces(str(self.val['contents']))


class BufPrinter:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "buf object"


class Msg_TypePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = ["mt_btree", "mt_lock" , "mt_perfmon"]
        return a[int(self.val)]


class StatePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = ["uninitialized", "start_transaction", "acquire_superblock", "acquire_root", "insert_root", "insert_root_on_split", "acquire_node", "update_complete", "committing"]
        return a[int(self.val)]


class Btree_Set_TypePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = ["btree_set_type_set", "btree_set_type_add", "btree_set_type_replace", "btree_set_type_incr", "btree_set_type_decr"]
        return a[int(self.val)]


pretty_printers_dict = {}
pretty_printers_dict[re.compile ('.*state.*')]                           = StatePrinter
pretty_printers_dict[re.compile ('.*fsm.*')]                       = Btree_FsmPrinter
pretty_printers_dict[re.compile ('.*cpu_message.*')]                     = Cpu_MessagePrinter
pretty_printers_dict[re.compile ('.*fsm_type.*')]                        = Fsm_TypePrinter
pretty_printers_dict[re.compile ('.*request.*')]                         = RequestPrinter
pretty_printers_dict[re.compile ('.*transaction.*')]                     = TransactionPrinter
pretty_printers_dict[re.compile ('.*access.*')]                          = AccessPrinter
pretty_printers_dict[re.compile ('.*cache.*')]                           = CachePrinter
pretty_printers_dict[re.compile ('.*transaction_begin_callback.*')]      = Transaction_Begin_CallbackPrinter
pretty_printers_dict[re.compile ('.*transaction_commit_callback.*')]     = Transaction_Commit_CallbackPrinter
pretty_printers_dict[re.compile ('.*msg_type.*')]                        = Msg_TypePrinter
pretty_printers_dict[re.compile ('.*btree_key.*')]                       = Btree_KeyPrinter
pretty_printers_dict[re.compile ('.*btree_value.*')]                     = Btree_ValuePrinter
pretty_printers_dict[re.compile ('.*buf.*')]                             = BufPrinter
pretty_printers_dict[re.compile ('.*btree_set_type.*')]                  = Btree_Set_TypePrinter
gdb.pretty_printers.append (lookup_function)
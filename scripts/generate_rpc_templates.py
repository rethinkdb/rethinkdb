#!/usr/bin/env python
# Copyright 2010-2012 RethinkDB, all rights reserved.
import sys

"""This script is used to generate the mailbox templates in
`rethinkdb/src/rpc/mailbox/typed.hpp`. It is meant to be run as follows
(assuming that the current directory is `rethinkdb/src/`):

$ ../scripts/generate_rpc_templates.py > rpc/mailbox/typed.hpp

"""

def ncsep(template, nargs):
    return ", ".join(template.replace("#", str(i)) for i in xrange(nargs))

def ncpre(template, nargs):
    return "".join(", " + template.replace("#", str(i)) for i in xrange(nargs))

def generate_async_message_template(nargs):

    def csep(template):
        return ncsep(template, nargs)

    def cpre(template):
        return ncpre(template, nargs)

    mailbox_t_str = "mailbox_t< void(%s) >" % csep("arg#_t")
    print
    print "template<%s>" % csep("class arg#_t")
    print "class %s {" % mailbox_t_str
    print "    class fun_runner_t;"
    print "    class write_impl_t : public mailbox_write_callback_t {"
    if nargs == 0:
        print "    public:"
        print "        write_impl_t() { }"
    else:
        print "    private:"
        print "        friend class fun_runner_t;"
        for i in xrange(nargs):
            print "        const arg%d_t &arg%d;" % (i, i)
        print "    public:"
        if nargs == 1:
            print "        explicit write_impl_t(%s) :" % csep("const arg#_t& _arg#")
        else:
            print "        write_impl_t(%s) :" % csep("const arg#_t& _arg#")
        print "            %s" % csep("arg#(_arg#)")
        print "        { }"
    print "        void write(write_stream_t *stream) {"
    print "            write_message_t msg;"
    for i in xrange(nargs):
        print "            msg << arg%d;" % i
    print "            int res = send_write_message(stream, &msg);"
    print "            if (res) { throw fake_archive_exc_t(); }"
    print "        }"
    print "    };"
    print
    print "    class fun_runner_t {"
    print "    public:"
    print "        fun_runner_t(%s *_parent," % mailbox_t_str
    print "                     int _thread_id,"
    print "                     %sread_stream_t *stream) :" % ("UNUSED " if nargs == 0 else "")
    print "            parent(_parent), thread_id(_thread_id) {"
    for i in xrange(nargs):
        print "            %sres = deserialize(stream, &arg%d);" % ("int " if i == 0 else "", i)
        print "            if (res) { throw fake_archive_exc_t(); }"
    print "        }"
    print
    print "        void run() {"
    print "            scoped_ptr_t<fun_runner_t> self_deleter(this);"
    print "            // Save these on the stack in case the mailbox is deleted during thread switch"
    print "            mailbox_manager_t *manager = parent->mailbox.get_manager();"
    print "            uint64_t mbox_id = parent->mailbox.get_id();"
    print "            on_thread_t rethreader(thread_id);"
    print "            if (!manager->check_existence(mbox_id)) {"
    print "                return;"
    print "            }"
    print "            parent->fun(%s);" % csep("arg#")
    print "        }"
    print
    print "        %s *parent;" % mailbox_t_str
    print "        int thread_id;"
    for i in xrange(nargs):
        print "        arg%d_t arg%d;" % (i, i)
    print "    };"
    print
    print "    class read_impl_t : public mailbox_read_callback_t {"
    print "    public:"
    print "        explicit read_impl_t(%s *_parent) : parent(_parent) { }" % mailbox_t_str
    if nargs == 0:
        print "        void read(UNUSED read_stream_t *stream, int thread_id) {"
    else:
        print "        void read(read_stream_t *stream, int thread_id) {"
    print "            fun_runner_t *runner = new fun_runner_t(parent, thread_id, stream);"
    print "            coro_t::spawn_now_dangerously(boost::bind(&%s::fun_runner_t::run," % mailbox_t_str
    print "                                                      runner));"
    print "        }"
    print "    private:"
    print "        %s *parent;" % mailbox_t_str
    print "    };"
    print
    print "    read_impl_t reader;"
    print
    print "public:"
    print "    typedef mailbox_addr_t< void(%s) > address_t;" % csep("arg#_t")
    print
    print "    mailbox_t(mailbox_manager_t *manager,"
    print "              const boost::function< void(%s)> &f," % csep("arg#_t")
    print "              mailbox_callback_mode_t cbm = mailbox_callback_mode_coroutine) :"
    print "        reader(this), fun(f), callback_mode(cbm), mailbox(manager, &reader)"
    print "        { }"
    print
    print "    address_t get_address() const {"
    print "        address_t a;"
    print "        a.addr = mailbox.get_address();"
    print "        return a;"
    print "    }"
    print
    print "private:"
    if nargs == 0:
        print "    friend void send(mailbox_manager_t*, address_t);"
    else:
        print "    template<%s>" % csep("class a#_t")
        print "    friend void send(mailbox_manager_t*,"
        print "                     typename mailbox_t< void(%s) >::address_t%s);" % (csep("a#_t"), cpre("const a#_t&"))
    print
    print "    boost::function< void(%s) > fun;" % csep("arg#_t")
    print "    mailbox_callback_mode_t callback_mode;"
    print "    raw_mailbox_t mailbox;"
    print "};"
    print
    if nargs == 0:
        print "inline"
    else:
        print "template<%s>" % csep("class arg#_t")
    print "void send(mailbox_manager_t *src,"
    print "          %s %s::address_t dest%s) {" % (("typename" if nargs > 0 else ""),
                                                    mailbox_t_str,
                                                    cpre("const arg#_t &arg#"))
    if nargs == 0:
        print "    %s::write_impl_t writer;" % mailbox_t_str
    else:
        print "    typename %s::write_impl_t writer(%s);" % (mailbox_t_str, csep("arg#"))
    print "    send(src, dest.addr, &writer);"
    print "}"
    print

if __name__ == "__main__":
    print "// Copyright 2010-2012 RethinkDB, all rights reserved."
    print "#ifndef RPC_MAILBOX_TYPED_HPP_"
    print "#define RPC_MAILBOX_TYPED_HPP_"
    print

    print "/* This file is automatically generated by '%s'." % " ".join(sys.argv)
    print "Please modify '%s' instead of modifying this file.*/" % sys.argv[0]
    print

    print "#include \"errors.hpp\""
    print "#include <boost/bind.hpp>"
    print "#include <boost/function.hpp>"
    print
    print "#include \"containers/archive/archive.hpp\""
    print "#include \"rpc/serialize_macros.hpp\""
    print "#include \"rpc/mailbox/mailbox.hpp\""
    print
    print "/* If you pass `mailbox_callback_mode_coroutine` to the `mailbox_t` "
    print "constructor, it will spawn the callback in a new coroutine. If you "
    print "`mailbox_callback_mode_inline`, it will call the callback inline "
    print "and the callback must not block. The former is the default for "
    print "historical reasons, but the latter is better. Eventually the former "
    print "will go away. */"
    print "enum mailbox_callback_mode_t {"
    print "    mailbox_callback_mode_coroutine,"
    print "    mailbox_callback_mode_inline"
    print "};"
    print
    print "template <class> class mailbox_t;"
    print
    print "template <class T>"
    print "class mailbox_addr_t {"
    print "public:"
    print "    bool is_nil() const { return addr.is_nil(); }"
    print "    peer_id_t get_peer() const { return addr.get_peer(); }"
    print
    print "    friend class mailbox_t<T>;"
    print
    print "    RDB_MAKE_ME_SERIALIZABLE_1(addr);"
    print
    print "private:"
    print "    friend void send(mailbox_manager_t *, mailbox_addr_t<void()>);"
    for nargs in xrange(1,15):
        print "    template <%s>" % ncsep("class a#_t", nargs)
        print "    friend void send(mailbox_manager_t *,"
        print "                     typename mailbox_t< void(%s) >::address_t%s);" % (ncsep("a#_t", nargs), ncpre("const a#_t&", nargs))
    print
    print "    raw_mailbox_t::address_t addr;"
    print "};"

    for nargs in xrange(15):
        generate_async_message_template(nargs)

    print "#endif // RPC_MAILBOX_TYPED_HPP_"

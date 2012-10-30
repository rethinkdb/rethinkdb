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

    print
    print "template<" + csep("class arg#_t") + ">"
    print "class mailbox_t< void(" + csep("arg#_t") + ") > {"
    print "    class read_impl_t;"
    print "    class write_impl_t : public mailbox_write_callback_t {"
    if nargs == 0:
        print "    public:"
        print "        write_impl_t() { }"
    else:
        print "    private:"
        print "        friend class read_impl_t;"
        for i in xrange(nargs):
            print "        const arg%d_t &arg%d;" % (i, i)
        print "    public:"
        if nargs == 1:
            print "        explicit write_impl_t(" + csep("const arg#_t& _arg#") + ") :"
        else:
            print "        write_impl_t(" + csep("const arg#_t& _arg#") + ") :"
        print "            " + csep("arg#(_arg#)")
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
    print "    class read_impl_t : public mailbox_read_callback_t {"
    print "    public:"
    print "        explicit read_impl_t(mailbox_t< void(" + csep("arg#_t") + ") > *_parent) : parent(_parent) { }"
    if nargs == 0:
        print "        void read(UNUSED read_stream_t *stream) {"
    else:
        print "        void read(read_stream_t *stream) {"
    for i in xrange(nargs):
        print "            arg%d_t arg%d;" % (i, i)

    for i in xrange(nargs):
        print "            %sres = deserialize(stream, &arg%d);" % ("int " if i == 0 else "", i)
        print "            if (res) { throw fake_archive_exc_t(); }"
    print "            if (parent->callback_mode == mailbox_callback_mode_coroutine) {"
    print "                coro_t::spawn_sometime(boost::bind(parent->fun" + cpre("arg#") + "));"
    print "            } else {"
    print "                parent->fun(" + csep("arg#") + ");"
    print "            }"
    print "        }"
    print
    if nargs == 0:
        print "        void read(UNUSED mailbox_write_callback_t *_writer) {"
    else:
        print "        void read(mailbox_write_callback_t *_writer) {"
        print "            write_impl_t *writer = static_cast<write_impl_t*>(_writer);"
    print "            if (parent->callback_mode == mailbox_callback_mode_coroutine) {"
    print "                coro_t::spawn_sometime(boost::bind(parent->fun" + cpre("writer->arg#") + "));"
    print "            } else {"
    print "                parent->fun(" + csep("writer->arg#") + ");"
    print "            }"
    print "        }"
    print "    private:"
    print "        mailbox_t< void(" + csep("arg#_t") + ") > *parent;"
    print "    };"
    print
    print "    read_impl_t reader;"
    print
    print "public:"
    print "    typedef mailbox_addr_t< void(" + csep("arg#_t") + ") > address_t;"
    print
    print "    mailbox_t(mailbox_manager_t *manager, const boost::function< void(" + csep("arg#_t") + ") > &f, mailbox_callback_mode_t cbm = mailbox_callback_mode_coroutine, mailbox_thread_mode_t tm = mailbox_home_thread) :"
    print "        reader(this), fun(f), callback_mode(cbm), mailbox(manager, tm, &reader)"
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
        print "    template<" + csep("class a#_t") + ">"
        print "    friend void send(mailbox_manager_t*, typename mailbox_t< void(" + csep("a#_t") + ") >::address_t" + cpre("const a#_t&") + ");"
    print
    print "    boost::function< void(" + csep("arg#_t") + ") > fun;"
    print "    mailbox_callback_mode_t callback_mode;"
    print "    raw_mailbox_t mailbox;"
    print "};"
    print
    if nargs == 0:
        print "inline"
    else:
        print "template<" + csep("class arg#_t") + ">"
    print "void send(mailbox_manager_t *src, " + ("typename " if nargs > 0 else "") + "mailbox_t< void(" + csep("arg#_t") + ") >::address_t dest" + cpre("const arg#_t &arg#") + ") {"
    if nargs == 0:
        print "    mailbox_t< void(" + csep("arg#_t") + ") >::write_impl_t writer;"
    else:
        print "    typename mailbox_t< void(" + csep("arg#_t") + ") >::write_impl_t writer(" + csep("arg#") + ");"
    print "    send(src, dest.addr, &writer);"
    print "}"
    print

if __name__ == "__main__":

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
        print "    template <" + ncsep("class a#_t", nargs) + ">"
        print "    friend void send(mailbox_manager_t *, typename mailbox_t< void(" + ncsep("a#_t", nargs) + ") >::address_t" + ncpre("const a#_t&", nargs) + ");"
    print
    print "    raw_mailbox_t::address_t addr;"
    print "};"

    for nargs in xrange(15):
        generate_async_message_template(nargs)

    print "#endif // RPC_MAILBOX_TYPED_HPP_"

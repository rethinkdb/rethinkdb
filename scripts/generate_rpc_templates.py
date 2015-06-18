#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
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
    print "    class write_impl_t : public mailbox_write_callback_t {"
    if nargs == 0:
        print "    public:"
        print "        write_impl_t() { }"
    else:
        print "    private:"
        for i in xrange(nargs):
            print "        const arg%d_t &arg%d;" % (i, i)
        print "    public:"
        if nargs == 1:
            print "        explicit write_impl_t(%s) :" % csep("const arg#_t& _arg#")
        else:
            print "        write_impl_t(%s) :" % csep("const arg#_t& _arg#")
        print "            %s" % csep("arg#(_arg#)")
        print "        { }"
    if nargs == 0:
        print "        void write(DEBUG_VAR cluster_version_t cluster_version, write_message_t *) {"
    else:
        print "        void write(DEBUG_VAR cluster_version_t cluster_version, write_message_t *wm) {"
    print "            rassert(cluster_version == cluster_version_t::CLUSTER);"
    for i in xrange(nargs):
        print "            serialize<cluster_version_t::CLUSTER>(wm, arg%d);" % i
    print "        }"
    print "#ifdef ENABLE_MESSAGE_PROFILER"
    print "        const char *message_profiler_tag() const {"
    if nargs == 0:
        print "            return \"mailbox<>\";"
    else:
        print "            static const std::string tag = "
        print "                strprintf(\"mailbox<%s>\", %s);" % \
            (csep("%s"), csep("typeid(arg#_t).name()"))
        print "            return tag.c_str();"
    print "        }"
    print "#endif"
    print "    };"
    print
    print "    class read_impl_t : public mailbox_read_callback_t {"
    print "    public:"
    print "        explicit read_impl_t(%s *_parent) : parent(_parent) { }" % mailbox_t_str
    if nargs == 0:
        print "        void read(UNUSED read_stream_t *stream, signal_t *interruptor) {"
    else:
        print "        void read(read_stream_t *stream, signal_t *interruptor) {"
    for i in xrange(nargs):
        print "            arg%d_t arg%d;" % (i, i)
        print "            %sres = deserialize<cluster_version_t::CLUSTER>(stream, &arg%d);" % ("archive_result_t " if i == 0 else "", i)
        print "            if (bad(res)) { throw fake_archive_exc_t(); }"
    print "            parent->fun(interruptor%s);" % cpre("std::move(arg#)")
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
    print "              const std::function< void(signal_t *%s)> &f) :" % cpre("arg#_t")
    print "        reader(this), fun(f), mailbox(manager, &reader)"
    print "        { }"
    print
    print "    void begin_shutdown() {"
    print "        mailbox.begin_shutdown();"
    print "    }"
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
    print "    std::function< void(signal_t *%s) > fun;" % cpre("arg#_t")
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
    print "// Copyright 2010-2014 RethinkDB, all rights reserved."
    print "#ifndef RPC_MAILBOX_TYPED_HPP_"
    print "#define RPC_MAILBOX_TYPED_HPP_"
    print

    print "/* This file is automatically generated by '%s'." % " ".join(sys.argv)
    print "Please modify '%s' instead of modifying this file.*/" % sys.argv[0]
    print

    print "#include <functional>"
    print
    print "#include \"containers/archive/versioned.hpp\""
    print "#include \"rpc/serialize_macros.hpp\""
    print "#include \"rpc/mailbox/mailbox.hpp\""
    print "#include \"rpc/semilattice/joins/macros.hpp\""
    print
    print "template <class> class mailbox_t;"
    print
    print "template <class T>"
    print "class mailbox_addr_t {"
    print "public:"
    print "    bool operator<(const mailbox_addr_t<T> &other) const {"
    print "        return addr < other.addr;"
    print "    }"
    print "    bool is_nil() const { return addr.is_nil(); }"
    print "    peer_id_t get_peer() const { return addr.get_peer(); }"
    print
    print "    friend class mailbox_t<T>;"
    print
    print "    RDB_MAKE_ME_SERIALIZABLE_1(mailbox_addr_t, addr);"
    print "    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(mailbox_addr_t<T>, addr);"
    print
    print "private:"
    print "    friend void send(mailbox_manager_t *, mailbox_addr_t<void()>);"
    for nargs in xrange(1, 15):
        print "    template <%s>" % ncsep("class a#_t", nargs)
        print "    friend void send(mailbox_manager_t *,"
        print "                     typename mailbox_t< void(%s) >::address_t%s);" % (ncsep("a#_t", nargs), ncpre("const a#_t&", nargs))
    print
    print "    raw_mailbox_t::address_t addr;"
    print "};"

    for nargs in xrange(15):
        generate_async_message_template(nargs)

    print "#endif // RPC_MAILBOX_TYPED_HPP_"

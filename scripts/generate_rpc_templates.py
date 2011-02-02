#!/usr/bin/env python
import sys

def generate_async_message_template(nargs):

    argtypes = ", ".join("arg%d_t" % i for i in xrange(nargs))

    print "template<%s>" % ", ".join(["int f_id"] + ["class arg%d_t" % i for i in xrange(nargs)])
    print "class async_mailbox_t< f_id, void(%s) > : private cluster_mailbox_t {" % argtypes
    print
    print "public:"
    print "    async_mailbox_t(const boost::function< void(%s) > &fun) :" % argtypes
    print "        callback(fun) { }"
    print
    print "    struct address_t {"
    print "        address_t() { }"
    print "        address_t(const address_t &other) : addr(other.addr) { }"
    print "        address_t(async_mailbox_t *mb) : addr(mb) { }"
    print "        void call(%s) {" % ", ".join("const arg%d_t &arg%d" % (i, i) for i in xrange(nargs))
    print "            unique_ptr_t<message_t> m(new message_t);"
    for i in xrange(nargs): print "            m->arg%d = arg%d;" % (i, i)
    print "            addr.send(m);"
    print "        }"
    print "        static void serialize(cluster_outpipe_t *p, const address_t *addr) {"
    print "            ::serialize(p, &addr->addr);"
    print "        }"
    print "        static void unserialize(cluster_inpipe_t *p, address_t *addr) {"
    print "            ::unserialize(p, &addr->addr);"
    print "        }"
    # print "    private:"    # Make public temporarily
    print "        cluster_address_t addr;"
    print "    };"
    print "    friend class address_t;"
    print
    print "private:"
    print "    struct message_t : public cluster_message_t {"
    for i in xrange(nargs):
        print "        arg%d_t arg%d;" % (i, i)
    print "        void serialize(cluster_outpipe_t *p) {"
    for i in xrange(nargs):
        print "            ::serialize(p, &arg%d);" % i
    print "        }"
    print "    };"
    print "    unique_ptr_t<cluster_message_t> unserialize(cluster_inpipe_t *p) {"
    print "        unique_ptr_t<message_t> mp(new message_t);"
    for i in xrange(nargs):
        print "        ::unserialize(p, &mp->arg%d);" % i
    print "        return mp;"
    print "    }"
    print
    print "    boost::function< void(%s) > callback;" % argtypes
    print "    void run(unique_ptr_t<cluster_message_t> cm) {"
    print "        unique_ptr_t<message_t> m(static_pointer_cast<message_t>(cm));"
    print "        callback(%s);" % ", ".join("m->arg%d" % i for i in xrange(nargs))
    print "    }"
    print "};"
    print

def generate_sync_message_template(nargs, void):

    argtypes = ", ".join("arg%d_t" % i for i in xrange(nargs))
    ret = "ret_t" if not void else "void"

    if void:
        print "template<%s>" % ", ".join(["int f_id"] + ["class arg%d_t" % i for i in xrange(nargs)])
    else:
        print "template<%s>" % ", ".join(["int f_id", "class ret_t"] + ["class arg%d_t" % i for i in xrange(nargs)])
    print "class sync_mailbox_t< f_id, %s(%s) > : private cluster_mailbox_t {" % (ret, argtypes)
    print
    print "public:"
    print "    sync_mailbox_t(const boost::function< %s(%s) > &fun) :" % (ret, argtypes)
    print "        callback(fun) { }"
    print
    print "    struct address_t {"
    print "        address_t() { }"
    print "        address_t(const address_t &other) : addr(other.addr) { }"
    print "        address_t(sync_mailbox_t *mb) : addr(mb) { }"
    print "        %s call(%s) {" % (ret, ", ".join("const arg%d_t &arg%d" % (i, i) for i in xrange(nargs)))
    print "            unique_ptr_t<call_message_t> m(new call_message_t);"
    for i in xrange(nargs): print "            m->arg%d = arg%d;" % (i, i)
    print "            struct : public cluster_mailbox_t, public %s {" % ("promise_t<ret_t>" if not void else "cond_t")
    print "                unique_ptr_t<cluster_message_t> unserialize(cluster_inpipe_t *p) {"
    print "                    unique_ptr_t<ret_message_t> m(new ret_message_t);"
    if not void:
        print "                    ::unserialize(p, &m->ret);"
    print "                    return m;"
    print "                }"
    print "                void run(unique_ptr_t<cluster_message_t> msg) {"
    if not void:
        print "                    unique_ptr_t<ret_message_t> m(static_pointer_cast<ret_message_t>(m));"
        print "                    pulse(m->ret);"
    else:
        print "                    pulse();"
    print "                }"
    print "            } reply_listener;"
    print "            m->reply_to = &reply_listener;"
    print "            addr.send(m);"
    if not void:
        print "            return reply_listener.wait();"
    else:
        print "            reply_listener.wait();"
    print "        }"
    print "        static void serialize(cluster_outpipe_t *p, const address_t *addr) {"
    print "            ::serialize(p, &addr->addr);"
    print "        }"
    print "        static void unserialize(cluster_inpipe_t *p, address_t *addr) {"
    print "            ::unserialize(p, &addr->addr);"
    print "        }"
    print "    private:"
    print "        cluster_address_t addr;"
    print "    };"
    print "    friend class address_t;"
    print
    print "private:"
    print "    struct call_message_t : public cluster_message_t {"
    for i in xrange(nargs):
        print "        arg%d_t arg%d;" % (i, i)
    print "        cluster_address_t reply_to;"
    print "    };"
    print
    print "    struct ret_message_t : public cluster_message_t {"
    if not void:
        print "        ret_t ret;"
    print "    };"
    print
    print "    boost::function< %s(%s) > callback;" % (ret, argtypes)
    print "    void run(unique_ptr_t<cluster_message_t> cm) {"
    print "        unique_ptr_t<call_message_t> m(static_pointer_cast<call_message_t>(cm));"
    print "        unique_ptr_t<ret_message_t> m2(new ret_message_t);"
    if not void:
        print "        m2.ret = callback(%s);" % ", ".join("m->arg%d" % i for i in xrange(nargs))
    else:
        print "        callback(%s);" % ", ".join("m->arg%d" % i for i in xrange(nargs))
    print "        m.reply_to.send(m2);"
    print "    }"
    print "};"
    print

if __name__ == "__main__":

    print "#ifndef __CLUSTERING_RPC_HPP__"
    print "#define __CLUSTERING_RPC_HPP__"
    print

    print "/* This file is automatically generated by '%s'." % " ".join(sys.argv)
    print "Please modify '%s' instead of modifying this file.*/" % sys.argv[0]
    print

    print "#include \"clustering/serialize.hpp\""
    print "#include \"concurrency/cond_var.hpp\""
    print

    print "template<int format_id, class proto_t> class async_mailbox_t;"
    print "template<int format_id, class proto_t> class sync_mailbox_t;"
    print

    for nargs in xrange(10):
        generate_async_message_template(nargs)
        generate_sync_message_template(nargs, True);
        generate_sync_message_template(nargs, False);

    print
    print "#endif /* __CLUSTERING_RPC_HPP__ */"

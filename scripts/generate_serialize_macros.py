#!/usr/bin/env python
# Copyright 2010-2014 RethinkDB, all rights reserved.
import sys

"""This script is used to generate the RDB_MAKE_SERIALIZABLE_*() and
RDB_MAKE_ME_SERIALIZABLE_*() macro definitions. Because there are so
many variations, and because they are so similar, it's easier to just
have a Python script to generate them.

This script is meant to be run as follows (assuming you are in the
"rethinkdb/src" directory):

$ ../scripts/generate_serialize_macros.py > rpc/serialize_macros.hpp

"""

def generate_make_serializable_macro(nfields):
    fields = "".join(", field%d" % (i + 1) for i in xrange(nfields))
    print "#define RDB_MAKE_SERIALIZABLE_%d(type_t%s) \\" % \
        (nfields, fields)
    zeroarg = ("UNUSED " if nfields == 0 else "")
    print "    template <cluster_version_t W> \\"
    print "    void serialize(%swrite_message_t *wm, %sconst type_t &thing) { \\" % (zeroarg, zeroarg)
    for i in xrange(nfields):
        print "        serialize<W>(wm, thing.field%d); \\" % (i + 1)
    print "    } \\"
    print "    template <cluster_version_t W> \\"
    print "    archive_result_t deserialize(%sread_stream_t *s, %stype_t *thing) { \\" % (zeroarg, zeroarg)
    print "        archive_result_t res = archive_result_t::SUCCESS; \\"
    for i in xrange(nfields):
        print "        res = deserialize<W>(s, deserialize_deref(thing->field%d)); \\" % (i + 1)
        print "        if (bad(res)) { return res; } \\"
    print "        return res; \\"
    print "    } \\"
    print "    extern int dont_use_RDB_MAKE_SERIALIZABLE_within_a_class_body"
    print

    print "#define RDB_MAKE_SERIALIZABLE_%d_FOR_CLUSTER(type_t%s) \\" % \
        (nfields, fields)
    zeroarg = ("UNUSED " if nfields == 0 else "")
    print "    template <> \\"
    print "    void serialize<cluster_version_t::CLUSTER>( \\"
    print "        %swrite_message_t *wm, %sconst type_t &thing) { \\" % (zeroarg, zeroarg)
    for i in xrange(nfields):
        print "        serialize<cluster_version_t::CLUSTER>(wm, thing.field%d); \\" % (i + 1)
    print "    } \\"
    print "    template <> \\"
    print "    archive_result_t deserialize<cluster_version_t::CLUSTER>( \\"
    print "        %sread_stream_t *s, %stype_t *thing) { \\" % (zeroarg, zeroarg)
    print "        archive_result_t res = archive_result_t::SUCCESS; \\"
    for i in xrange(nfields):
        print "        res = deserialize<cluster_version_t::CLUSTER>( \\"
        print "            s, deserialize_deref(thing->field%d)); \\" % (i + 1)
        print "        if (bad(res)) { return res; } \\"
    print "        return res; \\"
    print "    } \\"
    print "    extern int dont_use_RDB_MAKE_SERIALIZABLE_FOR_CLUSTER_within_a_class_body"
    print

    # See the note in the comment below.
    print "#define RDB_IMPL_SERIALIZABLE_%d(type_t%s) RDB_MAKE_SERIALIZABLE_%d(type_t%s); \\" % (nfields, fields, nfields, fields)
    print
    print "#define RDB_IMPL_SERIALIZABLE_%d_FOR_CLUSTER(type_t%s) \\" % (nfields, fields)
    print "    RDB_MAKE_SERIALIZABLE_%d_FOR_CLUSTER(type_t%s); \\" % (nfields, fields)
    print "    INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(type_t);"
    print
    print "#define RDB_IMPL_SERIALIZABLE_%d_SINCE_v1_13(type_t%s) \\" % (nfields, fields)
    print "    RDB_IMPL_SERIALIZABLE_%d(type_t%s); \\" % (nfields, fields)
    print "    INSTANTIATE_SERIALIZABLE_SINCE_v1_13(type_t)"
    print
    print "#define RDB_IMPL_SERIALIZABLE_%d_SINCE_v1_16(type_t%s) \\" % (nfields, fields)
    print "    RDB_IMPL_SERIALIZABLE_%d(type_t%s); \\" % (nfields, fields)
    print "    INSTANTIATE_SERIALIZABLE_SINCE_v1_16(type_t)"

def generate_make_me_serializable_macro(nfields):
    print "#define RDB_MAKE_ME_SERIALIZABLE_%d(%s) \\" % \
        (nfields, ", ".join("field%d" % (i + 1) for i in xrange(nfields)))
    zeroarg = ("UNUSED " if nfields == 0 else "")
    print "    friend class write_message_t; \\"
    print "    template <cluster_version_t W> \\"
    print "    void rdb_serialize(%swrite_message_t *wm) const { \\" % zeroarg
    for i in xrange(nfields):
        print "        serialize<W>(wm, field%d); \\" % (i + 1)
    print "    } \\"
    print "    template <cluster_version_t W> \\"
    print "    archive_result_t rdb_deserialize(%sread_stream_t *s) { \\" % zeroarg
    print "        archive_result_t res = archive_result_t::SUCCESS; \\"
    for i in xrange(nfields):
        print "        res = deserialize<W>(s, deserialize_deref(field%d)); \\" % (i + 1)
        print "        if (bad(res)) { return res; } \\"
    print "        return res; \\"
    print "    } \\"
    print "    friend class archive_deserializer_t"

def generate_impl_me_serializable_macro(nfields):
    print "#define RDB_IMPL_ME_SERIALIZABLE_%d(typ%s) \\" % \
        (nfields, "".join(", field%d" % (i + 1) for i in xrange(nfields)))
    zeroarg = ("UNUSED " if nfields == 0 else "")
    print "    template <cluster_version_t W> \\"
    print "    void typ::rdb_serialize(%swrite_message_t *wm) const { \\" % zeroarg
    for i in xrange(nfields):
        print "        serialize<W>(wm, field%d); \\" % (i + 1)
    print "    } \\"
    print "    template <cluster_version_t W> \\"
    print "    archive_result_t typ::rdb_deserialize(%sread_stream_t *s) { \\" % zeroarg
    print "        archive_result_t res = archive_result_t::SUCCESS; \\"
    for i in xrange(nfields):
        print "        res = deserialize<W>(s, deserialize_deref(field%d)); \\" % (i + 1)
        print "        if (bad(res)) { return res; } \\"
    print "        return res; \\"
    print "    } \\"
    print

    print "#define RDB_IMPL_ME_SERIALIZABLE_%d_FOR_CLUSTER(typ%s) \\" % \
        (nfields, "".join(", field%d" % (i + 1) for i in xrange(nfields)))
    zeroarg = ("UNUSED " if nfields == 0 else "")
    print "    template <> \\"
    print "    void typ::rdb_serialize<cluster_version_t::CLUSTER>( \\"
    print "        %swrite_message_t *wm) const { \\" % zeroarg
    for i in xrange(nfields):
        print "        serialize<cluster_version_t::CLUSTER>(wm, field%d); \\" % (i + 1)
    print "    } \\"
    print "    template <> \\"
    print "    archive_result_t typ::rdb_deserialize<cluster_version_t::CLUSTER>( \\"
    print "        %sread_stream_t *s) { \\" % zeroarg
    print "        archive_result_t res = archive_result_t::SUCCESS; \\"
    for i in xrange(nfields):
        print "        res = deserialize<cluster_version_t::CLUSTER>( \\"
        print "            s, deserialize_deref(field%d)); \\" % (i + 1)
        print "        if (bad(res)) { return res; } \\"
    print "        return res; \\"
    print "    } \\"
    print "    INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(typ)"
    print

    print "#define RDB_IMPL_ME_SERIALIZABLE_%d_SINCE_v1_13(typ%s) \\" % \
       (nfields, "".join(", field%d" % (i + 1) for i in xrange(nfields)))
    print "    RDB_IMPL_ME_SERIALIZABLE_%d(typ%s); \\" % \
       (nfields, "".join(", field%d" % (i + 1) for i in xrange(nfields)))
    print "    INSTANTIATE_SERIALIZABLE_SELF_SINCE_v1_13(typ)"


if __name__ == "__main__":

    print "// Copyright 2010-2014 RethinkDB, all rights reserved."
    print "#ifndef RPC_SERIALIZE_MACROS_HPP_"
    print "#define RPC_SERIALIZE_MACROS_HPP_"
    print

    print "/* This file is automatically generated by '%s'." % " ".join(sys.argv)
    print "Please modify '%s' instead of modifying this file.*/" % sys.argv[0]
    print

    print "#include <type_traits>"
    print

    print "#include \"containers/archive/archive.hpp\""
    print "#include \"containers/archive/versioned.hpp\""
    print "#include \"errors.hpp\""
    print "#include \"version.hpp\""
    print

    print """
/* The purpose of these macros is to make it easier to serialize and
unserialize data types that consist of a simple series of fields, each
of which is serializable. Suppose we have a type "struct point_t {
int32_t x, y; }" that we want to be able to serialize. To make it
serializable automatically, either write
RDB_MAKE_SERIALIZABLE_2(point_t, x, y) at the global scope, or write
RDB_MAKE_ME_SERIALIZABLE(x, y) within the body of the point_t type and
RDB_SERIALIZE_OUTSIDE(point_t) in the global scope.

The _FOR_CLUSTER variants of the macros exist to indicate that a type
can only be serialized for use within the cluster, thus should not be
serialized to disk.

The _SINCE_v1_13 variants of the macros exist to make the conversion to
versioned serialization easier. They must only be used for types which
serialization format has not changed since version 1.13.0.
Once the format changes, you can still use the macros without
the _SINCE_v1_13 suffix and instantiate the serialize() and deserialize()
functions explicitly for a certain version.

We use dummy "extern int" declarations to force a compile error in
macros that should not be used inside of class bodies. */
    """.strip()
    print "namespace helper {"
    print
    print "/* When a `static_assert` is used within a templated class or function,"
    print " * but does not depend on any template parameters the C++ compiler is free"
    print " * to evaluate the assert even before instantiating that template. This"
    print " * helper class allows a `static_assert(false, ...)` to depend on the"
    print " * `cluster_version_t` template parameter."
    print " * Also see http://stackoverflow.com/a/14637534. */"
    print "template <cluster_version_t W>"
    print "struct always_false"
    print "    : std::false_type { };"
    print
    print "} // namespace helper"
    print
    print "#define RDB_DECLARE_SERIALIZABLE(type_t) \\"
    print "    template <cluster_version_t W> \\"
    print "    void serialize(write_message_t *, const type_t &); \\"
    print "    template <cluster_version_t W> \\"
    print "    archive_result_t deserialize(read_stream_t *s, type_t *thing)"
    print
    print "#define RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(type_t) \\"
    print "    template <cluster_version_t W> \\"
    print "    void serialize(write_message_t *, const type_t &) { \\"
    print "        static_assert(helper::always_false<W>::value, \\"
    print "                      \"This type is only serializable for cluster.\"); \\"
    print "        unreachable(); \\"
    print "    } \\"
    print "    template <> \\"
    print "    void serialize<cluster_version_t::CLUSTER>( \\"
    print "        write_message_t *, const type_t &); \\"
    print "    template <cluster_version_t W> \\"
    print "    archive_result_t deserialize(read_stream_t *, type_t *) { \\"
    print "        static_assert(helper::always_false<W>::value, \\"
    print "                      \"This type is only deserializable for cluster.\"); \\"
    print "        unreachable(); \\"
    print "    } \\"
    print "    template <> \\"
    print "    archive_result_t deserialize<cluster_version_t::CLUSTER>( \\"
    print "        read_stream_t *s, type_t *thing)"
    print
    print "#define RDB_DECLARE_ME_SERIALIZABLE \\"
    print "    friend class write_message_t; \\"
    print "    template <cluster_version_t W> \\"
    print "    void rdb_serialize(write_message_t *wm) const; \\"
    print "    friend class archive_deserializer_t; \\"
    print "    template <cluster_version_t W> \\"
    print "    archive_result_t rdb_deserialize(read_stream_t *s)"
    print
    print "#define RDB_SERIALIZE_OUTSIDE(type_t) \\"
    print "    template <cluster_version_t W> \\"
    print "    void serialize(write_message_t *wm, const type_t &thing) { \\"
    print "        thing.template rdb_serialize<W>(wm); \\"
    print "    } \\"
    print "    template <cluster_version_t W> \\"
    print "    MUST_USE archive_result_t deserialize(read_stream_t *s, type_t *thing) { \\"
    print "        return thing->template rdb_deserialize<W>(s); \\"
    print "    } \\"
    print "    extern int dont_use_RDB_SERIALIZE_OUTSIDE_within_a_class_body"
    print
    print "#define RDB_SERIALIZE_TEMPLATED_OUTSIDE(type_t) \\"
    print "    template <cluster_version_t W, class T> \\"
    print "    void serialize(write_message_t *wm, const type_t &thing) { \\"
    print "        thing.template rdb_serialize<W>(wm); \\"
    print "    } \\"
    print "    template <cluster_version_t W, class T> \\"
    print "    MUST_USE archive_result_t deserialize(read_stream_t *s, type_t *thing) { \\"
    print "        return thing->template rdb_deserialize<W>(s); \\"
    print "    } \\"
    print "    extern int dont_use_RDB_SERIALIZE_OUTSIDE_within_a_class_body"
    print
    print "#define RDB_SERIALIZE_TEMPLATED_2_OUTSIDE(type_t) \\"
    print "    template <cluster_version_t W, class T, class U> \\"
    print "    void serialize(write_message_t *wm, const type_t &thing) { \\"
    print "        thing.template rdb_serialize<W>(wm); \\"
    print "    } \\"
    print "    template <cluster_version_t W, class T, class U> \\"
    print "    MUST_USE archive_result_t deserialize(read_stream_t *s, type_t *thing) { \\"
    print "        return thing->template rdb_deserialize<W>(s); \\"
    print "    } \\"
    print "    extern int dont_use_RDB_SERIALIZE_OUTSIDE_within_a_class_body"
    print
    for nfields in xrange(20):
        generate_make_serializable_macro(nfields)
        print
        generate_make_me_serializable_macro(nfields)
        print
        generate_impl_me_serializable_macro(nfields)
        print

    print "#endif // RPC_SERIALIZE_MACROS_HPP_"

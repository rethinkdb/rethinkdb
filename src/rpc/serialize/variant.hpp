#ifndef __RPC_SERIALIZE_VARIANT_HPP__
#define __RPC_SERIALIZE_VARIANT_HPP__

#include "rpc/serialize/serialize.hpp"
#include "rpc/serialize/basic.hpp"
#include <boost/variant.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/empty.hpp>

/* Serializing and unserializing boost::variant of serializable types */

/* Functor to write the appropriate member of a variant */
struct serialize_variant_visitor_t : public boost::static_visitor<> {
    cluster_outpipe_t *conn;
    template<class T>
    void operator()(const T &v) {
        serialize(conn, v);
    }
};

/* BOOST_VARIANT_ENUM_PARAMS allows for templatization on all different possible boost::variants. */

template<BOOST_VARIANT_ENUM_PARAMS(class T)>
void serialize(cluster_outpipe_t *conn, const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &v) {
    int which = v.which();
    serialize(conn, which);
    serialize_variant_visitor_t functor;
    functor.conn = conn;
    boost::apply_visitor(functor, v);
}

/* Functor to get the size of the right member of the variant */
struct ser_size_variant_visitor_t : public boost::static_visitor<int> {
    template<class T>
    int operator()(const T &v) const {
        return ser_size(v);
    }
};

template<BOOST_VARIANT_ENUM_PARAMS(class T)>
int ser_size(const boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> &v) {
    return ::ser_size(v.which()) + boost::apply_visitor(ser_size_variant_visitor_t(), v);
}

/* We need to be sure to pick the right class for unserialization. To do this we use
boost::mpl to iterate along a "list" of types until we get to the right one. This code
is adapted from boost::serialization's handing of variants. */
template<class typelist_t, class variant_t>
struct unserialize_nth_from_typelist_t {

    struct load_null {
        static void invoke(cluster_inpipe_t *inpipe, unserialize_extra_storage_t *es, variant_t *out, int which) {
            crash("The number which is supposed to identify which type the variant was is too "
                "high.");
        }
    };

    struct load_impl {
        static void invoke(cluster_inpipe_t *inpipe, unserialize_extra_storage_t *es, variant_t *out, int which) {

            /* We decrement "which" as we walk along the type list. That way, the initial value
            of "which" determines which type on the type-list we stop at. */

            if (which == 0) {
                /* We have determined that the value to be deserialized is of type
                head_type_t. */
                typedef typename boost::mpl::front<typelist_t>::type head_type_t;

                head_type_t value;
                unserialize(inpipe, es, &value);
                *out = value;

            } else {
                /* Recurse, popping the head off the type list and decrementing "which" */
                typedef typename boost::mpl::pop_front<typelist_t>::type typelist_tail_t;
                unserialize_nth_from_typelist_t<typelist_tail_t, variant_t>::load(inpipe, es, out, which - 1);
            }
        }
    };

    static void load(cluster_inpipe_t *inpipe, unserialize_extra_storage_t *es, variant_t *out, int which) {
        /* We should never reach the load_null case */
        typedef typename boost::mpl::eval_if<boost::mpl::empty<typelist_t>,
            boost::mpl::identity<load_null>,
            boost::mpl::identity<load_impl>
        >::type typex;
        typex::invoke(inpipe, es, out, which);
    }
};

template<BOOST_VARIANT_ENUM_PARAMS(class T)>
void unserialize(cluster_inpipe_t *inpipe, unserialize_extra_storage_t *es, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> *out) {
    int which;
    ::unserialize(inpipe, es, &which);
    typedef typename boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>::types typelist_t;
    unserialize_nth_from_typelist_t<typelist_t, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >::load(inpipe, es, out, which);
}

#endif /* __RPC_SERIALIZE_VARIANT_HPP__ */

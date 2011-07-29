#ifndef __RIAK_STRUCTURES__
#define __RIAK_STRUCTURES__

#include "riak/riak_value.hpp"
#include "containers/buffer_group.hpp"
#include "containers/scoped_malloc.hpp"
#include "containers/iterators.hpp"
#include "btree/iteration.hpp"

namespace riak {

// Data defintions:
// these definition include everything that will actually need to be serialized for the object
// bucket data definition

struct hook_t {
    enum lang_t {
        JAVASCRIPT,
        ERLANG
    } lang;
    std::string code;
};

struct bucket_t {
    std::string name;
    int n_val;
    bool allow_mult;
    bool last_write_wins;
    std::vector<hook_t> precommit;
    std::vector<hook_t> postcommit;
    int r, w, dw, rw; //Quorum values
    std::string backend;

    bucket_t() 
        : n_val(1), allow_mult(false),
          last_write_wins(false), r(1), w(1), dw(1), 
          rw(1), backend("rethinkdb")
    { }

    bucket_t(std::string name) 
        : name(name), n_val(1), allow_mult(false),
          last_write_wins(false), r(1), w(1), dw(1), 
          rw(1), backend("rethinkdb")
    { }
};


struct link_t {
    std::string bucket;
    std::string key;
    std::string tag;

    std::string as_string();
};
} //namespace riak

// this really sucks, but we can't do a BOOST_FUSION_ADAPT_STRUCT within a
// namespace so we need to break it

BOOST_FUSION_ADAPT_STRUCT(
    riak::link_t,
    (std::string, bucket)
    (std::string, key)
    (std::string, tag)
)

namespace riak {

struct link_filter_t {
    std::string bucket;
    std::string tag;
    bool keep;
};

struct object_t {
    std::string key;
    std::string content;
    std::string content_type;
    std::string meta_data;
    int ETag;
    int last_written;
    std::vector<link_t> links;

    object_t() { }

    object_t(std::string const &key, riak_value_t *val, transaction_t *txn) 
        : key(key)
    {
        last_written = val->mod_time;
        ETag = val->etag;

        blob_t blob(val->contents, blob::btree_maxreflen);
        buffer_group_t buffer_group;
        blob_acq_t acq;
        blob.expose_all(txn, rwi_read, &buffer_group, &acq);

        const_buffer_group_t::iterator it = buffer_group.begin(), end = buffer_group.end(); 

        /* grab the content type */
        content_type.reserve(val->content_type_len);
        for (unsigned i = 0; i < val->content_type_len; i++) {
            content_type += *it;
            it++;
        }

        for (unsigned i = 0; i < val->value_len; i++) {
            content += *it;
            it++;
        }

        //TODO links code goes here, when those are implemented in a sensible way
    }

};

class object_iterator_t : public transform_iterator_t<key_value_pair_t<riak_value_t>, object_t> {
private:
    object_t make_object(key_value_pair_t<riak_value_t> val) {
        return object_t(val.key, reinterpret_cast<riak_value_t *>(val.value.get()), txn.get());
    }

    boost::shared_ptr<transaction_t> txn;
    //boost::shared_ptr<slice_keys_iterator_t<riak_value_t> > it;

public:
    object_iterator_t(slice_keys_iterator_t<riak_value_t> *it, boost::shared_ptr<transaction_t> &txn)
        : transform_iterator_t<key_value_pair_t<riak_value_t>, object_t>(boost::bind(&object_iterator_t::make_object, this, _1), it), 
          txn(txn)
    { }
};

struct object_tree_iterator_t;

struct object_tree_t : public object_t {
    object_tree_iterator_t &children() { crash("Not implemented"); }
    object_tree_iterator_t &children_end() { crash("Not implemented"); }
};

struct object_tree_iterator_t {
    bool operator!=(object_tree_iterator_t const &) {crash("Not implemented");}
    bool operator==(object_tree_iterator_t const &) {crash("Not implemented");}
    object_tree_iterator_t operator++() {crash("Not implemented");}
    object_tree_iterator_t operator++(int) {crash("Not implemented");}
    object_tree_t operator*() {crash("Not implemented");}
    object_tree_t *operator->() {crash("Not implemented");}
};

struct luwak_props_t {
    std::string root_bucket, segment_bucket;
    int block_default;
};

} //namespace riak

#endif

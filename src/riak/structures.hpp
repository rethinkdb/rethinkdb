#ifndef __RIAK_STRUCTURES__
#define __RIAK_STRUCTURES__

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
};

class bucket_iterator_t {
    //TODO actually implement this instead of just duping the type checker
private:
    bucket_t bucket;
public:
    bool operator!=(bucket_iterator_t const &) {not_implemented(); return true;}
    bool operator==(bucket_iterator_t const &) {not_implemented(); return true;}
    bucket_iterator_t operator++() {not_implemented(); return *this;}
    bucket_iterator_t operator++(int) {not_implemented(); return *this;}
    bucket_t operator*() {not_implemented(); return bucket_t(); }
    bucket_t *operator->() {not_implemented(); return &bucket;};
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
    std::string ETag;
    int last_written;
    std::vector<link_t> links;
};


class object_iterator_t {
    //TODO actually implement this instead of just duping the type checker
private:
    object_t object;
public:
    bool operator!=(object_iterator_t const &) {not_implemented(); return true;}
    bool operator==(object_iterator_t const &) {not_implemented(); return true;}
    object_iterator_t operator++() {not_implemented(); return *this;}
    object_iterator_t operator++(int) {not_implemented(); return *this;}
    object_t operator*() {not_implemented(); return object_t(); }
    object_t *operator->() {not_implemented(); return &object;};
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

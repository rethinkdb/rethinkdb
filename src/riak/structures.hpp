#ifndef __RIAK_STRUCTURES__
#define __RIAK_STRUCTURES__

#include "riak/riak_value.hpp"
#include "containers/buffer_group.hpp"
#include "containers/scoped_malloc.hpp"
#include "containers/iterators.hpp"
#include "btree/iteration.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/bind.hpp>

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

    explicit bucket_t(std::string name) 
        : name(name), n_val(1), allow_mult(false),
          last_write_wins(false), r(1), w(1), dw(1), 
          rw(1), backend("rethinkdb")
    { }
};


struct link_t {
    std::string bucket;
    std::string key;
    std::string tag;
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
    boost::optional<std::string> bucket;
    boost::optional<std::string> tag;
    bool keep;
    link_filter_t(std::string bucket, std::string tag, bool keep);
};

bool match(link_filter_t const &, link_t const &);

/* cond specs are used for conditional writes */
struct etag_cond_spec_t {
    enum {
        MATCH,
        NOT_MATCH
    } mode;
    int etag;
};

struct time_cond_spec_t {
    enum {
        MODIFIED_SINCE,
        UNMODIFIED_SINCE
    } mode;
    int time;
};

struct object_t {
    std::string key;
    std::string bucket;
    size_t content_length;
    boost::shared_array<char> content;
    std::string content_type;
    std::string meta_data;
    int ETag;
    int last_written;
    std::vector<link_t> links;
    std::pair<int, int> range;
    size_t total_value_len; //this might not be equal to content.size() if what we have is a range

    object_t() :content_length(0) { }
    void resize_content(size_t);
    void set_content(const char*, size_t);
    const char* data_as_c_str();
    object_t(std::string const &, std::string const &, riak_value_t *, transaction_t *, std::pair<int, int> = std::make_pair(-1, -1));
    size_t on_disk_space_needed_for_links();

};

class object_iterator_t : public transform_iterator_t<key_value_pair_t<riak_value_t>, object_t> {
    object_t object;
public:
    bool operator!=(object_iterator_t const &) {
        not_implemented();
        return true;
    }
    bool operator==(object_iterator_t const &) {
        not_implemented();
        return true;
    }
    object_iterator_t& operator++() {
        not_implemented();
        return *this;
    }
    object_t operator*() {
        not_implemented();
        return object_t();
    }
    object_t *operator->() {
        not_implemented();
        return &object;
    }
};

struct object_tree_iterator_t;

struct object_tree_t : public object_t {
    object_tree_iterator_t &children() { crash("Not implemented"); }
    object_tree_iterator_t &children_end() { crash("Not implemented"); }
};

struct object_tree_iterator_t {
    bool operator!=(object_tree_iterator_t const &) {crash("Not implemented");}
    bool operator==(object_tree_iterator_t const &) {crash("Not implemented");}
    object_tree_iterator_t& operator++() { crash("Not implemented"); }
    object_tree_t operator*() {crash("Not implemented");}
    object_tree_t *operator->() {crash("Not implemented");}
};

/*
//TODO this could actually be a filter iterator
struct link_iterator_t {
    link_iterator_t(object_t const &object, link_filter_t filter)
        : filter(filter), it(object.links.begin()), end(object.links.end())
    { }
    boost::optional<std::pair<std::string, std::string> > next_child() {
        for (; it != end; it++) {
            //check to see if the filter matches the link
            if ((!filter.bucket || filter.bucket == it->bucket) && //make sure the bucket matches
                (!filter.tag    || filter.tag    == it->tag)) {    //make sure the tag matches

                boost::optional<std::pair<std::string, std::string> > res(std::make_pair(it->bucket, it->key));
                it++;
                return res;
            }
        }
        return boost::optional<std::pair<std::string, std::string> >();
    }

private:
    link_filter_t filter;
    std::vector<link_t>::const_iterator it, end;
}; */
struct luwak_props_t {
    std::string root_bucket, segment_bucket;
    int block_default;
};

} //namespace riak

/* For the purposes of clustering many of the riak data structures will need to
 * be serialized. They're all done down here to make them less intrusive */

/* namespace boost {
namespace serialization {

template <class Archive>
void serialize(Archive & ar, riak::link_t & link, const unsigned int) {
    ar & link.bucket;
    ar & link.key;
    ar & link.tag;
}

template <class Archive>
void serialize(Archive & ar, riak::object_t & object, const unsigned int) {
    ar & object.ETag;
    ar & object.bucket;
    ar & object.content;
}

} //namespace serialization
} //namespace boost */

#endif

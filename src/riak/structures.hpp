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


//methods implemented in riak.cc
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
    boost::optional<std::string> bucket;
    boost::optional<std::string> tag;
    bool keep;
};

//TODO TODO TODO make copying this structure more efficient
struct object_t {
    std::string key;
    size_t content_length;
    boost::shared_array<char> content;
    std::string content_type;
    std::string meta_data;
    int ETag;
    int last_written;
    std::vector<link_t> links;
    std::pair<int, int> range;
    size_t total_value_len; //this might not be equal to content.size() if what we have is a range

    object_t() { }

    void resize_content(int n) {
        content_length = n;
        content.reset(new char[n]);
    }

    object_t(std::string const &key, riak_value_t *val, transaction_t *txn, std::pair<int, int> range = std::make_pair(-1, -1))
        : key(key), range(range), total_value_len(val->value_len)
    {
        last_written = val->mod_time;
        ETag = val->etag;

        blob_t blob(val->contents, blob::btree_maxreflen);
        {
            buffer_group_t buffer_group;
            blob_acq_t acq;
            blob.expose_region(txn, rwi_read, 0, val->content_type_len, &buffer_group, &acq);

            const_buffer_group_t::iterator it = buffer_group.begin(), end = buffer_group.end(); 

            /* grab the content type */
            content_type.reserve(val->content_type_len);
            for (unsigned i = 0; i < val->content_type_len; i++) {
                content_type += *it;
                it++;
            }
        }

        {
            buffer_group_t buffer_group;
            blob_acq_t acq;

            if (range.first == -1) {
                //getting the whole value
                rassert(range.second == -1);
                blob.expose_region(txn, rwi_read, val->content_type_len, val->value_len, &buffer_group, &acq);
                const_buffer_group_t::iterator it = buffer_group.begin(), end = buffer_group.end(); 

                resize_content(val->value_len);
                for (char *hd = content.get(); hd - content.get() < val->value_len; hd++) {
                    *hd = *it;
                    it++;
                }
            } else {
                //getting a range
                rassert(0 <= range.first && range.first <= range.second && (unsigned) range.second < val->value_len, "Someone sent us an invalid range, this shouldn't be an assert but should return the error to the user");

                blob.expose_region(txn, rwi_read, val->content_type_len + range.first, range.second - range.first, &buffer_group, &acq);
                const_buffer_group_t::iterator it = buffer_group.begin(), end = buffer_group.end(); 

                resize_content(range.second - range.first);
                for (char *hd = content.get(); hd - content.get() < range.second - range.first; hd++) {
                    *hd = *it;
                    it++;
                }
            }
        }

        if (val->n_links > 0) {
        /* grab the links */
            buffer_group_t buffer_group;
            blob_acq_t acq;

            blob.expose_region(txn, rwi_read, val->content_type_len + val->value_len, val->links_length, &buffer_group, &acq);

            const_buffer_group_t::iterator it = buffer_group.begin(), end = buffer_group.end(); 
            for (int i = 0; i < val->n_links; i++) {
                link_hdr_t link_hdr;
                link_t link;

                link_hdr.bucket_len = *it; it++;
                link_hdr.key_len = *it; it++;
                link_hdr.tag_len = *it; it++;

                link.bucket.reserve(link_hdr.bucket_len);
                for (int j = 0; j < link_hdr.bucket_len; j++) {
                    link.bucket.push_back(*it);
                    it++;
                }


                link.key.reserve(link_hdr.key_len);
                for (int j = 0; j < link_hdr.key_len; j++) {
                    link.key.push_back(*it);
                    it++;
                }

                link.key.reserve(link_hdr.tag_len);
                for (int j = 0; j < link_hdr.tag_len; j++) {
                    link.tag.push_back(*it);
                    it++;
                }

                links.push_back(link);
            }
        }
    }

    size_t on_disk_space_needed_for_links() {
        size_t res = 0;
        for (std::vector<link_t>::const_iterator it = links.begin(); it != links.end(); it++) {
            res += sizeof(link_hdr_t);
            res += it->bucket.size();
            res += it->key.size();
            res += it->tag.size();
        }

        return res;
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

/* 
struct luwak_props_t {
    std::string root_bucket, segment_bucket;
    int block_default;
};
*/

} //namespace riak

#endif

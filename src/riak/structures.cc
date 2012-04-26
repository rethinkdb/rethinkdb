#include "riak/structures.hpp"

#include "riak/riak_value.hpp"

namespace riak {

link_filter_t::link_filter_t(std::string _bucket, std::string _tag, bool keep)
    : keep(keep)
{
    if (_bucket == "_") {
        bucket = boost::optional<std::string>();
    } else {
        bucket = boost::optional<std::string>(_bucket);
    }

    if (_tag == "_") {
        tag = boost::optional<std::string>();
    } else {
        tag = boost::optional<std::string>(_tag);
    }
}

bool match(link_filter_t const &link_filter, link_t const &link) {
    return ((!link_filter.bucket || (link_filter.bucket == link.bucket)) &&
            (!link_filter.tag    || (link_filter.tag    == link.tag)));
}

void object_t::resize_content(size_t n) {
    content_length = n;
    content.reset(new char[n]);
}

void object_t::set_content(const char* c, size_t n) {
    resize_content(n);
    memcpy(content.get(), c, n);
}

const char* object_t::data_as_c_str() {
    content[content_length] = '\0';
    return content.get();
}

object_t::object_t(std::string const &key, std::string const &bucket, riak_value_t *val, transaction_t *txn, std::pair<int, int> range)
    : key(key), bucket(bucket), range(range), total_value_len(val->value_len)
{
    last_written = val->mod_time;
    ETag = val->etag;

    blob_t blob(val->contents, blob::btree_maxreflen);
    {
        buffer_group_t buffer_group;
        blob_acq_t acq;
        blob.expose_region(txn, rwi_read, 0, val->content_type_len, &buffer_group, &acq);

        const_buffer_group_t::iterator it = buffer_group.begin();

        /* grab the content type */
        content_type.reserve(val->content_type_len);
        for (unsigned i = 0; i < val->content_type_len; ++i) {
            content_type += *it;
            ++it;
        }
    }

    {
        buffer_group_t buffer_group;
        blob_acq_t acq;

        if (range.first == -1) {
            //getting the whole value
            rassert(range.second == -1);
            blob.expose_region(txn, rwi_read, val->content_type_len, val->value_len, &buffer_group, &acq);
            const_buffer_group_t::iterator it = buffer_group.begin();

            resize_content(val->value_len);
            for (char *hd = content.get(); hd - content.get() < val->value_len; ++hd) {
                *hd = *it;
                ++it;
            }
        } else {
            //getting a range
            rassert(0 <= range.first && range.first <= range.second && (unsigned) range.second < val->value_len, "Someone sent us an invalid range, this shouldn't be an assert but should return the error to the user");

            blob.expose_region(txn, rwi_read, val->content_type_len + range.first, range.second - range.first, &buffer_group, &acq);
            const_buffer_group_t::iterator it = buffer_group.begin();

            resize_content(range.second - range.first);
            for (char *hd = content.get(); hd - content.get() < range.second - range.first; ++hd) {
                *hd = *it;
                ++it;
            }
        }
    }

    if (val->n_links > 0) {
    /* grab the links */
        buffer_group_t buffer_group;
        blob_acq_t acq;

        blob.expose_region(txn, rwi_read, val->content_type_len + val->value_len, val->links_length, &buffer_group, &acq);

        const_buffer_group_t::iterator it = buffer_group.begin();
        for (int i = 0; i < val->n_links; i++) {
            link_hdr_t link_hdr;
            link_t link;

            link_hdr.bucket_len = uint8_t(*it);
            ++it;
            link_hdr.key_len = uint8_t(*it);
            ++it;
            link_hdr.tag_len = uint8_t(*it);
            ++it;

            link.bucket.reserve(link_hdr.bucket_len);
            for (int j = 0; j < link_hdr.bucket_len; j++) {
                link.bucket.push_back(*it);
                ++it;
            }


            link.key.reserve(link_hdr.key_len);
            for (int j = 0; j < link_hdr.key_len; j++) {
                link.key.push_back(*it);
                ++it;
            }

            link.key.reserve(link_hdr.tag_len);
            for (int j = 0; j < link_hdr.tag_len; j++) {
                link.tag.push_back(*it);
                ++it;
            }

            links.push_back(link);
        }
    }
}

size_t object_t::on_disk_space_needed_for_links() {
    size_t res = 0;
    for (std::vector<link_t>::const_iterator it = links.begin(); it != links.end(); it++) {
        res += sizeof(link_hdr_t);
        res += it->bucket.size();
        res += it->key.size();
        res += it->tag.size();
    }

    return res;
}

} //namespace riak

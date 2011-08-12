#include "riak/structures.hpp"

namespace riak {

link_filter_t::link_filter_t(std::string _bucket, std::string _tag, bool keep) 
    : keep(keep)
{
    if (_bucket == "_") { bucket = boost::optional<std::string>(); }
    else { bucket = boost::optional<std::string>(_bucket); }

    if (_tag == "_") { tag = boost::optional<std::string>(); }
    else { tag = boost::optional<std::string>(_tag); }
}

bool match(link_filter_t const &link_filter, link_t const &link) {
    return ((!link_filter.bucket || (link_filter.bucket == link.bucket)) &&
            (!link_filter.tag    || (link_filter.tag    == link.tag)));
}

} //namespace riak

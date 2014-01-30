// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
namespace ql {

typedef rget_read_response_t::res_t res_t;
typedef rget_read_response_t::res_groups_t res_groups_t;
typedef rget_read_response_t::result_t result_t;
typedef rget_read_response_t::stream_t stream_t;

typedef std::vector<counted_t<const ql::datum_t> > lst_t;
typedef std::map<counted_t<const ql::datum_t>, lst_t> groups_t;

class accumulator_t {
public:
    accumulator_t() : finished(false) { }
    ~accumulator_t() { guarantee(finished); }
    virtual done_t operator()(groups_t *groups,
                              const store_key_t &key,
                              // sindex_val may be NULL
                              const counted_t<const datum_t> &sindex_val) = 0;
    virtual void finish(result_t *out);
    virtual void unshard(const std::vector<res_groups_t *> &rgs);
private:
    virtual void finish_impl(result_t *out);
    bool finished
};

// Make sure to put these right into a scoped pointer.
accumulator_t *make_append(const batcher_t *batcher); // NULL if unsharding
accumulator_t *make_terminal(ql::env_t *env, const rdb_protocol_details::terminal_t &t);

} // namespace ql
#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

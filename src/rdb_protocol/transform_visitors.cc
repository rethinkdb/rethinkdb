// // Copyright 2010-2013 RethinkDB, all rights reserved.
// #include "rdb_protocol/transform_visitors.hpp"

// #include "rdb_protocol/func.hpp"
// #include "rdb_protocol/lazy_json.hpp"

// typedef rdb_protocol_t::rget_read_response_t rget_read_response_t;
// typedef rget_read_response_t::res_t res_t;
// typedef rget_read_response_t::empty_t empty_t;
// typedef rget_read_response_t::grouped_res_t grouped_res_t;
// typedef rget_read_response_t::result_t result_t;

// namespace ql {
// namespace shards {

// class transform_visitor_t : public boost::static_visitor<lst_or_groups_t> {
// public:
//     explicit transform_visitor_t(env_t *_env) : env(_env) { }

//     // These should be rvalue references rather than non-const references, but
//     // Boost didn't like that.
//     lst_or_groups_t operator()(const map_wire_func_t &map, lst_t &l) const;
//     lst_or_groups_t operator()(const filter_transform_t &filter, lst_t &l) const;
//     lst_or_groups_t operator()(const concatmap_wire_func_t &concatmap, lst_t &l) const;

//     // RSI: group
//     // lst_or_groups_t operator()(const group_wire_func_t &concatmap, const lst_t &lst);

//     template<class T>
//     lst_or_groups_t operator()(const T &t, groups_t &groups) const {
//         for (auto it = groups.begin(); it != groups.end(); ++it) {
//             lst_or_groups_t res = (*this)(t, it->second);
//             lst_t *l = boost::get<lst_t>(&res);
//             rcheck_datum(l, ql::base_exc_t::GENERIC,
//                          "Cannot call `group` twice on the same stream.");
//             it->second = std::move(*l);
//         }
//         return lst_or_groups_t(std::move(groups));
//     };
// private:
//     env_t *env;
// };

// lst_or_groups_t transform_visitor_t::operator()(const map_wire_func_t &map,
//                                                 lst_t &lst) const {
//     counted_t<func_t> f = map.compile_wire_func();
//     for (auto it = lst.begin(); it != lst.end(); ++it) {
//         *it = f->call(env, *it)->as_datum();
//     }
//     return lst_or_groups_t(std::move(lst));
// }

// lst_or_groups_t transform_visitor_t::operator()(const filter_transform_t &filter,
//                                                 lst_t &lst) const {
//     std::vector<counted_t<const datum_t> > ret;
//     counted_t<func_t> f = filter.filter_func.compile_wire_func();
//     counted_t<func_t> default_val = filter.default_filter_val
//         ? filter.default_filter_val->compile_wire_func()
//         : counted_t<func_t>();
//     for (auto it = lst.begin(); it != lst.end(); ++it) {
//         if (f->filter_call(env, *it, default_val)) {
//             ret.push_back(std::move(*it));
//         }
//     }
//     return lst_or_groups_t(std::move(ret));
// }

// lst_or_groups_t transform_visitor_t::operator()(const concatmap_wire_func_t &concatmap,
//                                                 lst_t &lst) const {
//     std::vector<counted_t<const datum_t> > ret;
//     auto f = concatmap.compile_wire_func();
//     batchspec_t batchspec = batchspec_t::user(batch_type_t::TERMINAL, env);
//     profile::sampler_t sampler("Evaluating elements in concat map.", env->trace);
//     for (auto it = lst.begin(); it != lst.end(); ++it) {
//         counted_t<datum_stream_t> ds = f->call(env, *it)->as_seq(env);
//         while (auto d = ds->next(env, batchspec)) {
//             ret.push_back(std::move(d));
//             sampler.new_sample();
//         }
//     }
//     return lst_or_groups_t(std::move(ret));
// }

// void transform(env_t *env, const transform_variant_t &t, lst_or_groups_t *target) {
//     *target = boost::apply_visitor(transform_visitor_t(env), t, *target);
// }

// class terminal_visitor_t : public boost::static_visitor<result_t> {
// public:
//     terminal_visitor_t(env_t *_env) : env(_env) { }

//     // These should be rvalue references rather than non-const references, but
//     // Boost didn't like that.
//     result_t operator()(const count_wire_func_t &f, lst_t &l, res_t &r) const;
//     result_t operator()(const reduce_wire_func_t &f, lst_t &l, res_t &r) const;
//     result_t operator()(const gmr_wire_func_t &f, lst_t &l, res_t &r) const;

//     template<class T>
//     result_t operator()(const T &t, groups_t &g, grouped_res_t &gr) {
//         for (auto g_it = g.begin(); g_it != g.end(); ++g_it) {
//             auto gr_it = gr.find(g_it->first);
//             if (gr_it == gr.end()) {
//                 gr_it = gr.insert(std::make_pair(g_it->first, terminal_start(t))).first;
//             }
//             gr_it->second = (*this)(t, g_it->second, gr_it->second);
//         }
//         return std::move(gr);
//     }

//     // All of these cases are errors.
//     template<class T>
//     result_t operator()(const T &, lst_t &, grouped_res_t &) const {
//         r_sanity_check(false);
//     }

//     template<class T>
//     result_t operator()(const T &, groups_t &, res_t &) const {
//         r_sanity_check(false);
//     }

//     template<class T, class U>
//     result_t operator()(const T &, const U &, exc_t &) const {
//         r_sanity_check(false);
//     }

//     template<class T, class U>
//     result_t operator()(const T &, const U &, datum_exc_t &) const {
//         r_sanity_check(false);
//     }
// private:
//     env_t *env;
// };

// result_t terminal_visitor_t::operator()(
//     const count_wire_func_t &, lst_t &l, res_t &r) const {
//     size_t *out = boost::get<size_t>(&r);
//     r_sanity_check(out != NULL);
//     *out = *out + l.size();
//     return std::move(r);
// }

// result_t terminal_visitor_t::operator()(
//     const reduce_wire_func_t &func, lst_t &l, res_t &r) const {
//     counted_t<const ql::datum_t> *d = boost::get<counted_t<const ql::datum_t> >(&r);
//     counted_t<func_t> f = func.compile_wire_func();
//     for (auto it = l.begin(); it != l.end(); ++it) {
//         if (d == NULL) {
//             r_sanity_check(boost::get<empty_t>(&r) != NULL);
//             r = *it;
//         } else {
//             *d = f->call(env, *d, *it)->as_datum();
//         }
//     }
//     return std::move(r);
// }

// result_t terminal_visitor_t::operator()(
//     const gmr_wire_func_t &func, lst_t &l, res_t &r) const {
//     wire_datum_map_t *obj = boost::get<wire_datum_map_t>(&r);
//     r_sanity_check(obj != NULL);
//     counted_t<func_t> f_group = func.compile_group();
//     counted_t<func_t> f_map = func.compile_map();
//     counted_t<func_t> f_reduce = func.compile_reduce();
//     for (auto it = l.begin(); it != l.end(); ++it) {
//         counted_t<const datum_t> key = f_group->call(env, *it)->as_datum();
//         counted_t<const datum_t> val = f_map->call(env, *it)->as_datum();
//         if (!obj->has(key)) {
//             obj->set(std::move(key), std::move(val));
//         } else {
//             counted_t<const datum_t> lhs = obj->get(key);
//             obj->set(key, f_reduce->call(env, lhs, val)->as_datum());
//         }
//     }
//     return std::move(r);
// }

// void terminate(env_t *env, const terminal_variant_t &t, lst_or_groups_t *data,
//                rget_read_response_t::result_t *target) {
//     *target = boost::apply_visitor(terminal_visitor_t(env), t, *data, *target);
// }

// class append_visitor_t : public boost::static_visitor<void> {
// public:
//     append_visitor_t(const store_key_t *_key,
//                      const counted_t<const datum_t> &_sindex_val,
//                      sorting_t _sorting,
//                      batcher_t *_batcher)
//         : key(_key), sindex_val(_sindex_val), sorting(_sorting), batcher(_batcher) { }
//     void operator()(lst_t &l, res_t &r);
//     void operator()(groups_t &l, grouped_res_t &r);

//     // All of these cases are errors.
//     void operator()(lst_t &, grouped_res_t &) { r_sanity_check(false); }
//     void operator()(groups_t &, res_t &) { r_sanity_check(false); }
//     template<class T>
//     void operator()(T &, exc_t &) { r_sanity_check(false); }
//     template<class T>
//     void operator()(T &, datum_exc_t &) { r_sanity_check(false); }

// private:
//     const store_key_t *key;
//     counted_<const datum_t> sindex_val; // may be NULL
//     sorting_t sorting;
//     batcher_t *batcher;
// };

// void append_visitor_t::operator()(lst_t &l, res_t &r) {
//     stream_t *stream = boost::get<stream_t>(&r);
//     r_sanity_check(stream != NULL);
//     for (auto it = l.begin(); it != l.end(); ++it) {
//         batcher->note_el(*it);
//         stream->push_back((sorting != UNORDERED && sindex_val)
//                           ? rget_item_t(*key, sindex_val, *it)
//                           : rget_item_t(*key, *it));
//     }
// }

// void append_visitor_t::operator()(groups_t &g, grouped_res_t &gr) {
//     res_t res;
//     for (auto g_it = g.begin(); g_it != g.end(); ++g_it) {
//         auto gr_it = gr.find(g_it->first);
//         if (gr_it == gr.end()) {
//             gr_it = gr.emplace(it->first, res_t()).first;
//         }
//         (*this)(g_it->second, gr_it->second);
//     }
// }

// void append(const store_key_t &key,
//             counted_t<const datum_t> sindex_val, // may be NULL
//             sorting_t sorting,
//             batcher_t *batcher,
//             lst_or_groups_t *data,
//             rget_read_response_t::result_t *target) {

//     auto stream = boost::get<rget_read_response_t::stream_t>(
// }


// class terminal_start_visitor_t
//     : public boost::static_visitor<res_t> {
// public:
//     terminal_start_visitor_t() { }
//     res_t operator()(const gmr_wire_func_t &f) const {
//         counted_t<func_t> group = f.compile_group();
//         counted_t<func_t> map = f.compile_map();
//         counted_t<func_t> reduce = f.compile_reduce();
//         guarantee(group.has() && map.has() && reduce.has());
//         return wire_datum_map_t();
//     }
//     res_t operator()(const count_wire_func_t &) const {
//         return size_t(0);
//     }
//     res_t operator()(const reduce_wire_func_t &f) const {
//         counted_t<func_t> reduce = f.compile_wire_func();
//         guarantee(reduce.has());
//         return rget_read_response_t::empty_t();
//     }
// };

// const terminal_start_visitor_t terminal_start_visitor;

// res_t terminal_start(const terminal_variant_t &t) {
//     return boost::apply_visitor(terminal_start_visitor, t);
// }

// res_t stream_start() {
//     return rget_read_response_t::stream_t();
// }

// class terminal_exc_visitor_t : public boost::static_visitor<exc_t> {
// public:
//     terminal_exc_visitor_t(const datum_exc_t &_exc) : exc(_exc) { }
//     exc_t operator()(const gmr_wire_func_t &func) const {
//         return exc_t(exc, func.get_bt().get(), 1);
//     }
//     NORETURN exc_t operator()(const count_wire_func_t &) const {
//         r_sanity_check(false);  // Server should never crash here.
//         unreachable();
//     }
//     exc_t operator()(const reduce_wire_func_t &func) const {
//         return exc_t(exc, func.get_bt().get(), 1);
//     }
// private:
//     datum_exc_t exc;
// };

// exc_t terminal_exc(const datum_exc_t &e, const terminal_variant_t &t) {
//     return boost::apply_visitor(terminal_exc_visitor_t(e), t);
// }

// class transform_exc_visitor_t : public boost::static_visitor<exc_t> {
// public:
//     transform_exc_visitor_t(const datum_exc_t &_exc)
//         : exc(_exc) { }
//     exc_t operator()(const map_wire_func_t &func) const {
//         return exc_t(exc, func.get_bt().get(), 1);
//     }
//     exc_t operator()(const filter_transform_t &transf) const {
//         return exc_t(exc, transf.filter_func.get_bt().get(), 1);
//     }
//     exc_t operator()(const concatmap_wire_func_t &func) const {
//         return exc_t(exc, func.get_bt().get(), 1);
//     }
// private:
//     const datum_exc_t exc;
// };

// exc_t transform_exc(const datum_exc_t &e,
//                     const rdb_protocol_details::transform_variant_t &t) {
//     return boost::apply_visitor(transform_exc_visitor_t(e), t);
// }

// class terminal_uses_value_visitor_t : public boost::static_visitor<bool> {
// public:
//     terminal_uses_value_visitor_t() { }
//     bool operator()(const gmr_wire_func_t &) const { return true; }
//     bool operator()(const count_wire_func_t &) const { return false; }
//     bool operator()(const reduce_wire_func_t &) const { return true; }
// };

// const terminal_uses_value_visitor_t terminal_uses_value_visitor;

// bool terminal_uses_value(const terminal_variant_t &t) {
//     return boost::apply_visitor(terminal_uses_value_visitor, t);
// }

// } // namespace shards

// /* A visitor for applying a transformation to a bit of json. */
// class transform_visitor_t : public boost::static_visitor<void> {
// public:
//     transform_visitor_t(counted_t<const datum_t> _arg,
//                         std::vector<counted_t<const datum_t> > *_out,
//                         env_t *_ql_env);

//     void operator()(const map_wire_func_t &func) const;
//     void operator()(const filter_transform_t &func) const;
//     void operator()(const concatmap_wire_func_t &func) const;

// private:
//     counted_t<const datum_t> arg;
//     std::vector<counted_t<const datum_t> > *out;
//     env_t *ql_env;
// };

// transform_visitor_t::transform_visitor_t(
//     counted_t<const datum_t> _arg,
//     std::vector<counted_t<const datum_t> > *_out,
//     env_t *_ql_env)
//     : arg(_arg), out(_out), ql_env(_ql_env) { }

// // All of this logic is analogous to the eager logic in datum_stream.cc.  This
// // code duplication needs to go away, but I'm not 100% sure how to do it (there
// // are sometimes minor differences between the lazy and eager evaluations) and
// // it definitely isn't making it into 1.4.
// void transform_visitor_t::operator()(const map_wire_func_t &func) const {
//     out->push_back(func.compile_wire_func()->call(ql_env, arg)->as_datum());
// }

// void transform_visitor_t::operator()(const concatmap_wire_func_t &func) const {
//     counted_t<datum_stream_t> ds
//         = func.compile_wire_func()->call(ql_env, arg)->as_seq(ql_env);
//     batchspec_t batchspec
//         = batchspec_t::user(batch_type_t::TERMINAL, ql_env);
//     {
//         profile::sampler_t sampler("Evaluating elements in concat map.", ql_env->trace);
//         while (counted_t<const datum_t> d = ds->next(ql_env, batchspec)) {
//             out->push_back(d);
//             sampler.new_sample();
//         }
//     }
// }

// void transform_visitor_t::operator()(const filter_transform_t &transf) const {
//     counted_t<func_t> f = transf.filter_func.compile_wire_func();
//     counted_t<func_t> default_filter_val = transf.default_filter_val ?
//         transf.default_filter_val->compile_wire_func() :
//         counted_t<func_t>();
//     if (f->filter_call(ql_env, arg, default_filter_val)) {
//         out->push_back(arg);
//     }
// }

// void transform_apply(env_t *ql_env,
//                      counted_t<const datum_t> json,
//                      const rdb_protocol_details::transform_variant_t *t,
//                      std::vector<counted_t<const datum_t> > *out) {
//     boost::apply_visitor(transform_visitor_t(json, out, ql_env), *t);
// }

// /* A visitor for applying a terminal to a bit of json. */
// class terminal_visitor_t : public boost::static_visitor<void> {
// public:
//     terminal_visitor_t(lazy_json_t _json,
//                        env_t *_ql_env,
//                        rget_read_response_t::result_t *_out);

//     void operator()(const count_wire_func_t &) const;
//     void operator()(const gmr_wire_func_t &) const;
//     void operator()(const reduce_wire_func_t &) const;
// private:
//     lazy_json_t json;
//     env_t *ql_env;
//     rget_read_response_t::result_t *out;
// };

// terminal_visitor_t::terminal_visitor_t(lazy_json_t _json,
//                                        env_t *_ql_env,
//                                        rget_read_response_t::result_t *_out)
//     : json(_json), ql_env(_ql_env), out(_out) { }

// // All of this logic is analogous to the eager logic in datum_stream.cc.  This
// // code duplication needs to go away, but I'm not 100% sure how to do it (there
// // are sometimes minor differences between the lazy and eager evaluations) and
// // it definitely isn't making it into 1.4.
// void terminal_visitor_t::operator()(const gmr_wire_func_t &func) const {
//     wire_datum_map_t *obj = boost::get<wire_datum_map_t>(out);
//     guarantee(obj);

//     counted_t<const datum_t> el = json.get();
//     counted_t<const datum_t> el_group
//         = func.compile_group()->call(ql_env, el)->as_datum();
//     counted_t<const datum_t> elm = json.get();
//     counted_t<const datum_t> el_map
//         = func.compile_map()->call(ql_env, elm)->as_datum();

//     if (!obj->has(el_group)) {
//         obj->set(el_group, el_map);
//     } else {
//         counted_t<const datum_t> lhs = obj->get(el_group);
//         obj->set(el_group, func.compile_reduce()->call(ql_env, lhs, el_map)->as_datum());
//     }
// }

// void terminal_visitor_t::operator()(UNUSED const count_wire_func_t &func) const {
//     counted_t<const datum_t> d = boost::get<counted_t<const datum_t> >(*out);
//     *out = make_counted<const datum_t>(d->as_int() + 1.0);
// }

// void terminal_visitor_t::operator()(const reduce_wire_func_t &func) const {
//     counted_t<const datum_t> *d = boost::get<counted_t<const datum_t> >(out);
//     counted_t<const datum_t> rhs = json.get();
//     if (d != NULL) {
//         *out = func.compile_wire_func()->call(ql_env, *d, rhs)->as_datum();
//     } else {
//         guarantee(boost::get<rget_read_response_t::empty_t>(out));
//         *out = rhs;
//     }
// }

// void terminal_apply(env_t *ql_env,
//                     lazy_json_t json,
//                     const rdb_protocol_details::terminal_variant_t *t,
//                     rget_read_response_t::result_t *out) {
//     boost::apply_visitor(terminal_visitor_t(json, ql_env, out), *t);
// }


// /* A visitor for setting the result type based on a terminal. */
// class terminal_initializer_visitor_t : public boost::static_visitor<void> {
// public:
//     explicit terminal_initializer_visitor_t(rget_read_response_t::result_t *_out)
//         : out(_out) { }

//     void operator()(const gmr_wire_func_t &f) const {
//         counted_t<func_t> group = f.compile_group();
//         counted_t<func_t> map = f.compile_map();
//         counted_t<func_t> reduce = f.compile_reduce();
//         guarantee(group.has() && map.has() && reduce.has());
//         *out = wire_datum_map_t();
//     }

//     void operator()(const count_wire_func_t &) const {
//         *out = make_counted<const datum_t>(0.0);
//     }

//     void operator()(const reduce_wire_func_t &f) const {
//         counted_t<func_t> reduce = f.compile_wire_func();
//         guarantee(reduce.has());
//         *out = rget_read_response_t::empty_t();
//     }

// private:
//     rget_read_response_t::result_t *out;
// };

// void terminal_initialize(const rdb_protocol_details::terminal_variant_t *t,
//                          rget_read_response_t::result_t *out) {
//     boost::apply_visitor(terminal_initializer_visitor_t(out), *t);
// }



// } // namespace ql

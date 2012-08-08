#ifndef HTTP_JSON_JSON_ADAPTER_TCC_
#define HTTP_JSON_JSON_ADAPTER_TCC_

#include "http/json/json_adapter.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include "containers/uuid.hpp"
#include "http/json.hpp"
#include "logger.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"

//implementation for standard_subfield_functor_t
template<class T>
standard_subfield_change_functor_t<T>::standard_subfield_change_functor_t(T *_target)
    : target(_target) { }

template<class T>
void standard_subfield_change_functor_t<T>::on_change() {
    on_subfield_change(target);
}

//implementation for standard_ctx_subfield_functor_t
template<class T, class ctx_t>
standard_ctx_subfield_change_functor_t<T, ctx_t>::standard_ctx_subfield_change_functor_t(T *_target, const ctx_t &_ctx)
    : target(_target), ctx(_ctx) { }

template<class T, class ctx_t>
void standard_ctx_subfield_change_functor_t<T, ctx_t>::on_change() {
    with_ctx_on_subfield_change(target, ctx);
}

//implementation for json_adapter_t
template <class T>
json_adapter_t<T>::json_adapter_t(T *target) : target_(target) { }

template <class T>
cJSON *json_adapter_t<T>::render_impl() { return render_as_json(target_); }

template <class T>
void json_adapter_t<T>::apply_impl(cJSON *change) { apply_json_to(change, target_); }

template <class T>
void json_adapter_t<T>::erase_impl() { erase_json(target_); }

template <class T>
void json_adapter_t<T>::reset_impl() { reset_json(target_); }


template <class T>
json_adapter_if_t::json_adapter_map_t json_adapter_t<T>::get_subfields_impl() {
    return get_json_subfields(target_);
}

template <class T>
boost::shared_ptr<subfield_change_functor_t> json_adapter_t<T>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_subfield_change_functor_t<T>(target_));
}

//implementation for json_ctx_adapter_t
template <class T, class ctx_t>
json_ctx_adapter_t<T, ctx_t>::json_ctx_adapter_t(T *target, const ctx_t &ctx)
    : target_(target), ctx_(ctx) { }

template <class T, class ctx_t>
cJSON *json_ctx_adapter_t<T, ctx_t>::render_impl() {
    return with_ctx_render_as_json(target_, ctx_);
}

template <class T, class ctx_t>
void json_ctx_adapter_t<T, ctx_t>::apply_impl(cJSON *change) {
    with_ctx_apply_json_to(change, target_, ctx_);
}

template <class T, class ctx_t>
void json_ctx_adapter_t<T, ctx_t>::erase_impl() {
    with_ctx_erase_json(target_, ctx_);
}

template <class T, class ctx_t>
void json_ctx_adapter_t<T, ctx_t>::reset_impl() {
    with_ctx_reset_json(target_, ctx_);
}


template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t json_ctx_adapter_t<T, ctx_t>::get_subfields_impl() {
    return with_ctx_get_json_subfields(target_, ctx_);
}

template <class T, class ctx_t>
boost::shared_ptr<subfield_change_functor_t> json_ctx_adapter_t<T, ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_ctx_subfield_change_functor_t<T, ctx_t>(target_, ctx_));
}


//implementation for json_read_only_adapter_t
template <class T>
json_read_only_adapter_t<T>::json_read_only_adapter_t(T *t)
    : json_adapter_t<T>(t) { }

template <class T>
void json_read_only_adapter_t<T>::apply_impl(cJSON *) {
    throw permission_denied_exc_t("Trying to write to a readonly value");
}

template <class T>
void json_read_only_adapter_t<T>::erase_impl() {
    throw permission_denied_exc_t("Trying to erase a readonly value");
}

template <class T>
void json_read_only_adapter_t<T>::reset_impl() {
    throw permission_denied_exc_t("Trying to reset a readonly value");
}

//implementation for json_ctx_read_only_adapter_t
template <class T, class ctx_t>
json_ctx_read_only_adapter_t<T, ctx_t>::json_ctx_read_only_adapter_t(T *t, const ctx_t &ctx)
    : json_ctx_adapter_t<T, ctx_t>(t, ctx)
{ }

template <class T, class ctx_t>
void json_ctx_read_only_adapter_t<T, ctx_t>::apply_impl(cJSON *) {
    throw permission_denied_exc_t("Trying to write to a readonly value");
}

template <class T, class ctx_t>
void json_ctx_read_only_adapter_t<T, ctx_t>::erase_impl() {
    throw permission_denied_exc_t("Trying to erase a readonly value");
}

template <class T, class ctx_t>
void json_ctx_read_only_adapter_t<T, ctx_t>::reset_impl() {
    throw permission_denied_exc_t("Trying to reset a readonly value");
}

//implementation for json_temporary_adapter_t
template <class T>
json_temporary_adapter_t<T>::json_temporary_adapter_t(const T &value)
    : json_read_only_adapter_t<T>(&value_), value_(value)
{ }

//implementation for map_inserter_t
template <class container_t>
json_map_inserter_t<container_t>::json_map_inserter_t(container_t *_target, gen_function_t _generator, value_t _initial_value)
    : target(_target), generator(_generator), initial_value(_initial_value) { }

template <class container_t>
cJSON *json_map_inserter_t<container_t>::render_impl() {
    /* This is perhaps a bit confusing, json_map_inserters will generally
     * render as nothing (an empty object) unless they have been used to insert
     * something, this is kind of a weird thing so that when someone does a
     * post to this value they get a view of the things we inserted. */
    cJSON *res = cJSON_CreateObject();
    for (typename keys_set_t::iterator it = added_keys.begin(); it != added_keys.end(); ++it) {
        scoped_cJSON_t key(render_as_json(&*it));
        cJSON_AddItemToObject(res, get_string(key.get()).c_str(), render_as_json(&(target->find(*it)->second)));
    }
    return res;
}

template <class container_t>
void json_map_inserter_t<container_t>::apply_impl(cJSON *change) {
    typename container_t::key_type key = generator();
    added_keys.insert(key);

    typename container_t::mapped_type val = initial_value;
    apply_json_to(change, &val);

    target->insert(typename container_t::value_type(key, val));
}

template <class container_t>
void json_map_inserter_t<container_t>::erase_impl() {
    throw permission_denied_exc_t("Trying to erase a value that can't be erase.");
}

template <class container_t>
void json_map_inserter_t<container_t>::reset_impl() {
    throw permission_denied_exc_t("Trying to reset a value that can't be reset.");
}

template <class container_t>
json_adapter_if_t::json_adapter_map_t json_map_inserter_t<container_t>::get_subfields_impl() {
    json_adapter_map_t res;
    for (typename keys_set_t::iterator it = added_keys.begin(); it != added_keys.end(); ++it) {
        scoped_cJSON_t key(render_as_json(&*it));
        res[get_string(key.get())] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<typename container_t::mapped_type>(&(target->find(*it)->second)));
    }
    return res;
}

template <class container_t>
boost::shared_ptr<subfield_change_functor_t> json_map_inserter_t<container_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_subfield_change_functor_t<container_t>(target));
}

//implementation for json_ctx_map_inserter_t
template <class container_t, class ctx_t>
json_ctx_map_inserter_t<container_t, ctx_t>::json_ctx_map_inserter_t(container_t *_target, gen_function_t _generator, const ctx_t &_ctx, value_t _initial_value)
    : target(_target), generator(_generator), initial_value(_initial_value), ctx(_ctx)
{ }

template <class container_t, class ctx_t>
cJSON *json_ctx_map_inserter_t<container_t, ctx_t>::render_impl() {
    /* This is perhaps a bit confusing, json_map_inserters will generally
     * render as nothing (an empty object) unless they have been used to insert
     * something, this is kind of a weird thing so that when someone does a
     * post to this value they get a view of the things we inserted. */
    cJSON *res = cJSON_CreateObject();
    for (typename keys_set_t::iterator it = added_keys.begin(); it != added_keys.end(); ++it) {
        scoped_cJSON_t key(render_as_json(&*it, ctx));
        cJSON_AddItemToObject(res, get_string(key.get()).c_str(), render_as_json(&(target->find(*it)->second), ctx));
    }
    return res;
}

template <class container_t, class ctx_t>
void json_ctx_map_inserter_t<container_t, ctx_t>::apply_impl(cJSON *change) {
    typename container_t::key_type key = generator();
    added_keys.insert(key);

    typename container_t::mapped_type val = initial_value;
    apply_json_to(change, &val, ctx);

    target->insert(typename container_t::value_type(key, val));
}

template <class container_t, class ctx_t>
void json_ctx_map_inserter_t<container_t, ctx_t>::erase_impl() {
    throw permission_denied_exc_t("Trying to erase a value that can't be erase.");
}

template <class container_t, class ctx_t>
void json_ctx_map_inserter_t<container_t, ctx_t>::reset_impl() {
    throw permission_denied_exc_t("Trying to reset a value that can't be reset.");
}

template <class container_t, class ctx_t>
json_adapter_if_t::json_adapter_map_t json_ctx_map_inserter_t<container_t, ctx_t>::get_subfields_impl() {
    json_adapter_map_t res;
    for (typename keys_set_t::iterator it = added_keys.begin(); it != added_keys.end(); ++it) {
        scoped_cJSON_t key(render_as_json(&*it, ctx));
        res[get_string(key.get())] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<typename container_t::mapped_type, ctx_t>(&(target->find(*it)->second), ctx));
    }
    return res;
}

template <class container_t, class ctx_t>
boost::shared_ptr<subfield_change_functor_t> json_ctx_map_inserter_t<container_t, ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_ctx_subfield_change_functor_t<container_t, ctx_t>(target, ctx));
}


//implementation for json_adapter_with_inserter_t
template <class container_t>
json_adapter_with_inserter_t<container_t>::json_adapter_with_inserter_t(container_t *_target, gen_function_t _generator, value_t _initial_value, std::string _inserter_key)
    : target(_target), generator(_generator),
      initial_value(_initial_value), inserter_key(_inserter_key) { }

template <class container_t>
cJSON *json_adapter_with_inserter_t<container_t>::render_impl() {
    return render_as_json(target);
}

template <class container_t>
void json_adapter_with_inserter_t<container_t>::apply_impl(cJSON *change) {
    apply_json_to(change, target);
}

template <class container_t>
void json_adapter_with_inserter_t<container_t>::erase_impl() {
    erase_json(target);
}

template <class container_t>
void json_adapter_with_inserter_t<container_t>::reset_impl() {
    reset_json(target);
}

template <class container_t>
json_adapter_if_t::json_adapter_map_t json_adapter_with_inserter_t<container_t>::get_subfields_impl() {
    json_adapter_map_t res = get_json_subfields(target);
    rassert(res.find(inserter_key) == res.end(), "Error, inserter_key %s  conflicts with another field of the target, (you probably want to change the value of inserter_key).", inserter_key.c_str());
    res[inserter_key] = boost::shared_ptr<json_adapter_if_t>(new json_map_inserter_t<container_t>(target, generator, initial_value));

    return res;
}

template <class container_t>
boost::shared_ptr<subfield_change_functor_t> json_adapter_with_inserter_t<container_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_subfield_change_functor_t<container_t>(target));
}

//implementation for json_ctx_adapter_with_inserter_t
template <class container_t, class ctx_t>
json_ctx_adapter_with_inserter_t<container_t, ctx_t>::json_ctx_adapter_with_inserter_t(container_t *_target, gen_function_t _generator, const ctx_t &_ctx, value_t _initial_value, std::string _inserter_key)
    : target(_target), generator(_generator),
      initial_value(_initial_value), inserter_key(_inserter_key),
      ctx(_ctx)
{ }

template <class container_t, class ctx_t>
cJSON *json_ctx_adapter_with_inserter_t<container_t, ctx_t>::render_impl() {
    return render_as_json(target, ctx);
}

template <class container_t, class ctx_t>
void json_ctx_adapter_with_inserter_t<container_t, ctx_t>::apply_impl(cJSON *change) {
    apply_json_to(change, target, ctx);
}

template <class container_t, class ctx_t>
void json_ctx_adapter_with_inserter_t<container_t, ctx_t>::erase_impl() {
    erase_json(target, ctx);
}

template <class container_t, class ctx_t>
void json_ctx_adapter_with_inserter_t<container_t, ctx_t>::reset_impl() {
    reset_json(target, ctx);
}

template <class container_t, class ctx_t>
json_adapter_if_t::json_adapter_map_t json_ctx_adapter_with_inserter_t<container_t, ctx_t>::get_subfields_impl() {
    json_adapter_map_t res = get_json_subfields(target, ctx);
    rassert(res.find(inserter_key) == res.end(), "Error, inserter_key %s  conflicts with another field of the target, (you probably want to change the value of inserter_key).", inserter_key.c_str());
    res[inserter_key] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_map_inserter_t<container_t, ctx_t>(target, generator, ctx, initial_value));

    return res;
}

template <class container_t, class ctx_t>
boost::shared_ptr<subfield_change_functor_t> json_ctx_adapter_with_inserter_t<container_t, ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_ctx_subfield_change_functor_t<container_t, ctx_t>(target, ctx));
}


/* Here we have implementations of the json adapter concept for several
 * prominent types, these could in theory be relocated to a different file if
 * need be */

//JSON adapter for int
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(int *, const ctx_t &) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(int *target, const ctx_t &) {
    return cJSON_CreateNumber(*target);
}

template <class ctx_t>
void apply_json_to(cJSON *change, int *target, const ctx_t &) {
    *target = get_int(change);
}

template <class ctx_t>
void on_subfield_change(int *, const ctx_t &) { }

//JSON adapter for char
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(char *, const ctx_t &) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(char *target, const ctx_t &) {
    return cJSON_CreateString(target);
}

template <class ctx_t>
void apply_json_to(cJSON *change, char *target, const ctx_t &) {
    std::string str = cJSON_print_unformatted_std_string(change);
    if (str.size() != 1) {
        throw schema_mismatch_exc_t(strprintf("Trying to write %s to a char."
                                    "The change should only be one character long.", str.c_str()));
    } else {
        *target = str[0];
    }
}

//JSON adapter for uuid_t
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(uuid_t *target, const ctx_t &) {
    return get_json_subfields(target);
}

template <class ctx_t>
cJSON *render_as_json(const uuid_t *target, const ctx_t &) {
    return render_as_json(target);
}

template <class ctx_t>
void apply_json_to(cJSON *change, uuid_t *target, const ctx_t &) {
    apply_json_to(change, target);
}

template <class ctx_t>
void on_subfield_change(uuid_t *target, const ctx_t &) { on_subfield_change(target); }

namespace boost {

//JSON adapter for boost::optional
template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(boost::optional<T> *target, const ctx_t &ctx) {
    if (*target) {
        return get_json_subfields(&**target, ctx);
    } else {
        return json_adapter_if_t::json_adapter_map_t();
    }
}

template <class T, class ctx_t>
cJSON *render_as_json(boost::optional<T> *target, const ctx_t &ctx) {
    if (*target) {
        return render_as_json(&**target, ctx);
    } else {
        // TODO: This is obviously broken?
        return cJSON_CreateString("Unset value");
    }
}

template <class T, class ctx_t>
void apply_json_to(cJSON *change, boost::optional<T> *target, const ctx_t &ctx) {
    if (!*target) {
        *target = T();
    }
    apply_json_to(change, &**target, ctx);
}

template <class T, class ctx_t>
void on_subfield_change(boost::optional<T> *, const ctx_t &) { }

//JSON adapter for boost::variant
template <class ctx_t>
class variant_json_subfield_getter_t : public boost::static_visitor<json_adapter_if_t::json_adapter_map_t> {
public:
    explicit variant_json_subfield_getter_t(ctx_t _ctx)
        : ctx(_ctx)
    { }

    template <class T>
    json_adapter_if_t::json_adapter_map_t operator()(const T &t) {
        T _t = t;
        return get_json_subfields(&_t, ctx);
    }
private:
    ctx_t ctx;
};

template <class ctx_t>
class variant_json_renderer_t : public boost::static_visitor<cJSON *> {
public:
    explicit variant_json_renderer_t(ctx_t _ctx)
        : ctx(_ctx)
    { }

    template <class T>
    cJSON *operator()(const T &t) {
        T _t = t;
        return render_as_json(&_t, ctx);
    }
private:
    ctx_t ctx;
};

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *target, const ctx_t &ctx) {
    variant_json_subfield_getter_t<ctx_t> visitor(ctx);
    return boost::apply_visitor(visitor, *target);
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
cJSON *render_as_json(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *target, const ctx_t &ctx) {
    variant_json_renderer_t<ctx_t> visitor(ctx);
    return boost::apply_visitor(visitor, *target);
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
void apply_json_to(cJSON *, boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *, const ctx_t &) {
    throw permission_denied_exc_t("Can't write to a boost::variant.");
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20, class ctx_t>
void on_subfield_change(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *, const ctx_t &) { }


// ctx-less JSON adapter for boost::variant
class noctx_variant_json_subfield_getter_t : public boost::static_visitor<json_adapter_if_t::json_adapter_map_t> {
public:
    noctx_variant_json_subfield_getter_t() { }

    template <class T>
    json_adapter_if_t::json_adapter_map_t operator()(const T &t) {
        T _t = t;
        return get_json_subfields(&_t);
    }
};

class noctx_variant_json_renderer_t : public boost::static_visitor<cJSON *> {
public:
    noctx_variant_json_renderer_t() { }

    template <class T>
    cJSON *operator()(const T &t) {
        T _t = t;
        return render_as_json(&_t);
    }
};

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
json_adapter_if_t::json_adapter_map_t get_json_subfields(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *target) {
    noctx_variant_json_subfield_getter_t visitor;
    return boost::apply_visitor(visitor, *target);
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
cJSON *render_as_json(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *target) {
    noctx_variant_json_renderer_t visitor;
    return boost::apply_visitor(visitor, *target);
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
void apply_json_to(cJSON *, boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *) {
    throw permission_denied_exc_t("Can't write to a boost::variant.");
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
void on_subfield_change(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *) { }



} //namespace boost

namespace std {
//JSON adapter for std::string
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::string *target, const ctx_t &) {
    return get_json_subfields(target);
}

template <class ctx_t>
cJSON *render_as_json(std::string *target, const ctx_t &) {
    return render_as_json(target);
}

template <class ctx_t>
void apply_json_to(cJSON *change, std::string *target, const ctx_t &) {
    apply_json_to(change, target);
}

template <class ctx_t>
void on_subfield_change(std::string *target, const ctx_t &) { on_subfield_change(target); }


//JSON adapter for std::map

// TODO: User-specified data shouldn't produce fields.  A std::map
// should be serialized as an array of key, value pairs.  No?

template <class K, class V, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::map<K, V> *map, const ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;

#ifdef JSON_SHORTCUTS
    int shortcut_index = 0;
#endif

    for (typename std::map<K, V>::iterator it = map->begin(); it != map->end(); ++it) {
        typename std::map<K, V>::key_type key = it->first;
        try {
            scoped_cJSON_t scoped_key(render_as_json(&key, ctx));
            res[get_string(scoped_key.get())]
                = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<V, ctx_t>(&it->second, ctx));
        } catch (schema_mismatch_exc_t &) {
            crash("Someone tried to json adapt a std::map with a key type that"
                   "does not yield a JSON object of string type when"
                   "render_as_json is applied to it.");
        }

#ifdef JSON_SHORTCUTS
        res[strprintf("%d", shortcut_index)] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<V, ctx_t>(&(it->second)));
        shortcut_index++;
#endif
    }

    return res;
}

template <class K, class V, class ctx_t>
cJSON *render_as_json(std::map<K, V> *map, const ctx_t &ctx) {
    return render_as_directory(map, ctx);
}

template <class K, class V, class ctx_t>
void apply_json_to(cJSON *change, std::map<K, V> *map, const ctx_t &ctx) {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;

    json_adapter_map_t elements = get_json_subfields(map, ctx);

    json_object_iterator_t it = get_object_it(change);

    cJSON *val;
    while ((val = it.next())) {
        if (std_contains(elements, val->string)) {
            elements[val->string]->apply(val);
        } else {
            K k;
            scoped_cJSON_t key(cJSON_CreateString(val->string));
            apply_json_to(key.get(), &k, ctx);

            V v;
            apply_json_to(val, &v, ctx);

            (*map)[k] = v;
        }
    }
}

template <class K, class V, class ctx_t>
void on_subfield_change(std::map<K, V> *, const ctx_t &) { }

// ctx-less JSON adapter for std::map

// TODO: User-specified data shouldn't produce fields.  A std::map
// should be serialized as an array of key, value pairs.  No?

template <class K, class V>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::map<K, V> *map) {
    json_adapter_if_t::json_adapter_map_t res;

#ifdef JSON_SHORTCUTS
    int shortcut_index = 0;
#endif

    for (typename std::map<K, V>::iterator it = map->begin(); it != map->end(); ++it) {
        typename std::map<K, V>::key_type key = it->first;
        try {
            scoped_cJSON_t scoped_key(render_as_json(&key));
            res[get_string(scoped_key.get())]
                = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<V>(&it->second));
        } catch (schema_mismatch_exc_t &) {
            crash("Someone tried to json adapt a std::map with a key type that"
                   "does not yield a JSON object of string type when"
                   "render_as_json is applied to it.");
        }

#ifdef JSON_SHORTCUTS
        res[strprintf("%d", shortcut_index)] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<V>(&(it->second)));
        shortcut_index++;
#endif
    }

    return res;
}

template <class K, class V>
cJSON *render_as_json(std::map<K, V> *map) {
    return render_as_directory(map);
}

template <class K, class V>
void apply_json_to(cJSON *change, std::map<K, V> *map) {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;

    json_adapter_map_t elements = get_json_subfields(map);

    json_object_iterator_t it = get_object_it(change);

    cJSON *val;
    while ((val = it.next())) {
        if (std_contains(elements, val->string)) {
            elements[val->string]->apply(val);
        } else {
            K k;
            scoped_cJSON_t key(cJSON_CreateString(val->string));
            apply_json_to(key.get(), &k);

            V v;
            apply_json_to(val, &v);

            (*map)[k] = v;
        }
    }
}

template <class K, class V>
void on_subfield_change(std::map<K, V> *) { }


//JSON adapter for std::set
template <class V, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::set<V> *, const ctx_t &) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class V, class ctx_t>
cJSON *render_as_json(std::set<V> *target, const ctx_t &ctx) {
    cJSON *res = cJSON_CreateArray();

    for (typename std::set<V>::const_iterator it = target->begin(); it != target->end(); ++it) {
        V tmp = *it;
        cJSON_AddItemToArray(res, render_as_json(&tmp, ctx));
    }
    return res;
}

template <class V, class ctx_t>
void apply_json_to(cJSON *change, std::set<V> *target, const ctx_t &ctx) {
    std::set<V> res;
    json_array_iterator_t it = get_array_it(change);
    cJSON *val;
    while ((val = it.next())) {
        V v;
        apply_json_to(val, &v, ctx);
        res.insert(v);
    }

    *target = res;
}

template <class V, class ctx_t>
void on_subfield_change(std::set<V> *, const ctx_t &) { }

// ctx-less JSON adapter for std::set
template <class V>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::set<V> *) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class V>
cJSON *render_as_json(std::set<V> *target) {
    cJSON *res = cJSON_CreateArray();

    for (typename std::set<V>::const_iterator it = target->begin(); it != target->end(); ++it) {
        V tmp = *it;
        cJSON_AddItemToArray(res, render_as_json(&tmp));
    }
    return res;
}

template <class V>
void apply_json_to(cJSON *change, std::set<V> *target) {
    std::set<V> res;
    json_array_iterator_t it = get_array_it(change);
    cJSON *val;
    while ((val = it.next())) {
        V v;
        apply_json_to(val, &v);
        res.insert(v);
    }

    *target = res;
}

template <class V>
void on_subfield_change(std::set<V> *) { }


//JSON adapter for std::pair
template <class F, class S, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::pair<F, S> *target, const ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    res["first"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<F, ctx_t>(&target->first, ctx));
    res["second"] = boost::shared_ptr<json_adapter_if_t>(new json_ctx_adapter_t<S, ctx_t>(&target->second, ctx));
    return res;
}

template <class F, class S, class ctx_t>
cJSON *render_as_json(std::pair<F, S> *target, const ctx_t &ctx) {
    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, render_as_json(&target->first, ctx));
    cJSON_AddItemToArray(res, render_as_json(&target->second, ctx));
    return res;
}

template <class F, class S, class ctx_t>
void apply_json_to(cJSON *change, std::pair<F, S> *target, const ctx_t &ctx) {
    json_array_iterator_t it = get_array_it(change);
    cJSON *first = it.next(), *second = it.next();
    if (!first || !second || it.next()) {
        throw schema_mismatch_exc_t("Expected an array with exactly 2 elements in it");
    }
    apply_json_to(first, &target->first, ctx);
    apply_json_to(second, &target->second, ctx);
}

template <class F, class S, class ctx_t>
void on_subfield_change(std::pair<F, S> *, const ctx_t &) { }

// ctx-less JSON adapter for std::pair
template <class F, class S>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::pair<F, S> *target) {
    json_adapter_if_t::json_adapter_map_t res;
    res["first"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<F>(&target->first));
    res["second"] = boost::shared_ptr<json_adapter_if_t>(new json_adapter_t<S>(&target->second));
    return res;
}

template <class F, class S>
cJSON *render_as_json(std::pair<F, S> *target) {
    cJSON *res = cJSON_CreateArray();
    cJSON_AddItemToArray(res, render_as_json(&target->first));
    cJSON_AddItemToArray(res, render_as_json(&target->second));
    return res;
}

template <class F, class S>
void apply_json_to(cJSON *change, std::pair<F, S> *target) {
    json_array_iterator_t it = get_array_it(change);
    cJSON *first = it.next(), *second = it.next();
    if (!first || !second || it.next()) {
        throw schema_mismatch_exc_t("Expected an array with exactly 2 elements in it");
    }
    apply_json_to(first, &target->first);
    apply_json_to(second, &target->second);
}

template <class F, class S>
void on_subfield_change(std::pair<F, S> *) { }


//JSON adapter for std::vector
template <class V, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::vector<V> *, const ctx_t &) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class V, class ctx_t>
cJSON *render_as_json(std::vector<V> *target, const ctx_t &ctx) {
    cJSON *res = cJSON_CreateArray();
    for (typename std::vector<V>::iterator it = target->begin(); it != target->end(); ++it) {
        cJSON_AddItemToArray(res, render_as_json(&*it, ctx));
    }

    return res;
}

template <class V, class ctx_t>
void apply_json_to(cJSON *change, std::vector<V> *target, const ctx_t &ctx) {
    std::vector<V> val;
    json_array_iterator_t it = get_array_it(change);
    cJSON *hd;
    while ((hd = it.next())) {
        V v;
        apply_json_to(hd, &v, ctx);
        val.push_back(v);
    }

    *target = val;
}

template <class V, class ctx_t>
void on_subfield_change(std::vector<V> *, const ctx_t &) { }

// ctx-less JSON adapter for std::vector
template <class V>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::vector<V> *) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class V>
cJSON *render_as_json(std::vector<V> *target) {
    cJSON *res = cJSON_CreateArray();
    for (typename std::vector<V>::iterator it = target->begin(); it != target->end(); ++it) {
        cJSON_AddItemToArray(res, render_as_json(&*it));
    }

    return res;
}

template <class V>
void apply_json_to(cJSON *change, std::vector<V> *target) {
    std::vector<V> val;
    json_array_iterator_t it = get_array_it(change);
    cJSON *hd;
    while ((hd = it.next())) {
        V v;
        apply_json_to(hd, &v);
        val.push_back(v);
    }

    *target = val;
}

template <class V>
void on_subfield_change(std::vector<V> *) { }


} //namespace std

//some convenience functions
template <class T, class ctx_t>
cJSON *render_as_directory(T *target, const ctx_t &ctx) {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;

    cJSON *res = cJSON_CreateObject();

    json_adapter_map_t elements = get_json_subfields(target, ctx);
    for (json_adapter_map_t::iterator it = elements.begin(); it != elements.end(); ++it) {
        cJSON_AddItemToObject(res, it->first.c_str(), it->second->render());
    }

    return res;
}

template <class T>
cJSON *render_as_directory(T *target) {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;

    cJSON *res = cJSON_CreateObject();

    json_adapter_map_t elements = get_json_subfields(target);
    for (json_adapter_map_t::iterator it = elements.begin(); it != elements.end(); ++it) {
        cJSON_AddItemToObject(res, it->first.c_str(), it->second->render());
    }

    return res;
}


template <class T, class ctx_t>
void apply_as_directory(cJSON *change, T *target, const ctx_t &ctx) {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
    json_adapter_map_t elements = get_json_subfields(target, ctx);

    json_object_iterator_t it = get_object_it(change);
    cJSON *val;
    while ((val = it.next())) {
        if (elements.find(val->string) == elements.end()) {
#ifndef NDEBUG
            logERR("Error, couldn't find element %s in adapter map.", val->string);
#else
            throw schema_mismatch_exc_t(strprintf("Couldn't find element %s.", val->string));
#endif
        } else {
            elements[val->string]->apply(val);
        }
    }
}

template <class T>
void apply_as_directory(cJSON *change, T *target) {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
    json_adapter_map_t elements = get_json_subfields(target);

    json_object_iterator_t it = get_object_it(change);
    cJSON *val;
    while ((val = it.next())) {
        if (elements.find(val->string) == elements.end()) {
#ifndef NDEBUG
            logERR("Error, couldn't find element %s in adapter map.", val->string);
#else
            throw schema_mismatch_exc_t(strprintf("Couldn't find element %s.", val->string));
#endif
        } else {
            elements[val->string]->apply(val);
        }
    }
}


#endif  // HTTP_JSON_JSON_ADAPTER_TCC_

#ifndef CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_TCC_
#define CLUSTERING_ADMINISTRATION_HTTP_JSON_ADAPTERS_TCC_

#include "clustering/administration/http/json_adapters.hpp"

#include <string>

#include "http/json.hpp"
#include "http/json/json_adapter.hpp"
#include "rpc/semilattice/joins/deletable.hpp"
#include "rpc/semilattice/joins/vclock.hpp"
#include "stl_utils.hpp"
#include "protocol_api.hpp"

//json adapter concept for vclock_t
template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(vclock_t<T> *target, const ctx_t &ctx) {
    try {
        return get_json_subfields(&target->get_mutable(), ctx);
    } catch (in_conflict_exc_t e) {
        return json_adapter_if_t::json_adapter_map_t();
    }
}

template <class T, class ctx_t>
cJSON *with_ctx_render_as_json(vclock_t<T> *target, const ctx_t &ctx) {
    try {
        T t = target->get();
        return render_as_json(&t, ctx);
    } catch (in_conflict_exc_t e) {
        return cJSON_CreateString("VALUE_IN_CONFLICT");
    }
}

template <class T, class ctx_t>
cJSON *with_ctx_render_all_values(vclock_t<T> *target, const ctx_t &ctx) {
    cJSON *res = cJSON_CreateArray();
    for (typename vclock_t<T>::value_map_t::iterator it  =   target->values.begin();
                                                             it != target->values.end();
                                                             it++) {
        cJSON *version_value_pair = cJSON_CreateArray();

        //This is really bad, fix this as soon as we can render using non const references
        vclock_details::version_map_t tmp = it->first;
        cJSON_AddItemToArray(version_value_pair, render_as_json(&tmp, ctx));
        cJSON_AddItemToArray(version_value_pair, render_as_json(&it->second, ctx));

        cJSON_AddItemToArray(res, version_value_pair);
    }

    return res;
}

template <class T, class ctx_t>
void with_ctx_apply_json_to(cJSON *change, vclock_t<T> *target, const ctx_t &ctx) {
    try {
        apply_json_to(change, &target->get_mutable(), ctx);
        target->upgrade_version(ctx.us);
    } catch (in_conflict_exc_t e) {
        throw multiple_choices_exc_t();
    }
}

template <class T, class ctx_t>
void with_ctx_on_subfield_change(vclock_t<T> *target, const ctx_t &ctx) {
    target->upgrade_version(ctx.us);
}


template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t json_vclock_resolver_t<T, ctx_t>::get_subfields_impl() {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class T, class ctx_t>
cJSON *json_vclock_resolver_t<T, ctx_t>::render_impl() {
    return with_ctx_render_all_values(target_, ctx_);
}

template <class T, class ctx_t>
void json_vclock_resolver_t<T, ctx_t>::apply_impl(cJSON *change) {
    T new_value;
    apply_json_to(change, &new_value, ctx_);

    *target_ = target_->make_resolving_version(new_value, ctx_.us);
}

template <class T, class ctx_t>
void json_vclock_resolver_t<T, ctx_t>::reset_impl() {
    throw permission_denied_exc_t("Can't reset a vclock resolver.\n");
}

template <class T, class ctx_t>
void json_vclock_resolver_t<T, ctx_t>::erase_impl() {
    throw permission_denied_exc_t("Can't erase a vclock resolver.\n");
}

template <class T, class ctx_t>
boost::shared_ptr<subfield_change_functor_t> json_vclock_resolver_t<T, ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>();
}

template <class T, class ctx_t>
json_vclock_resolver_t<T, ctx_t>::json_vclock_resolver_t(vclock_t<T> *target, const ctx_t &ctx)
    : target_(target), ctx_(ctx) { }

template <class T, class ctx_t>
json_vclock_adapter_t<T, ctx_t>::json_vclock_adapter_t(vclock_t<T> *target, const ctx_t &ctx)
    : target_(target), ctx_(ctx) { }

template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t json_vclock_adapter_t<T, ctx_t>::get_subfields_impl() {
    json_adapter_if_t::json_adapter_map_t res = with_ctx_get_json_subfields(target_, ctx_);
    rassert(!std_contains(res, "resolve"), "Programmer error: do not put anything with a \"resolve\" subfield in a vector clock.\n");
    res["resolve"] = boost::shared_ptr<json_adapter_if_t>(new json_vclock_resolver_t<T, ctx_t>(target_, ctx_));

    return res;
}

template <class T, class ctx_t>
cJSON *json_vclock_adapter_t<T, ctx_t>::render_impl() {
    return with_ctx_render_as_json(target_, ctx_);
}

template <class T, class ctx_t>
void json_vclock_adapter_t<T, ctx_t>::apply_impl(cJSON *change) {
    with_ctx_apply_json_to(change, target_, ctx_);
}

template <class T, class ctx_t>
void json_vclock_adapter_t<T, ctx_t>::reset_impl() {
    throw permission_denied_exc_t("Can't reset a vclock adapter.\n");
}

template <class T, class ctx_t>
void json_vclock_adapter_t<T, ctx_t>::erase_impl() {
    throw permission_denied_exc_t("Can't erase a vclock adapter.\n");
}

template <class T, class ctx_t>
boost::shared_ptr<subfield_change_functor_t>  json_vclock_adapter_t<T, ctx_t>::get_change_callback() {
    return boost::shared_ptr<subfield_change_functor_t>(new standard_ctx_subfield_change_functor_t<vclock_t<T>, ctx_t>(target_, ctx_));
}

//json adapter concept for deletable_t
template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(deletable_t<T> *target, const ctx_t &ctx) {
    return get_json_subfields(&target->get_mutable(), ctx);
}

template <class T, class ctx_t>
cJSON *render_as_json(deletable_t<T> *target, const ctx_t &ctx) {
    if (target->is_deleted()) {
        return cJSON_CreateNull();
    } else {
        return render_as_json(&target->get_mutable(), ctx);
    }
}

template <class T, class ctx_t>
void apply_json_to(cJSON *change, deletable_t<T> *target, const ctx_t &ctx) {
    if (is_null(change)) {
        target->mark_deleted();
    } else {
        apply_json_to(change, &target->get_mutable(), ctx);
    }
}

template <class T, class ctx_t>
void erase_json(deletable_t<T> *target, const ctx_t &) {
    *target = target->get_deletion();
}

template <class T, class ctx_t>
void on_subfield_change(deletable_t<T> *, const ctx_t &) { }

//json adapter concept for peer_id_t
template <class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(peer_id_t *, const ctx_t &) {
    return json_adapter_if_t::json_adapter_map_t();
}

template <class ctx_t>
cJSON *render_as_json(peer_id_t *target, const ctx_t &ctx) {
    uuid_t uuid = target->get_uuid();
    return render_as_json(&uuid, ctx);
}

template <class ctx_t>
void apply_json_to(cJSON *change, peer_id_t *target, const ctx_t &ctx) {
    uuid_t uuid;
    apply_json_to(change, &uuid, ctx);
    *target = peer_id_t(uuid);
}

template <class ctx_t>
void on_subfield_change(peer_id_t *, const ctx_t &) { }

/* A special adapter for a region_map's value */
template <class protocol_t, class value_t, class ctx_t>
class json_region_adapter_t : public json_adapter_if_t {
private:
    typedef region_map_t<protocol_t, value_t> target_region_map_t;
public:
    json_region_adapter_t(target_region_map_t *_parent, typename protocol_t::region_t _target_region, const ctx_t &_ctx)
        : parent(_parent), target_region(_target_region), ctx(_ctx) { }
private:
    json_adapter_if_t::json_adapter_map_t get_subfields_impl() {
            return json_adapter_if_t::json_adapter_map_t();
    }

    cJSON *render_impl() {
        target_region_map_t target_map = parent->mask(target_region);
        return render_as_json(&target_map, ctx);
    }

    void apply_impl(cJSON *change) {
        value_t val;
        apply_json_to(change, &val, ctx);

        parent->set(target_region, val);
    }

    void erase_impl() {
        throw permission_denied_exc_t("Can't erase from a region map.\n");
    }

    void reset_impl() {
        throw permission_denied_exc_t("Can't reset from a region map.\n");
    }

    /* follows the creation paradigm, ie the caller is responsible for the
     * object this points to */
    boost::shared_ptr<subfield_change_functor_t>  get_change_callback() {
        return boost::shared_ptr<subfield_change_functor_t>(new noop_subfield_change_functor_t());
    }

    target_region_map_t *parent;
    typename protocol_t::region_t target_region;
    const ctx_t ctx;

    DISABLE_COPYING(json_region_adapter_t);
};

//json adapter concept for region map
template <class protocol_t, class value_t, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(region_map_t<protocol_t, value_t> *target, const ctx_t &ctx) {
    json_adapter_if_t::json_adapter_map_t res;
    for (typename region_map_t<protocol_t, value_t>::iterator it  = target->begin();
                                                              it != target->end();
                                                              ++it) {
        scoped_cJSON_t key(render_as_json(&it->first, ctx));
        rassert(key.get()->type == cJSON_String);
        res[get_string(key.get())] = boost::shared_ptr<json_adapter_if_t>(new json_region_adapter_t<protocol_t, value_t, ctx_t>(target, it->first, ctx));
    }

    return res;
}

template <class protocol_t, class value_t, class ctx_t>
cJSON *render_as_json(region_map_t<protocol_t, value_t> *target, const ctx_t &ctx) {
    cJSON *res = cJSON_CreateObject();
    for (typename region_map_t<protocol_t, value_t>::iterator it  = target->begin();
                                                              it != target->end();
                                                              ++it) {
        std::string key(render_region_as_string(&it->first, ctx));
        cJSON_AddItemToObject(res, key.c_str(), render_as_json(&it->second, ctx));
    }

    return res;
}

template <class protocol_t, class value_t, class ctx_t>
void apply_json_to(cJSON *change, region_map_t<protocol_t, value_t> *target, const ctx_t &ctx) {
    json_object_iterator_t it = get_object_it(change);
    cJSON *hd;

    while ((hd = it.next())) {
       typename protocol_t::region_t key;
       value_t val;

       scoped_cJSON_t key_desc(cJSON_CreateString(hd->string));
       apply_json_to(key_desc.get(), &key, ctx);
       apply_json_to(hd, &val, ctx);

       target->set(key, val);
    }
}

template <class protocol_t, class value_t, class ctx_t>
void on_subfield_change(region_map_t<protocol_t, value_t> *, const ctx_t &) { }

#endif

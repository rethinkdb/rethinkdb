#ifndef HTTP_JSON_JSON_ADAPTER_HPP_
#define HTTP_JSON_JSON_ADAPTER_HPP_

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include "containers/uuid.hpp"
#include "http/json.hpp"

/* A note about json adapter exceptions: When an operation throws an exception
 * there is no guarantee that the target object has been left in tact.
 * Generally this is okay because we first apply changes and then join them in
 * to semilattice metadata. Generally once a particular object has thrown one
 * of these exceptions it should probably not be used anymore. */
struct json_adapter_exc_t : public std::exception {
    virtual const char *what() const throw () {
        return "Generic json adapter exception\n";
    }

    virtual ~json_adapter_exc_t() throw () { }
};

struct schema_mismatch_exc_t : public json_adapter_exc_t {
private:
    std::string desc;
public:
    explicit schema_mismatch_exc_t(const std::string &_desc)
        : desc(_desc)
    { }

    const char *what() const throw () {
        return desc.c_str();
    }

    ~schema_mismatch_exc_t() throw () { }
};

struct permission_denied_exc_t : public json_adapter_exc_t {
private:
    std::string desc;
public:
    explicit permission_denied_exc_t(const std::string &_desc)
        : desc(_desc)
    { }

    const char *what() const throw () {
        return desc.c_str();
    }

    ~permission_denied_exc_t() throw () { }
};

struct multiple_choices_exc_t : public json_adapter_exc_t {
    virtual const char *what() const throw () {
        return "Multiple choices exists for this json value (probably vector clock divergence).";
    }
};

//Type checking functions

bool is_null(cJSON *json);

//Functions to make accessing cJSON *objects easier

bool get_bool(cJSON *json);

std::string get_string(cJSON *json);

int get_int(cJSON *json);

double get_double(cJSON *json);

json_array_iterator_t get_array_it(cJSON *json);

json_object_iterator_t get_object_it(cJSON *json);

class subfield_change_functor_t {
public:
    subfield_change_functor_t() { }
    virtual void on_change() = 0;
    virtual ~subfield_change_functor_t() { }

private:
    DISABLE_COPYING(subfield_change_functor_t);
};

class noop_subfield_change_functor_t : public subfield_change_functor_t {
public:
    void on_change() { }
};

template <class T>
class standard_subfield_change_functor_t : public subfield_change_functor_t {
public:
    standard_subfield_change_functor_t(T *target);
    void on_change();
private:
    T *target;

    DISABLE_COPYING(standard_subfield_change_functor_t);
};

template <class T, class ctx_t>
class standard_ctx_subfield_change_functor_t : public subfield_change_functor_t {
public:
    standard_ctx_subfield_change_functor_t(T *target, const ctx_t &ctx);
    void on_change();
private:
    T *target;
    const ctx_t ctx;

    DISABLE_COPYING(standard_ctx_subfield_change_functor_t);
};

//TODO come up with a better name for this
class json_adapter_if_t {
public:
    typedef std::map<std::string, boost::shared_ptr<json_adapter_if_t> > json_adapter_map_t;

public:
    json_adapter_if_t() { }
    virtual ~json_adapter_if_t() { }

    json_adapter_map_t get_subfields();
    cJSON *render();
    void apply(cJSON *);
    void erase();
    void reset();

private:
    virtual json_adapter_map_t get_subfields_impl() = 0;
    virtual cJSON *render_impl() = 0;
    virtual void apply_impl(cJSON *) = 0;
    virtual void erase_impl() = 0;
    virtual void reset_impl() = 0;
    /* follows the creation paradigm, ie the caller is responsible for the
     * object this points to */
    virtual boost::shared_ptr<subfield_change_functor_t>  get_change_callback() = 0;

    std::vector<boost::shared_ptr<subfield_change_functor_t> > superfields;

    DISABLE_COPYING(json_adapter_if_t);
};

/* A json adapter is the most basic adapter, you can instantiate one with any
 * type that implements the json adapter concept as T */
template <class T>
class json_adapter_t : public json_adapter_if_t {
private:
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
public:
    json_adapter_t(T *);

private:
    json_adapter_map_t get_subfields_impl();
    cJSON *render_impl();
    virtual void apply_impl(cJSON *);
    virtual void erase_impl();
    virtual void reset_impl();
    boost::shared_ptr<subfield_change_functor_t> get_change_callback();

    T *target_;

    DISABLE_COPYING(json_adapter_t);
};

/* A json adapter is the most basic adapter, you can instantiate one with any
 * type that implements the json adapter concept as T */
template <class T, class ctx_t>
class json_ctx_adapter_t : public json_adapter_if_t {
private:
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
public:
    json_ctx_adapter_t(T *, const ctx_t &);

private:
    json_adapter_map_t get_subfields_impl();
    cJSON *render_impl();
    virtual void apply_impl(cJSON *);
    virtual void erase_impl();
    virtual void reset_impl();
    boost::shared_ptr<subfield_change_functor_t> get_change_callback();

    T *target_;
    const ctx_t ctx_;

    DISABLE_COPYING(json_ctx_adapter_t);
};


/* A read only adapter is like a normal adapter but it throws an exception when
 * you try to do an apply call. */
template <class T>
class json_read_only_adapter_t : public json_adapter_t<T> {
public:
    json_read_only_adapter_t(T *);

private:
    void apply_impl(cJSON *);
    void erase_impl();
    void reset_impl();

    DISABLE_COPYING(json_read_only_adapter_t);
};

template <class T, class ctx_t>
class json_ctx_read_only_adapter_t : public json_ctx_adapter_t<T, ctx_t> {
public:
    json_ctx_read_only_adapter_t(T *, const ctx_t &);

private:
    void apply_impl(cJSON *);
    void erase_impl();
    void reset_impl();

    DISABLE_COPYING(json_ctx_read_only_adapter_t);
};

/* A json temporary adapter is like a read only adapter but it stores a copy of
 * the what it's adapting inside it. This is convenient when we want to have
 * json data that's not actually reflected in our structures such as having the
 * id of every element in a map referenced in an id field */
template <class T>
class json_temporary_adapter_t : public json_read_only_adapter_t<T> {
public:
    json_temporary_adapter_t(const T &value);

private:
    T value_;

    DISABLE_COPYING(json_temporary_adapter_t);
};

/* This adapter is a little bit different from the other ones, it's meant to
 * target a map and allow a way to insert in to it, using serveside generated
 * keys. Because of this the render and apply functions have different schemas
 * in particular:
 *
 *  If target is of type map<K,V>
 *  schema(render) = schema(render_as_json(map<K,V>))
 *  schema(apply) = shchema(apply_json_to(V))
 *
 *  Rendering an inserter will give you only the entries in the map that were
 *  created using this inserter. Thus rendering with a newly constructed
 *  inserter gives you an empty map.
 */
template <class container_t>
class json_map_inserter_t : public json_adapter_if_t {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
    typedef boost::function<typename container_t::key_type()> gen_function_t;
    typedef typename container_t::mapped_type value_t;
    typedef std::set<typename container_t::key_type> keys_set_t;

public:
    json_map_inserter_t(container_t *, gen_function_t, value_t _initial_value = value_t());

private:
    cJSON *render_impl();
    void apply_impl(cJSON *);
    void erase_impl();
    void reset_impl();
    json_adapter_map_t get_subfields_impl();
    boost::shared_ptr<subfield_change_functor_t> get_change_callback();

    container_t *target;
    gen_function_t generator;
    value_t initial_value;
    keys_set_t added_keys;

    DISABLE_COPYING(json_map_inserter_t);
};

template <class container_t, class ctx_t>
class json_ctx_map_inserter_t : public json_adapter_if_t {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
    typedef boost::function<typename container_t::key_type()> gen_function_t;
    typedef typename container_t::mapped_type value_t;
    typedef std::set<typename container_t::key_type> keys_set_t;

public:
    json_ctx_map_inserter_t(container_t *, gen_function_t, const ctx_t &ctx, value_t _initial_value = value_t());

private:
    cJSON *render_impl();
    void apply_impl(cJSON *);
    void erase_impl();
    void reset_impl();
    json_adapter_map_t get_subfields_impl();
    boost::shared_ptr<subfield_change_functor_t> get_change_callback();

    container_t *target;
    gen_function_t generator;
    value_t initial_value;
    keys_set_t added_keys;
    const ctx_t ctx;

    DISABLE_COPYING(json_ctx_map_inserter_t);
};


/* This combines the inserter json adapter with the standard adapter for a map,
 * thus creating an adapter for a map with which we can do normal modifications
 * and insertions */
template <class container_t>
class json_adapter_with_inserter_t : public json_adapter_if_t {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
    typedef boost::function<typename container_t::key_type()> gen_function_t;
    typedef typename container_t::mapped_type value_t;
public:
    json_adapter_with_inserter_t(container_t *, gen_function_t, value_t _initial_value = value_t(), std::string _inserter_key = std::string("new"));
private:
    json_adapter_map_t get_subfields_impl();
    cJSON *render_impl();
    void apply_impl(cJSON *);
    void erase_impl();
    void reset_impl();
    boost::shared_ptr<subfield_change_functor_t> get_change_callback();
private:
    container_t *target;
    gen_function_t generator;
    value_t initial_value;
    std::string inserter_key;

    DISABLE_COPYING(json_adapter_with_inserter_t);
};

template <class container_t, class ctx_t>
class json_ctx_adapter_with_inserter_t : public json_adapter_if_t {
    typedef json_adapter_if_t::json_adapter_map_t json_adapter_map_t;
    typedef boost::function<typename container_t::key_type()> gen_function_t;
    typedef typename container_t::mapped_type value_t;
public:
    json_ctx_adapter_with_inserter_t(container_t *, gen_function_t, const ctx_t &ctx, value_t _initial_value = value_t(), std::string _inserter_key = std::string("new"));
private:
    json_adapter_map_t get_subfields_impl();
    cJSON *render_impl();
    void apply_impl(cJSON *);
    void erase_impl();
    void reset_impl();
    boost::shared_ptr<subfield_change_functor_t> get_change_callback();
private:
    container_t *target;
    gen_function_t generator;
    value_t initial_value;
    std::string inserter_key;
    const ctx_t ctx;

    DISABLE_COPYING(json_ctx_adapter_with_inserter_t);
};


/* Erase is a fairly rare function for an adapter to allow so we implement a
 * generic version of it. */
template <class T>
void erase_json(T *) {
#ifndef NDEBUG
    throw permission_denied_exc_t("Can't erase this object.. by default"
            "json_adapters disallow deletion. if you'd like to be able to please"
            "implement a working erase method for it.");
#else
    throw permission_denied_exc_t("Can't erase this object.");
#endif
}

template <class T, class ctx_t>
void erase_json(T *target, const ctx_t &) {
    erase_json(target);
}


template <class T, class ctx_t>
void with_ctx_erase_json(T *target, const ctx_t &ctx) {
    return erase_json(target, ctx);
}


/* Reset is a fairly rare function for an adapter to allow so we implement a
 * generic version of it. */
template <class T>
void reset_json(T *) {
#ifndef NDEBUG
    throw permission_denied_exc_t("Can't reset this object.. by default"
            "json_adapters disallow deletion. if you'd like to be able to please"
            "implement a working reset method for it.");
#else
    throw permission_denied_exc_t("Can't reset this object.");
#endif
}

template <class T, class ctx_t>
void reset_json(T *target, const ctx_t &) {
    reset_json(target);
}

template <class T, class ctx_t>
void with_ctx_reset_json(T *target, const ctx_t &ctx) {
    reset_json(target, ctx);
}


/* Here we have implementations of the json adapter concept for several
 * prominent types, these could in theory be relocated to a different file if
 * need be */


// ctx-less JSON adapter for int
json_adapter_if_t::json_adapter_map_t get_json_subfields(int *);
cJSON *render_as_json(int *);
void apply_json_to(cJSON *, int *);
void on_subfield_change(int *);


// ctx-less JSON adapter for time_t;
json_adapter_if_t::json_adapter_map_t get_json_subfields(time_t *);
cJSON *render_as_json(time_t *);
void apply_json_to(cJSON *, time_t *);
void on_subfield_change(time_t *);


// ctx-less JSON adapter for uint64_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(uint64_t *);
cJSON *render_as_json(uint64_t *);
void apply_json_to(cJSON *, uint64_t *);
void on_subfield_change(uint64_t *);


// ctx-less JSON adapter for bool
json_adapter_if_t::json_adapter_map_t get_json_subfields(bool *);
cJSON *render_as_json(bool *);
void apply_json_to(cJSON *, bool *);
void on_subfield_change(bool *);

// ctx-less JSON adapter for uuid_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(uuid_t *);
cJSON *render_as_json(const uuid_t *);
void apply_json_to(cJSON *, uuid_t *);
void on_subfield_change(uuid_t *);


namespace boost {

// ctx-less JSON adapter for boost::variant
template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
json_adapter_if_t::json_adapter_map_t get_json_subfields(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *);

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
cJSON *render_as_json(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *);

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
void apply_json_to(cJSON *, boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *);

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class T11, class T12, class T13, class T14, class T15, class T16, class T17, class T18, class T19, class T20>
void on_subfield_change(boost::variant<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20> *);

} //namespace boost

namespace std {
//JSON adapter for std::string
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::string *);
cJSON *render_as_json(std::string *);
void apply_json_to(cJSON *, std::string *);
void  on_subfield_change(std::string *);

//JSON adapter for std::map
template <class K, class V, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::map<K, V> *, const ctx_t &);

template <class K, class V, class ctx_t>
cJSON *render_as_json(std::map<K, V> *, const ctx_t &);

template <class K, class V, class ctx_t>
void apply_json_to(cJSON *, std::map<K, V> *, const ctx_t &);

template <class K, class V, class ctx_t>
void on_subfield_change(std::map<K, V> *, const ctx_t &);

// ctx-less JSON adapter for std::map
template <class K, class V>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::map<K, V> *);

template <class K, class V>
cJSON *render_as_json(std::map<K, V> *);

template <class K, class V>
void apply_json_to(cJSON *, std::map<K, V> *);

template <class K, class V>
void on_subfield_change(std::map<K, V> *);


// ctx-less JSON adapter for std::set
template <class V>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::set<V> *);

template <class V>
cJSON *render_as_json(std::set<V> *);

template <class V>
void apply_json_to(cJSON *, std::set<V> *);

template <class V>
void on_subfield_change(std::set<V> *);


//JSON adapter for std::pair
template <class F, class S, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::pair<F, S> *, const ctx_t &);

template <class F, class S, class ctx_t>
cJSON *render_as_json(std::pair<F, S> *, const ctx_t &);

template <class F, class S, class ctx_t>
void apply_json_to(cJSON *, std::pair<F, S> *, const ctx_t &);

template <class F, class S, class ctx_t>
void on_subfield_change(std::pair<F, S> *, const ctx_t &);

// ctx-less JSON adapter for std::pair
template <class F, class S>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::pair<F, S> *);

template <class F, class S>
cJSON *render_as_json(std::pair<F, S> *);

template <class F, class S>
void apply_json_to(cJSON *, std::pair<F, S> *);

template <class F, class S>
void on_subfield_change(std::pair<F, S> *);


//JSON adapter for std::vector
template <class V, class ctx_t>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::vector<V> *, const ctx_t &);

template <class V, class ctx_t>
cJSON *render_as_json(std::vector<V> *, const ctx_t &);

template <class V, class ctx_t>
void apply_json_to(cJSON *, std::vector<V> *, const ctx_t &);

template <class V, class ctx_t>
void on_subfield_change(std::vector<V> *, const ctx_t &);

// ctx-less JSON adapter for std::vector
template <class V>
json_adapter_if_t::json_adapter_map_t get_json_subfields(std::vector<V> *);

template <class V>
cJSON *render_as_json(std::vector<V> *);

template <class V>
void apply_json_to(cJSON *, std::vector<V> *);

template <class V>
void on_subfield_change(std::vector<V> *);

} //namespace std

//some convenience functions
template <class T, class ctx_t>
cJSON *render_as_directory(T *, const ctx_t &);

template <class T>
cJSON *render_as_directory(T *);

template <class T, class ctx_t>
void apply_as_directory(cJSON *change, T *, const ctx_t &);

template <class T>
void apply_as_directory(cJSON *change, T *);


template<class T, class cxt_t>
std::string render_as_json_string(const T &t, const cxt_t cxt) {
    T copy = t;
    scoped_cJSON_t json(render_as_json(&copy, cxt));
    return json.PrintUnformatted();
}

template<class T>
std::string render_as_json_string(const T &t) {
    return render_as_json_string(t, 0);
}

// ctx-dropping adapters, eventually.
template <class T, class ctx_t>
json_adapter_if_t::json_adapter_map_t with_ctx_get_json_subfields(T *target, const ctx_t &ctx) {
    return get_json_subfields(target, ctx);
}

template <class T, class ctx_t>
cJSON *with_ctx_render_as_json(T *target, const ctx_t &ctx) {
    return render_as_json(target, ctx);
}

template <class T, class ctx_t>
void with_ctx_apply_json_to(cJSON *change, T *target, const ctx_t &ctx) {
    apply_json_to(change, target, ctx);
}

template <class T, class ctx_t>
void with_ctx_on_subfield_change(T *target, const ctx_t &ctx) {
    on_subfield_change(target, ctx);
}

#include "http/json/json_adapter.tcc"

#endif /* HTTP_JSON_JSON_ADAPTER_HPP_ */

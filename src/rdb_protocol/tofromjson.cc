#include "rdb_protocol/javascript_impl.hpp"

#include "errors.hpp"
#include <boost/scoped_array.hpp>
#include <boost/make_shared.hpp>

namespace js {

static void get_string(boost::scoped_array<char> *out, const v8::Handle<v8::String> string) {
    // Create buffer to hold string contents.
    //
    // It's really stupid that we have to do this, because it involves a double
    // copy: once from the v8::String to our buffer, then from our buffer to a
    // cJSON buffer, because cJSON strdup()s the strings its given.
    //
    // NB. v8::String::Length() gives us the number of logical characters, which
    // we don't want. Instead, we use v8::String::Utf8Length(), which us the
    // number of bytes in the utf-8 representation.
    //
    // We add 1 because cJSON is stupid and needs a terminating null byte.
    //
    // TODO(rntz): What if the string contains a null character? cJSON
    // assumes null termination, which means it'll cut off the string. We
    // should switch away from cJSON.
    int length = string->Utf8Length() + 1;
    out->reset(new char[length]);
    string->WriteUtf8(out->get(), length);
}

// Returns NULL & sets `*errmsg` on failure.
//
// FIXME: implement max recursion depth, to avoid potential security
// vulnerabilies if nothing else.
//
// FIXME: avoid infinite recursion on cyclic data structures. Not sure whether
// there's a good way to detect this, or if I should just rely on maximum
// recursion depth.
static cJSON *mkJSON(const v8::Handle<v8::Value> value, std::string *errmsg) {
    // TODO(rntz): should we handle BooleanObject, NumberObject, StringObject?
    v8::HandleScope handle_scope;

    if (value->IsString()) {
        boost::scoped_array<char> buf;
        get_string(&buf, value->ToString());

        // NB. cJSON_CreateString strdup()s the buffer it's given.
        cJSON *r = cJSON_CreateString(buf.get());
        if (NULL == r) *errmsg = "cJSON_CreateString() failed";
        return r;

    } else if (value->IsObject()) {
        // This case is kinda weird. Objects can have stuff in them that isn't
        // represented in their JSON (eg. their prototype, v8 hidden fields).

        if (value->IsArray()) {
            v8::Handle<v8::Array> arrayh = v8::Handle<v8::Array>::Cast(value);
            scoped_cJSON_t arrayj(cJSON_CreateArray());
            if (NULL == arrayj.get()) {
                *errmsg = "cJSON_CreateArray() failed";
                return NULL;
            }

            uint32_t len = arrayh->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::Value> elth = arrayh->Get(i);
                // FIXME: theoretically this could raise a JS error, and then
                // elt would be empty.
                rassert(!elth.IsEmpty());

                cJSON *eltj = mkJSON(elth, errmsg);
                if (NULL == eltj) return NULL;

                arrayj.AddItemToArray(eltj);
            }

            return arrayj.release();

        } else if (value->IsFunction()) {
            // We can't represent functions in JSON.
            *errmsg = "Can't convert function to JSON";
            return NULL;

        } else if (value->IsRegExp()) {
            // Ditto.
            *errmsg = "Can't convert RegExp to JSON";
            return NULL;

        } else {
            // Treat it as a dictionary.
            v8::Handle<v8::Object> objh = value->ToObject();
            v8::Handle<v8::Array> props = objh->GetPropertyNames();
            rassert(!objh.IsEmpty() && !props.IsEmpty()); // FIXME.

            scoped_cJSON_t objj(cJSON_CreateObject());
            if (NULL == objj.get()) {
                *errmsg = "cJSON_CreateObject() failed";
                return NULL;
            }

            uint32_t len = props->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::Value> keyh = props->Get(i);
                v8::Handle<v8::Value> valueh = objh->Get(keyh);
                rassert(!keyh.IsEmpty() && !valueh.IsEmpty()); // FIXME.

                cJSON *valuej = mkJSON(valueh, errmsg);
                if (NULL == valuej) return NULL;

                boost::scoped_array<char> buf;
                // FIXME TODO (rntz): could a property name be a non-string?
                get_string(&buf, keyh->ToString());

                objj.AddItemToObject(buf.get(), valuej);
            }

            return objj.release();
        }

    } else if (value->IsNumber()) {
        cJSON *r = cJSON_CreateNumber(value->NumberValue());
        if (!r) *errmsg = "cJSON_CreateNumber() failed";
        return r;

    } else if (value->IsBoolean()) {
        cJSON *r = cJSON_CreateBool(value->BooleanValue());
        if (!r) *errmsg = "cJSON_CreateBool() failed";
        return r;

    } else if (value->IsNull()) {
        cJSON *r = cJSON_CreateNull();
        if (!r) *errmsg = "cJSON_CreateNull() failed";
        return r;

    } else {
        *errmsg = "unknown value type when converting value to json";
        return NULL;
    }
}

boost::shared_ptr<scoped_cJSON_t> toJSON(const v8::Handle<v8::Value> value, std::string *errmsg) {
    rassert(!value.IsEmpty());
    rassert(errmsg);

    // TODO (rntz): probably want a TryCatch for javascript errors that might happen.
    v8::HandleScope handle_scope;
    *errmsg = "unknown error when converting to JSON";

    cJSON *json = mkJSON(value, errmsg);
    boost::shared_ptr<scoped_cJSON_t> ptr = boost::make_shared<scoped_cJSON_t>(json);
    return ptr;
}

v8::Handle<v8::Value> fromJSON(const cJSON &json) {
    // FIXME TODO (rntz)
    (void) json;
    crash("unimplemented");
}

} // namespace js

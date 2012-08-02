#include "errors.hpp"
#include <boost/scoped_array.hpp>
#include <boost/make_shared.hpp>

#include "rdb_protocol/jsimpl.hpp"

// Picked from a hat.
#define TOJSON_RECURSION_LIMIT  500

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
// TODO(rntz): Is there a better way of detecting cyclic data structures than
// using a recursion limit?
static cJSON *mkJSON(const v8::Handle<v8::Value> value, int recursion_limit, std::string *errmsg) {
    if (0 == recursion_limit) {
        *errmsg = "toJSON recursion limit exceeded";
        return NULL;
    }
    --recursion_limit;

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

                cJSON *eltj = mkJSON(elth, recursion_limit, errmsg);
                if (NULL == eltj) return NULL;

                // TODO (rntz) FIXME: this is O(n). do this better by explicitly
                // manipulating object chains.
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
            rassert(!objh.IsEmpty()); // FIXME
            v8::Handle<v8::Array> props = objh->GetPropertyNames();
            rassert(!props.IsEmpty()); // FIXME

            scoped_cJSON_t objj(cJSON_CreateObject());
            if (NULL == objj.get()) {
                *errmsg = "cJSON_CreateObject() failed";
                return NULL;
            }

            uint32_t len = props->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::String> keyh = props->Get(i)->ToString();
                rassert(!keyh.IsEmpty()); // FIXME
                v8::Handle<v8::Value> valueh = objh->Get(keyh);
                rassert(!valueh.IsEmpty()); // FIXME

                cJSON *valuej = mkJSON(valueh, recursion_limit, errmsg);
                if (NULL == valuej) return NULL;

                boost::scoped_array<char> buf;
                get_string(&buf, keyh);

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
        // TODO (rntz): detect javascript `undefined` & give better error
        // message for it
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

    cJSON *json = mkJSON(value, TOJSON_RECURSION_LIMIT, errmsg);
    if (json) {
        return boost::make_shared<scoped_cJSON_t>(json);
    } else {
        return boost::shared_ptr<scoped_cJSON_t>();
    }
}

v8::Handle<v8::Value> fromJSON(const cJSON &json) {
    switch (json.type & ~cJSON_IsReference) {
      case cJSON_False: return v8::False();

      case cJSON_True: return v8::True();

      case cJSON_NULL: return v8::Null();

      case cJSON_Number: return v8::Number::New(json.valuedouble);

      case cJSON_String: return v8::String::New(json.valuestring);

      case cJSON_Array: {
          v8::Handle<v8::Array> array = v8::Array::New();

          uint32_t index = 0;
          for (cJSON *child = json.child; child; child = child->next, ++index) {
              v8::HandleScope scope;
              v8::Handle<v8::Value> val = fromJSON(*child);
              rassert(!val.IsEmpty());
              array->Set(index, val);
              // FIXME: try_catch code
          }

          return array;
      }

      case cJSON_Object: {
          v8::Handle<v8::Object> obj = v8::Object::New();

          for (cJSON *child = json.child; child; child = child->next) {
              v8::HandleScope scope;
              v8::Handle<v8::Value> key = v8::String::New(child->string);
              v8::Handle<v8::Value> val = fromJSON(*child);
              rassert(!key.IsEmpty() && !val.IsEmpty());

              obj->Set(key, val); // FIXME: try_catch code
          }

          return obj;
      }

      default:
        crash("bad cJSON value");
    }
}

} // namespace js

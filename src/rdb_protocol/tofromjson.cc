#include <stdlib.h>

#include <math.h>

#include "errors.hpp"
#include <boost/scoped_array.hpp>
#include <boost/make_shared.hpp>

#include "rdb_protocol/jsimpl.hpp"

// Picked from a hat.
#define TOJSON_RECURSION_LIMIT  500

namespace js {

// Returns new "last" pointer.
static cJSON *append_cJSON(cJSON *root, cJSON *last, cJSON *append) {
    if (last) {
        last->next = append;
        append->prev = last;
    } else {
        root->child = append;
    }
    return append;
}

// Returns NULL & sets `*errmsg` on failure.
//
// TODO(rntz): Is there a better way of detecting cyclic data structures than
// using a recursion limit?
static cJSON *mkJSON(const v8::Handle<v8::Value> value, int recursion_limit, std::string *errmsg) {
    if (0 == recursion_limit) {
        *errmsg = "toJSON recursion limit exceeded (cyclic datastructure?)";
        return NULL;
    }
    --recursion_limit;

    // TODO(rntz): should we handle BooleanObject, NumberObject, StringObject?
    v8::HandleScope handle_scope;

    if (value->IsString()) {
        cJSON *p = cJSON_CreateBlank();
        if (NULL == p) {
            *errmsg = "cJSON_CreateBlank() failed";
            return NULL;
        }
        scoped_cJSON_t result(p);

        // Copy in the string. TODO(rntz): cJSON requires null termination. We
        // should switch away from cJSON.
        v8::Handle<v8::String> string = value->ToString();
        guarantee_reviewed(!string.IsEmpty());
        int length = string->Utf8Length() + 1; // +1 for null byte

        p->type = cJSON_String;
        p->valuestring = reinterpret_cast<char *>(malloc(length));
        if (NULL == p->valuestring) {
            *errmsg = "failed to allocate space for string";
            return NULL;
        }
        string->WriteUtf8(p->valuestring, length);

        return result.release();

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

            cJSON *last = NULL;
            uint32_t len = arrayh->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::Value> elth = arrayh->Get(i);
                guarantee_reviewed(!elth.IsEmpty()); // FIXME

                cJSON *eltj = mkJSON(elth, recursion_limit, errmsg);
                if (NULL == eltj) return NULL;

                // Append it to the array.
                last = append_cJSON(arrayj.get(), last, eltj);
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
            guarantee_reviewed(!objh.IsEmpty()); // FIXME
            v8::Handle<v8::Array> props = objh->GetPropertyNames();
            guarantee_reviewed(!props.IsEmpty()); // FIXME

            scoped_cJSON_t objj(cJSON_CreateObject());
            if (NULL == objj.get()) {
                *errmsg = "cJSON_CreateObject() failed";
                return NULL;
            }

            cJSON *last = NULL;
            uint32_t len = props->Length();
            for (uint32_t i = 0; i < len; ++i) {
                v8::Handle<v8::String> keyh = props->Get(i)->ToString();
                guarantee_reviewed(!keyh.IsEmpty()); // FIXME
                v8::Handle<v8::Value> valueh = objh->Get(keyh);
                guarantee_reviewed(!valueh.IsEmpty()); // FIXME

                scoped_cJSON_t valuej(mkJSON(valueh, recursion_limit, errmsg));
                if (NULL == valuej.get()) return NULL;

                // Create string key.
                int length = keyh->Utf8Length() + 1; // +1 for null byte.
                char *str = valuej.get()->string = reinterpret_cast<char *>(malloc(length));
                if (NULL == str) {
                    *errmsg = "could not allocate space for string";
                    return NULL;
                }
                keyh->WriteUtf8(str, length);

                // Append to object.
                last = append_cJSON(objj.get(), last, valuej.release());
            }

            return objj.release();
        }

    } else if (value->IsNumber()) {
        double d = value->NumberValue();
        cJSON *r = 0;
        if (isfinite(d)) r = cJSON_CreateNumber(value->NumberValue());
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
        *errmsg = value->IsUndefined() ? "cannot convert javascript 'undefined' to JSON"
                                       : "unrecognized value type when converting to JSON";
        return NULL;
    }
}

boost::shared_ptr<scoped_cJSON_t> toJSON(const v8::Handle<v8::Value> value, std::string *errmsg) {
    guarantee_reviewed(!value.IsEmpty());
    guarantee_reviewed(errmsg);

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
              guarantee_reviewed(!val.IsEmpty());
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
              guarantee_reviewed(!key.IsEmpty() && !val.IsEmpty());

              obj->Set(key, val); // FIXME: try_catch code
          }

          return obj;
      }

      default:
        crash("bad cJSON value");
    }
}

} // namespace js

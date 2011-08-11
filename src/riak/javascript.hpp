#ifndef __RIAK_JAVASCRIPT__
#define __RIAK_JAVASCRIPT__

#include <JavaScriptCore/JavaScript.h>
#include "API/JSContextRefPrivate.h"
#include "arch/runtime/context_switching.hpp"
#include "riak/riak_interface.hpp"
#include "http/json.hpp"

/* We need to hand the object_t to the javascript function is the following format:
 *
 * struct value_t {
 *     string data;
 * };
 *
 * struct input_t {
 *     [value_t] values;
 * };
 */

namespace riak {
namespace json = json_spirit;
    JSValueRef object_to_jsvalue(JSContextRef, object_t &);

    std::string js_obj_to_string(JSStringRef);

} //namespace riak

#endif

#include "riak/javascript.hpp"
#include <boost/regex.hpp>

namespace riak { 
    JSValueRef object_to_jsvalue(JSContextRef ctx, object_t &obj) {
        json::mObject js_obj;

        js_obj["key"] = obj.key;

        js_obj["values"] = json::mArray();
        js_obj["values"].get_array().push_back(json::mObject());
        js_obj["values"].get_array()[0].get_obj()["data"] = std::string(obj.content.get(), obj.content_length); //extra copy
        return JSValueMakeFromJSONString(ctx, JSStringCreateWithUTF8CString(json::write_string(json::mValue(js_obj)).c_str()));
    }

    std::string js_obj_to_string(JSStringRef str) {
        char result_buf[1024];
        JSStringGetUTF8CString(str, result_buf, sizeof(result_buf));
        return std::string(result_buf);
    }

    js_ctx_t::js_ctx_t() 
        : m_ctxGroup(JSContextGroupCreate()), m_ctx(JSGlobalContextCreateInGroup(m_ctxGroup, NULL))
    { }

    js_ctx_t::~js_ctx_t() {
        JSGlobalContextRelease(m_ctx);
        JSContextGroupRelease(m_ctxGroup);

    JSSetStackBounds(m_ctxGroup,
                     coro_t::self()->get_stack()->get_stack_base(),
                     coro_t::self()->get_stack()->get_stack_bound());
    }

    JSContextGroupRef js_ctx_t::get_ctx_group() {
        return m_ctxGroup;
    }

    JSGlobalContextRef js_ctx_t::get_ctx() {
        return m_ctx;
    }
} //namespace riak 

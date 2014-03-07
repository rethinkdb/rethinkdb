// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef TCP_WRAP_H_
#define TCP_WRAP_H_
#include "stream_wrap.h"

namespace node {

class TCPWrap : public StreamWrap {
 public:
  static v8::Local<v8::Object> Instantiate();
  static TCPWrap* Unwrap(v8::Local<v8::Object> obj);
  static void Initialize(v8::Handle<v8::Object> target);

  uv_tcp_t* UVHandle();

 private:
  TCPWrap(v8::Handle<v8::Object> object);
  ~TCPWrap();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetSockName(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetPeerName(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetNoDelay(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetKeepAlive(const v8::Arguments& args);
  static v8::Handle<v8::Value> Bind(const v8::Arguments& args);
  static v8::Handle<v8::Value> Bind6(const v8::Arguments& args);
  static v8::Handle<v8::Value> Listen(const v8::Arguments& args);
  static v8::Handle<v8::Value> Connect(const v8::Arguments& args);
  static v8::Handle<v8::Value> Connect6(const v8::Arguments& args);
  static v8::Handle<v8::Value> Open(const v8::Arguments& args);

#ifdef _WIN32
  static v8::Handle<v8::Value> SetSimultaneousAccepts(const v8::Arguments& args);
#endif

  static void OnConnection(uv_stream_t* handle, int status);
  static void AfterConnect(uv_connect_t* req, int status);

  uv_tcp_t handle_;
};


}  // namespace node


#endif  // TCP_WRAP_H_

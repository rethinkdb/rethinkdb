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


#ifdef HAVE_DTRACE
#include "node_dtrace.h"
#include <string.h>
#include "node_provider.h"
#elif HAVE_ETW
#include "node_dtrace.h"
#include <string.h>
#include "node_win32_etw_provider.h"
#include "node_win32_etw_provider-inl.h"
#elif HAVE_SYSTEMTAP
#include <string.h>
#include <node.h>
#include <v8.h>
#include <sys/sdt.h>
#include "node_systemtap.h"
#include "node_dtrace.h"
#else
#define NODE_HTTP_SERVER_REQUEST(arg0, arg1)
#define NODE_HTTP_SERVER_REQUEST_ENABLED() (0)
#define NODE_HTTP_SERVER_RESPONSE(arg0)
#define NODE_HTTP_SERVER_RESPONSE_ENABLED() (0)
#define NODE_HTTP_CLIENT_REQUEST(arg0, arg1)
#define NODE_HTTP_CLIENT_REQUEST_ENABLED() (0)
#define NODE_HTTP_CLIENT_RESPONSE(arg0)
#define NODE_HTTP_CLIENT_RESPONSE_ENABLED() (0)
#define NODE_NET_SERVER_CONNECTION(arg0)
#define NODE_NET_SERVER_CONNECTION_ENABLED() (0)
#define NODE_NET_STREAM_END(arg0)
#define NODE_NET_STREAM_END_ENABLED() (0)
#define NODE_NET_SOCKET_READ(arg0, arg1)
#define NODE_NET_SOCKET_READ_ENABLED() (0)
#define NODE_NET_SOCKET_WRITE(arg0, arg1)
#define NODE_NET_SOCKET_WRITE_ENABLED() (0)
#define NODE_GC_START(arg0, arg1)
#define NODE_GC_DONE(arg0, arg1)
#endif

namespace node {

using namespace v8;

#define SLURP_STRING(obj, member, valp) \
  if (!(obj)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "object for " #obj " to contain string member " #member)))); \
  } \
  String::Utf8Value _##member(obj->Get(String::New(#member))); \
  if ((*(const char **)valp = *_##member) == NULL) \
    *(const char **)valp = "<unknown>";

#define SLURP_INT(obj, member, valp) \
  if (!(obj)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "object for " #obj " to contain integer member " #member)))); \
  } \
  *valp = obj->Get(String::New(#member))->ToInteger()->Value();

#define SLURP_OBJECT(obj, member, valp) \
  if (!(obj)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "object for " #obj " to contain object member " #member)))); \
  } \
  *valp = Local<Object>::Cast(obj->Get(String::New(#member)));

#define SLURP_CONNECTION(arg, conn) \
  if (!(arg)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "argument " #arg " to be a connection object")))); \
  } \
  node_dtrace_connection_t conn; \
  Local<Object> _##conn = Local<Object>::Cast(arg); \
  Local<Value> _handle = (_##conn)->Get(String::New("_handle")); \
  if (_handle->IsObject()) { \
    SLURP_INT(_handle.As<Object>(), fd, &conn.fd); \
  } else { \
    conn.fd = -1; \
  } \
  SLURP_STRING(_##conn, remoteAddress, &conn.remote); \
  SLURP_INT(_##conn, remotePort, &conn.port); \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);

#define SLURP_CONNECTION_HTTP_CLIENT(arg, conn) \
  if (!(arg)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "argument " #arg " to be a connection object")))); \
  } \
  node_dtrace_connection_t conn; \
  Local<Object> _##conn = Local<Object>::Cast(arg); \
  SLURP_INT(_##conn, fd, &conn.fd); \
  SLURP_STRING(_##conn, host, &conn.remote); \
  SLURP_INT(_##conn, port, &conn.port); \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);

#define SLURP_CONNECTION_HTTP_CLIENT_RESPONSE(arg0, arg1, conn) \
  if (!(arg0)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "argument " #arg0 " to be a connection object")))); \
  } \
  if (!(arg1)->IsObject()) { \
    return (ThrowException(Exception::Error(String::New("expected " \
      "argument " #arg1 " to be a connection object")))); \
  } \
  node_dtrace_connection_t conn; \
  Local<Object> _##conn = Local<Object>::Cast(arg0); \
  SLURP_INT(_##conn, fd, &conn.fd); \
  SLURP_INT(_##conn, bufferSize, &conn.buffered); \
  _##conn = Local<Object>::Cast(arg1); \
  SLURP_STRING(_##conn, host, &conn.remote); \
  SLURP_INT(_##conn, port, &conn.port);


Handle<Value> DTRACE_NET_SERVER_CONNECTION(const Arguments& args) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_SERVER_CONNECTION_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  SLURP_CONNECTION(args[0], conn);
#ifdef HAVE_SYSTEMTAP
  NODE_NET_SERVER_CONNECTION(conn.fd, conn.remote, conn.port, \
                             conn.buffered);
#else
  NODE_NET_SERVER_CONNECTION(&conn, conn.remote, conn.port, conn.fd);
#endif

  return Undefined();
}

Handle<Value> DTRACE_NET_STREAM_END(const Arguments& args) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_STREAM_END_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  SLURP_CONNECTION(args[0], conn);
#ifdef HAVE_SYSTEMTAP
  NODE_NET_STREAM_END(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_NET_STREAM_END(&conn, conn.remote, conn.port, conn.fd);
#endif

  return Undefined();
}

Handle<Value> DTRACE_NET_SOCKET_READ(const Arguments& args) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_SOCKET_READ_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  SLURP_CONNECTION(args[0], conn);

#ifdef HAVE_SYSTEMTAP
  NODE_NET_SOCKET_READ(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  if (!args[1]->IsNumber()) {
    return (ThrowException(Exception::Error(String::New("expected " 
      "argument 1 to be number of bytes"))));
  }
  int nbytes = args[1]->Int32Value();
  NODE_NET_SOCKET_READ(&conn, nbytes, conn.remote, conn.port, conn.fd);
#endif

  return Undefined();
}

Handle<Value> DTRACE_NET_SOCKET_WRITE(const Arguments& args) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_SOCKET_WRITE_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  SLURP_CONNECTION(args[0], conn);

#ifdef HAVE_SYSTEMTAP
  NODE_NET_SOCKET_WRITE(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  if (!args[1]->IsNumber()) {
    return (ThrowException(Exception::Error(String::New("expected " 
      "argument 1 to be number of bytes"))));
  }
  int nbytes = args[1]->Int32Value();
  NODE_NET_SOCKET_WRITE(&conn, nbytes, conn.remote, conn.port, conn.fd);
#endif

  return Undefined();
}

Handle<Value> DTRACE_HTTP_SERVER_REQUEST(const Arguments& args) {
  node_dtrace_http_server_request_t req;

#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_SERVER_REQUEST_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  Local<Object> arg0 = Local<Object>::Cast(args[0]);
  Local<Object> headers;

  memset(&req, 0, sizeof(req));
  req._un.version = 1;
  SLURP_STRING(arg0, url, &req.url);
  SLURP_STRING(arg0, method, &req.method);

  SLURP_OBJECT(arg0, headers, &headers);

  if (!(headers)->IsObject())
    return (ThrowException(Exception::Error(String::New("expected "
      "object for request to contain string member headers"))));

  Local<Value> strfwdfor = headers->Get(String::New("x-forwarded-for"));
  String::Utf8Value fwdfor(strfwdfor);

  if (!strfwdfor->IsString() || (req.forwardedFor = *fwdfor) == NULL)
    req.forwardedFor = const_cast<char*>("");

  SLURP_CONNECTION(args[1], conn);

#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_SERVER_REQUEST(&req, conn.fd, conn.remote, conn.port, \
                           conn.buffered);
#else
  NODE_HTTP_SERVER_REQUEST(&req, &conn, conn.remote, conn.port, req.method, \
                           req.url, conn.fd);
#endif
  return Undefined();
}

Handle<Value> DTRACE_HTTP_SERVER_RESPONSE(const Arguments& args) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_SERVER_RESPONSE_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  SLURP_CONNECTION(args[0], conn);
#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_SERVER_RESPONSE(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_HTTP_SERVER_RESPONSE(&conn, conn.remote, conn.port, conn.fd);
#endif

  return Undefined();
}

Handle<Value> DTRACE_HTTP_CLIENT_REQUEST(const Arguments& args) {
  node_dtrace_http_client_request_t req;
  char *header;

#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_CLIENT_REQUEST_ENABLED())
    return Undefined();
#endif

  HandleScope scope;

  /*
   * For the method and URL, we're going to dig them out of the header.  This
   * is not as efficient as it could be, but we would rather not force the
   * caller here to retain their method and URL until the time at which
   * DTRACE_HTTP_CLIENT_REQUEST can be called.
   */
  Local<Object> arg0 = Local<Object>::Cast(args[0]);
  SLURP_STRING(arg0, _header, &header);

  req.method = header;

  while (*header != '\0' && *header != ' ')
    header++;

  if (*header != '\0')
    *header++ = '\0';

  req.url = header;

  while (*header != '\0' && *header != ' ')
    header++;

  *header = '\0';

  SLURP_CONNECTION_HTTP_CLIENT(args[1], conn);
#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_CLIENT_REQUEST(&req, conn.fd, conn.remote, conn.port, \
                           conn.buffered);
#else
  NODE_HTTP_CLIENT_REQUEST(&req, &conn, conn.remote, conn.port, req.method, \
                           req.url, conn.fd);
#endif
  return Undefined();
}

Handle<Value> DTRACE_HTTP_CLIENT_RESPONSE(const Arguments& args) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_CLIENT_RESPONSE_ENABLED())
    return Undefined();
#endif
  HandleScope scope;

  SLURP_CONNECTION_HTTP_CLIENT_RESPONSE(args[0], args[1], conn);
#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_CLIENT_RESPONSE(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_HTTP_CLIENT_RESPONSE(&conn, conn.remote, conn.port, conn.fd);
#endif

  return Undefined();
}

#define NODE_PROBE(name) #name, name, Persistent<FunctionTemplate>()

static int dtrace_gc_start(GCType type, GCCallbackFlags flags) {
#ifdef HAVE_SYSTEMTAP
  NODE_GC_START();
#else
  NODE_GC_START(type, flags);
#endif
  /*
   * We avoid the tail-call elimination of the USDT probe (which screws up
   * args) by forcing a return of 0.
   */
  return 0;
}

static int dtrace_gc_done(GCType type, GCCallbackFlags flags) {
#ifdef HAVE_SYSTEMTAP
  NODE_GC_DONE();
#else
  NODE_GC_DONE(type, flags);
#endif
  return 0;
}

void InitDTrace(Handle<Object> target) {
  static struct {
    const char *name;
    Handle<Value> (*func)(const Arguments&);
    Persistent<FunctionTemplate> templ;
  } tab[] = {
    { NODE_PROBE(DTRACE_NET_SERVER_CONNECTION) },
    { NODE_PROBE(DTRACE_NET_STREAM_END) },
    { NODE_PROBE(DTRACE_NET_SOCKET_READ) },
    { NODE_PROBE(DTRACE_NET_SOCKET_WRITE) },
    { NODE_PROBE(DTRACE_HTTP_SERVER_REQUEST) },
    { NODE_PROBE(DTRACE_HTTP_SERVER_RESPONSE) },
    { NODE_PROBE(DTRACE_HTTP_CLIENT_REQUEST) },
    { NODE_PROBE(DTRACE_HTTP_CLIENT_RESPONSE) }
  };

  for (unsigned int i = 0; i < ARRAY_SIZE(tab); i++) {
    tab[i].templ = Persistent<FunctionTemplate>::New(
        FunctionTemplate::New(tab[i].func));
    target->Set(String::NewSymbol(tab[i].name), tab[i].templ->GetFunction());
  }

#ifdef HAVE_ETW
  init_etw();
#endif

#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
  v8::V8::AddGCPrologueCallback((GCPrologueCallback)dtrace_gc_start);
  v8::V8::AddGCEpilogueCallback((GCEpilogueCallback)dtrace_gc_done);
#endif
}

}

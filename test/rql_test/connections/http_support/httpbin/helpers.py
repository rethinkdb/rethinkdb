# -*- coding: utf-8 -*-

"""
httpbin.helpers
~~~~~~~~~~~~~~~

This module provides helper functions for httpbin.
"""

import json
import base64
from hashlib import md5
from werkzeug.http import parse_authorization_header

from flask import request, make_response

try:
    from urlparse import urlparse, urlunparse
except ImportError:
    from urllib.parse import urlparse, urlunparse


from .structures import CaseInsensitiveDict


ASCII_ART = """
    -=[ teapot ]=-

       _...._
     .'  _ _ `.
    | ."` ^ `". _,
    \_;`"---"`|//
      |       ;/
      \_     _/
        `\"\"\"`
"""

REDIRECT_LOCATION = '/redirect/1'

ENV_HEADERS = (
    'X-Varnish',
    'X-Request-Start',
    'X-Heroku-Queue-Depth',
    'X-Real-Ip',
    'X-Forwarded-Proto',
    'X-Heroku-Queue-Wait-Time',
    'X-Forwarded-For',
    'X-Heroku-Dynos-In-Use',
    'X-Forwarded-For',
    'X-Forwarded-Protocol',
    'X-Forwarded-Port'
)

ROBOT_TXT = """User-agent: *
Disallow: /deny
"""

ANGRY_ASCII ="""
          .-''''''-.
        .' _      _ '.
       /   O      O   \\
      :                :
      |                |
      :       __       :
       \  .-"`  `"-.  /
        '.          .'
          '-......-'
      YOU SHOUDN'T BE HERE
"""


def json_safe(string, content_type='application/octet-stream'):
    """Returns JSON-safe version of `string`.

    If `string` is a Unicode string or a valid UTF-8, it is returned unmodified,
    as it can safely be encoded to JSON string.

    If `string` contains raw/binary data, it is Base64-encoded, formatted and
    returned according to "data" URL scheme (RFC2397). Since JSON is not
    suitable for binary data, some additional encoding was necessary; "data"
    URL scheme was chosen for its simplicity.
    """
    try:
        string = string.decode('utf-8')
        _encoded = json.dumps(string)
        return string
    except (ValueError, TypeError):
        return b''.join([
            b'data:',
            content_type.encode('utf-8'),
            b';base64,',
            base64.b64encode(string)
        ]).decode('utf-8')


def get_files():
    """Returns files dict from request context."""

    files = dict()

    for k, v in request.files.items():
        content_type = request.files[k].content_type or 'application/octet-stream'
        val = json_safe(v.read(), content_type)
        if files.get(k):
            if not isinstance(files[k], list):
                files[k] = [files[k]]
            files[k].append(val)
        else:
            files[k] = val

    return files


def get_headers(hide_env=True):
    """Returns headers dict from request context."""

    headers = dict(request.headers.items())

    if hide_env and ('show_env' not in request.args):
        for key in ENV_HEADERS:
            try:
                del headers[key]
            except KeyError:
                pass

    return CaseInsensitiveDict(headers.items())


def semiflatten(multi):
    """Convert a MutiDict into a regular dict. If there are more than one value
    for a key, the result will have a list of values for the key. Otherwise it
    will have the plain value."""
    if multi:
        result = multi.to_dict(flat=False)
        for k, v in result.items():
            if len(v) == 1:
                result[k] = v[0]
        return result
    else:
        return multi

def get_url(request):
    """
    Since we might be hosted behind a proxy, we need to check the
    X-Forwarded-Proto header to find out what protocol was used to access us.
    """
    if 'X-Forwarded-Proto' not in request.headers:
        return request.url
    url = list(urlparse(request.url))
    url[0] = request.headers.get('X-Forwarded-Proto')
    return urlunparse(url)


def get_dict(*keys, **extras):
    """Returns request dict of given keys."""

    _keys = ('url', 'args', 'form', 'data', 'origin', 'headers', 'files', 'json')

    assert all(map(_keys.__contains__, keys))
    data = request.data
    form = request.form
    form = semiflatten(request.form)

    try:
        _json = json.loads(data.decode('utf-8'))
    except (ValueError, TypeError):
        _json = None

    d = dict(
        url=get_url(request),
        args=semiflatten(request.args),
        form=form,
        data=json_safe(data),
        origin=request.headers.get('X-Forwarded-For', request.remote_addr),
        headers=get_headers(),
        files=get_files(),
        json=_json
    )

    out_d = dict()

    for key in keys:
        out_d[key] = d.get(key)

    out_d.update(extras)

    return out_d


def status_code(code):
    """Returns response object of given status code."""

    redirect = dict(headers=dict(location=REDIRECT_LOCATION))

    code_map = {
        301: redirect,
        302: redirect,
        303: redirect,
        304: dict(data=''),
        305: redirect,
        307: redirect,
        401: dict(headers={'WWW-Authenticate': 'Basic realm="Fake Realm"'}),
        402: dict(
            data='Fuck you, pay me!',
            headers={
                'x-more-info': 'http://vimeo.com/22053820'
            }
        ),
        407: dict(headers={'Proxy-Authenticate': 'Basic realm="Fake Realm"'}),
        418: dict(  # I'm a teapot!
            data=ASCII_ART,
            headers={
                'x-more-info': 'http://tools.ietf.org/html/rfc2324'
            }
        ),

    }

    r = make_response()
    r.status_code = code

    if code in code_map:

        m = code_map[code]

        if 'data' in m:
            r.data = m['data']
        if 'headers' in m:
            r.headers = m['headers']

    return r


def check_basic_auth(user, passwd):
    """Checks user authentication using HTTP Basic Auth."""

    auth = request.authorization
    return auth and auth.username == user and auth.password == passwd



# Digest auth helpers
# qop is a quality of protection

def H(data):
    return md5(data).hexdigest()


def HA1(realm, username, password):
    """Create HA1 hash by realm, username, password

    HA1 = md5(A1) = MD5(username:realm:password)
    """
    if not realm:
        realm = u''
    return H(b":".join([username.encode('utf-8'),
                           realm.encode('utf-8'),
                           password.encode('utf-8')]))


def HA2(credentails, request):
    """Create HA2 md5 hash

    If the qop directive's value is "auth" or is unspecified, then HA2:
        HA2 = md5(A2) = MD5(method:digestURI)
    If the qop directive's value is "auth-int" , then HA2 is
        HA2 = md5(A2) = MD5(method:digestURI:MD5(entityBody))
    """
    if credentails.get("qop") == "auth" or credentails.get('qop') is None:
        return H(b":".join([request['method'].encode('utf-8'), request['uri'].encode('utf-8')]))
    elif credentails.get("qop") == "auth-int":
        for k in 'method', 'uri', 'body':
            if k not in request:
                raise ValueError("%s required" % k)
        return H("%s:%s:%s" % (request['method'],
                               request['uri'],
                               H(request['body'])))
    raise ValueError


def response(credentails, user, password, request):
    """Compile digest auth response

    If the qop directive's value is "auth" or "auth-int" , then compute the response as follows:
       RESPONSE = MD5(HA1:nonce:nonceCount:clienNonce:qop:HA2)
    Else if the qop directive is unspecified, then compute the response as follows:
       RESPONSE = MD5(HA1:nonce:HA2)

    Arguments:
    - `credentails`: credentails dict
    - `user`: request user name
    - `password`: request user password
    - `request`: request dict
    """
    for key in 'nonce', 'realm':
        if key not in credentails:
            raise ValueError("%s required for response" % key)
    response = None
    HA1_value = HA1(
        credentails.get('realm'),
        user,
        password
    )
    HA2_value = HA2(credentails, request)
    if credentails.get('qop') is None:
        response = H(b":".join([
            HA1_value.encode('utf-8'), 
            credentails.get('nonce').encode('utf-8'), 
            HA2_value.encode('utf-8')
        ]))
    elif credentails.get('qop') == 'auth' or credentails.get('qop') == 'auth-int':
        for k in 'nonce', 'nc', 'cnonce', 'qop':
            if k not in credentails:
                raise ValueError("%s required for response H" % k)
        response = H(b":".join([HA1_value.encode('utf-8'),
                               credentails.get('nonce').encode('utf-8'),
                               credentails.get('nc').encode('utf-8'),
                               credentails.get('cnonce').encode('utf-8'),
                               credentails.get('qop').encode('utf-8'),
                               HA2_value.encode('utf-8')]))
    else:
        raise ValueError("qop value are wrong")

    return response


def check_digest_auth(user, passwd):
    """Check user authentication using HTTP Digest auth"""

    if request.headers.get('Authorization'):
        credentails = parse_authorization_header(request.headers.get('Authorization'))
        if not credentails:
            return False
        response_hash = response(credentails, user, passwd, dict(uri=request.path,
                                                           body=request.data,
                                                           method=request.method))
        if credentails.get('response') == response_hash:
            return True
    return False

def secure_cookie():
    """Return true if cookie should have secure attribute"""
    return request.environ['wsgi.url_scheme'] == 'https'

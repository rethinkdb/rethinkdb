#!/usr/bin/env python
###
# Tests the http term
###

import sys, os, datetime
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'drivers', 'python')))
import rethinkdb as r

port = int(sys.argv[1])
conn = r.connect('localhost', port)

def expect_error(query, err_type, err_info):
    try:
        res = query.run(conn)
    except err_type as ex:
        if ex.message != err_info:
            raise
        return
    raise RuntimeError("Expected an error, but got success with result: %s" % str(res))

def expect_eq(left, right):
    if left != right:
        raise RuntimeError("Actual result not equal to expected result:" +
                           "\n  ACTUAL: %s" % str(left) +
                           "\n  EXPECTED: %s" % str(right))

def err_string(method, url, msg):
    return 'Error in HTTP %s of `%s`: %s.' % (method, url, msg)

def test_get():
    url = 'httpbin.org/get'

    res = r.http(url).run(conn)
    expect_eq(res['args'], {})
    expect_eq(res['headers']['Accept-Encoding'], 'deflate=1;gzip=0.5')
    expect_eq(res['headers']['User-Agent'].split('/')[0], 'RethinkDB')

def test_params():
    url = 'httpbin.org/get'

    res = r.http(url, params={'fake':123,'things':'stuff','nil':None}).run(conn)
    expect_eq(res['args']['fake'], '123')
    expect_eq(res['args']['things'], 'stuff')
    expect_eq(res['args']['nil'], '')

    res = r.http(url + '?dummy=true', params={'fake':123}).run(conn)
    expect_eq(res['args']['fake'], '123')
    expect_eq(res['args']['dummy'], 'true')

def test_headers():
    url = 'httpbin.org/headers'

    res = r.http(url, header={'Test':'entry','Accept-Encoding':'override'}).run(conn)
    expect_eq(res['headers']['Test'], 'entry')
    expect_eq(res['headers']['Accept-Encoding'], 'override')

    res = r.http(url, header=['Test: entry','Accept-Encoding: override']).run(conn)
    expect_eq(res['headers']['Test'], 'entry')
    expect_eq(res['headers']['Accept-Encoding'], 'override')

def test_head():
    url = 'httpbin.org/get'

    res = r.http(url, method='HEAD', result_format='text').run(conn)
    expect_eq(res, '')

    res = r.http(url, method='HEAD').run(conn)
    expect_eq(res, None)

def test_post():
    url = 'httpbin.org/post'
    post_data = {'str':'%in fo+','number':135.5,'nil':None}
    res = r.http(url, method='POST', data=post_data).run(conn)
    post_data['number'] = str(post_data['number'])
    post_data['nil'] = ''
    expect_eq(res['form'], post_data)

    post_data = {'str':'%in fo+','number':135.5,'nil':None}
    res = r.http(url, method='POST', data=r.expr(post_data).coerce_to('string'),
                 header={'Content-Type':'application/json'}).run(conn)
    expect_eq(res['json'], post_data)

    res = r.http(url, method='POST', data=r.expr(post_data).coerce_to('string')).run(conn)
    post_data['str'] = '%in fo ' # Default content type is x-www-form-encoded, which changes the '+' to a space
    expect_eq(res['json'], post_data)

    post_data = 'a=b&b=c'
    res = r.http(url, method='POST', data=post_data).run(conn)
    expect_eq(res['form'], {'a':'b','b':'c'})

    post_data = '<arbitrary>data</arbitrary>'
    res = r.http(url, method='POST', data=post_data).run(conn)
    expect_eq(res['data'], post_data)

def test_put():
    url = 'httpbin.org/put'
    put_data = {'nested':{'arr':[123.45, ['a', 555], 0.123], 'str':'info','number':135,'nil':None},'time':r.epoch_time(1000)}
    res = r.http(url, method='PUT', data=put_data).run(conn)
    expect_eq(res['json']['nested'], put_data['nested'])
    expect_eq(res['json']['time'], datetime.datetime(1970, 1, 1, 0, 16, 40, tzinfo=res['json']['time'].tzinfo))

    put_data = '<arbitrary> +%data!$%^</arbitrary>'
    res = r.http(url, method='PUT', data=put_data).run(conn)
    expect_eq(res['data'], put_data)

def test_patch():
    url = 'httpbin.org/patch'
    patch_data = {'nested':{'arr':[123.45, ['a', 555], 0.123], 'str':'info','number':135},'time':r.epoch_time(1000),'nil':None}
    res = r.http(url, method='PATCH', data=patch_data).run(conn)
    expect_eq(res['json']['nested'], patch_data['nested'])
    expect_eq(res['json']['time'], datetime.datetime(1970, 1, 1, 0, 16, 40, tzinfo=res['json']['time'].tzinfo))

    patch_data = '<arbitrary> +%data!$%^</arbitrary>'
    res = r.http(url, method='PATCH', data=patch_data).run(conn)
    expect_eq(res['data'], patch_data)

def test_delete():
    url = 'httpbin.org/delete'
    delete_data = {'nested':{'arr':[123.45, ['a', 555], 0.123], 'str':'info','number':135},'time':r.epoch_time(1000),'nil':None}
    res = r.http(url, method='DELETE', data=delete_data).run(conn)
    expect_eq(res['json']['nested'], delete_data['nested'])
    expect_eq(res['json']['time'], datetime.datetime(1970, 1, 1, 0, 16, 40, tzinfo=res['json']['time'].tzinfo))

    delete_data = '<arbitrary> +%data!$%^</arbitrary>'
    res = r.http(url, method='DELETE', data=delete_data).run(conn)
    expect_eq(res['data'], delete_data)

def test_redirects():
    url = 'httpbin.org/redirect/2'
    expect_error(r.http(url),
                 r.RqlRuntimeError, err_string('GET', url, 'status code 302'))
    expect_error(r.http(url, redirects=1),
                 r.RqlRuntimeError, err_string('GET', url, 'Number of redirects hit maximum amount'))
    res = r.http(url, redirects=2).run(conn)
    expect_eq(res['headers']['Host'], 'httpbin.org')

def test_gzip():
    res = r.http('httpbin.org/gzip').run(conn)
    expect_eq(res['gzipped'], True)

def test_failed_json_parse():
    url = 'httpbin.org/html'
    expect_error(r.http(url, result_format='json'),
                 r.RqlRuntimeError, err_string('GET', url, 'failed to parse JSON response'))

def test_basic_auth():
    url = 'http://httpbin.org/basic-auth/azure/hunter2'

    # Wrong password
    expect_error(r.http(url, auth={'type':'basic','user':'azure','pass':'wrong'}),
                 r.RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Wrong username
    expect_error(r.http(url, auth={'type':'basic','user':'fake','pass':'hunter2'}),
                 r.RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Wrong authentication type
    expect_error(r.http(url, auth={'type':'digest','user':'azure','pass':'hunter2'}),
                 r.RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Correct credentials
    res = r.http(url, auth={'type':'basic','user':'azure','pass':'hunter2'}).run(conn)
    expect_eq(res, {'authenticated': True, 'user': 'azure'})

    # Default auth type should be basic
    res = r.http(url, auth={'user':'azure','pass':'hunter2'}).run(conn)
    expect_eq(res, {'authenticated': True, 'user': 'azure'})

# This test requires us to set a cookie (any cookie) due to a bug in httpbin.org
# See https://github.com/kennethreitz/httpbin/issues/124
def test_digest_auth():
    url = 'http://httpbin.org/digest-auth/auth/azure/hunter2'

    # Wrong password
    expect_error(r.http(url, header={'Cookie':'dummy'}, redirects=5,
                        auth={'type':'digest','user':'azure','pass':'wrong'}),
                 r.RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # httpbin apparently doesn't check the username, just the password
    # Wrong username
    #expect_error(r.http(url, header={'Cookie':'dummy'}, redirects=5,
    #                    auth={'type':'digest','user':'fake','pass':'hunter2'}),
    #             r.RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # httpbin has a 500 error on this
    # Wrong authentication type
    #expect_error(r.http(url, header={'Cookie':'dummy'}, redirects=5,
    #                    auth={'type':'basic','user':'azure','pass':'hunter2'}),
    #             r.RqlRuntimeError, err_string('GET', url, 'status code 401'))

    # Correct credentials
    res = r.http(url, header={'Cookie':'dummy'}, redirects=5,
                 auth={'type':'digest','user':'azure','pass':'hunter2'}).run(conn)
    expect_eq(res, {'authenticated': True, 'user': 'azure'})

def test_verify():
    def test_part(url):
        expect_error(r.http(url, verify=True, redirects=5),
                     r.RqlRuntimeError, err_string('GET', url, 'Peer certificate cannot be authenticated with given CA certificates'))

        res = r.http(url, verify=False, redirects=5).split()[0].run(conn)
        expect_eq(res, '<html>')

    test_part('http://dev.rethinkdb.com')
    test_part('https://dev.rethinkdb.com')

def main():
    tests = {
             'verify': test_verify,
             'get': test_get,
             'head': test_head,
             'params': test_params,
             'headers': test_headers,
             'post': test_post,
             'put': test_put,
             'patch': test_patch,
             'delete': test_delete,
             'redirects': test_redirects,
             'gzip': test_gzip,
             'failed_json_parse': test_failed_json_parse,
             'digest_auth': test_digest_auth,
             'basic_auth': test_basic_auth
             }

    # TODO: try/catch, print errors and continue?
    for name, fn in tests.iteritems():
        print 'Running test: %s' % name
        fn()
        print ' - PASS'


if __name__ == '__main__':
    exit(main())

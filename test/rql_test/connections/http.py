#!/usr/bin/env python

'''Tests the http term'''

import datetime, os, re, subprocess, sys, tempfile, time, unittest

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir, 'common'))
import driver, utils

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))
import http_support

r = utils.import_python_driver()

class WithServer(unittest.TestCase):
    
    host = 'localhost'
    
    targetServer = None
    
    conn = None
    _server = None
    _server_log = None
    
    def setUp(self):
        
        # -- startup local httpbin server
        
        if self.targetServer is None or self.targetServer.poll() is not None:
             self.targetServer = http_support.HttpTargetServer()
        
        # -- startup the RethinkDB server
        
        if self.conn == None:
            try:
                self._server.driver_port = int(os.getenv('RDB_DRIVER_PORT') or sys.argv[1])
            except Exception:
                self.__class__._server_log = tempfile.NamedTemporaryFile('w+')
                self.__class__._server = driver.Process(console_output=self._server_log.name)
                self.__class__.conn = self._server.driver_port
            
            self.__class__.conn = r.connect('localhost', self._server.driver_port)
        
        # -- patch the Python2.6 version of unittest to have assertRaisesRegex
        
        if not hasattr(self, 'assertRaisesRegex'):
            def assertRaisesRegex_replacement(self, exception, regexp, function, *args, **kwds):
                result = None
                try:
                    result = function(*args, **kwds)
                except Exception as e:
                    if not isinstance(e, exception):
                        raise AssertionError('Got the wrong type of exception: %s vs. expected: %s' % (e.__class__.__name__, exception.__name__))
                    if not re.match(regexp, str(e)):
                        raise AssertionError('Error message: "%s" does not match "%s"' % (str(regexp), str(e)))
                    return
                else:
                    raise AssertionError('%s not raised for: %s, rather got: %s' % (exception.__name__, repr(function), repr(result)))
            self.__class__.assertRaisesRegex = assertRaisesRegex_replacement
    
    def err_string(self, method, url, msg):
        return 'Error in HTTP %s of `%s`: %s' % (method, url, msg)

class TestHttpTerm(WithServer):
    
    def getHttpBinURL(self, *path):
        if len(path) == 0:
            path = ['get']
        return 'http://%s:%d/%s' % (self.host, self.targetServer.httpbinPort, '/'.join(path))
    
    # =============
    
    def test_get(self):
        url = self.getHttpBinURL('get')
        
        res = r.http(url).run(self.conn)
        self.assertEqual(res['args'], {})
        self.assertEqual(res['headers']['Accept-Encoding'], 'deflate;q=1, gzip;q=0.5')
        self.assertEqual(res['headers']['User-Agent'].split('/')[0], 'RethinkDB')
    
    def test_params(self):
        url = self.getHttpBinURL('get')
        
        res = r.http(url, params={'fake':123,'things':'stuff','nil':None}).run(self.conn)
        self.assertEqual(res['args']['fake'], '123')
        self.assertEqual(res['args']['things'], 'stuff')
        self.assertEqual(res['args']['nil'], '')
        
        res = r.http(url + '?dummy=true', params={'fake':123}).run(self.conn)
        self.assertEqual(res['args']['fake'], '123')
        self.assertEqual(res['args']['dummy'], 'true')
    
    def test_headers(self):
        url = self.getHttpBinURL('headers')
        
        res = r.http(url, header={'Test':'entry','Accept-Encoding':'override'}).run(self.conn)
        self.assertEqual(res['headers']['Test'], 'entry')
        self.assertEqual(res['headers']['Accept-Encoding'], 'override')
        
        res = r.http(url, header=['Test: entry','Accept-Encoding: override']).run(self.conn)
        self.assertEqual(res['headers']['Test'], 'entry')
        self.assertEqual(res['headers']['Accept-Encoding'], 'override')
    
    def test_head(self):
        url = self.getHttpBinURL('get')
        
        res = r.http(url, method='HEAD', result_format='text').run(self.conn)
        self.assertEqual(res, None)
        
        res = r.http(url, method='HEAD').run(self.conn)
        self.assertEqual(res, None)
    
    def test_post(self):
        url = self.getHttpBinURL('post')
        
        post_data = {'str':'%in fo+','number':135.5,'nil':None}
        res = r.http(url, method='POST', data=post_data).run(self.conn)
        post_data['number'] = str(post_data['number'])
        post_data['nil'] = ''
        self.assertEqual(res['form'], post_data)
        
        post_data = {'str':'%in fo+','number':135.5,'nil':None}
        res = r.http(url, method='POST', data=r.expr(post_data).coerce_to('string'), header={'Content-Type':'application/json'}).run(self.conn)
        self.assertEqual(res['json'], post_data)
        
        post_data = 'a=b&b=c'
        res = r.http(url, method='POST', data=post_data).run(self.conn)
        self.assertEqual(res['form'], {'a':'b','b':'c'})
        
        post_data = '<arbitrary>data</arbitrary>'
        res = r.http(url, method='POST', data=post_data, header={'Content-Type':'application/text/'}).run(self.conn)
        self.assertEqual(res['data'], post_data)
    
    def test_put(self):
        url = self.getHttpBinURL('put')
        
        put_data = {'nested':{'arr':[123.45, ['a', 555], 0.123], 'str':'info','number':135,'nil':None},'time':r.epoch_time(1000)}
        res = r.http(url, method='PUT', data=put_data).run(self.conn)
        self.assertEqual(res['json']['nested'], put_data['nested'])
        self.assertEqual(res['json']['time'], datetime.datetime(1970, 1, 1, 0, 16, 40, tzinfo=res['json']['time'].tzinfo))
        
        put_data = '<arbitrary> +%data!$%^</arbitrary>'
        res = r.http(url, method='PUT', data=put_data).run(self.conn)
        self.assertEqual(res['data'], put_data)
    
    def test_patch(self):
        url = self.getHttpBinURL('patch')
        
        patch_data = {'nested':{'arr':[123.45, ['a', 555], 0.123], 'str':'info','number':135},'time':r.epoch_time(1000),'nil':None}
        res = r.http(url, method='PATCH', data=patch_data).run(self.conn)
        self.assertEqual(res['json']['nested'], patch_data['nested'])
        self.assertEqual(res['json']['time'], datetime.datetime(1970, 1, 1, 0, 16, 40, tzinfo=res['json']['time'].tzinfo))
        
        patch_data = '<arbitrary> +%data!$%^</arbitrary>'
        res = r.http(url, method='PATCH', data=patch_data).run(self.conn)
        self.assertEqual(res['data'], patch_data)
    
    def test_delete(self):
        url = self.getHttpBinURL('delete')
        
        delete_data = {'nested':{'arr':[123.45, ['a', 555], 0.123], 'str':'info','number':135},'time':r.epoch_time(1000),'nil':None}
        res = r.http(url, method='DELETE', data=delete_data).run(self.conn)
        self.assertEqual(res['json']['nested'], delete_data['nested'])
        self.assertEqual(res['json']['time'], datetime.datetime(1970, 1, 1, 0, 16, 40, tzinfo=res['json']['time'].tzinfo))
        
        delete_data = '<arbitrary> +%data!$%^</arbitrary>'
        res = r.http(url, method='DELETE', data=delete_data).run(self.conn)
        self.assertEqual(res['data'], delete_data)
    
    def test_redirects(self):
        url = self.getHttpBinURL('redirect', '2')
        
        self.assertRaisesRegex(r.RqlRuntimeError, self.err_string('GET', url, 'status code 302'), r.http(url, redirects=0).run, self.conn)
        self.assertRaisesRegex(r.RqlRuntimeError, self.err_string('GET', url, 'Number of redirects hit maximum amount'), r.http(url, redirects=1).run, self.conn)
        res = r.http(url, redirects=2).run(self.conn)
        self.assertEqual(res['headers']['Host'], '%s:%d' % (self.host, self.targetServer.httpbinPort))
    
    def test_gzip(self):
        url = self.getHttpBinURL('gzip')
        
        res = r.http(url).run(self.conn)
        self.assertEqual(res['gzipped'], True)
    
    def test_failed_json_parse(self):
        url = self.getHttpBinURL('robots.txt')
        
        self.assertRaisesRegex(
            r.RqlRuntimeError, self.err_string('GET', url, 'failed to parse JSON response'),
            r.http(url, result_format='json').run, self.conn
        )
    
    def test_basic_auth(self):
        url = self.getHttpBinURL('basic-auth', 'azure', 'hunter2')
        
        # Wrong password
        self.assertRaisesRegex(
            r.RqlRuntimeError, self.err_string('GET', url, 'status code 401'), 
            r.http(url, auth={'type':'basic','user':'azure','pass':'wrong'}).run, self.conn
        )
                
        # Wrong username
        self.assertRaisesRegex(
            r.RqlRuntimeError, self.err_string('GET', url, 'status code 401'),
            r.http(url, auth={'type':'basic','user':'fake','pass':'hunter2'}).run, self.conn
        )
        
        # Wrong authentication type
        self.assertRaisesRegex(
            r.RqlRuntimeError, self.err_string('GET', url, 'status code 401'),
            r.http(url, auth={'type':'digest','user':'azure','pass':'hunter2'}).run, self.conn
        )
        
        # Correct credentials
        res = r.http(url, auth={'type':'basic','user':'azure','pass':'hunter2'}).run(self.conn)
        self.assertEqual(res, {'authenticated': True, 'user': 'azure'})
        
        # Default auth type should be basic
        res = r.http(url, auth={'user':'azure','pass':'hunter2'}).run(self.conn)
        self.assertEqual(res, {'authenticated': True, 'user': 'azure'})
    
    def test_digest_auth(self):
        url = self.getHttpBinURL('digest-auth', 'auth', 'azure', 'hunter2')
        
        # Wrong password
        self.assertRaisesRegex(
            r.RqlRuntimeError, self.err_string('GET', url, 'status code 401'),
            r.http(url, redirects=5, auth={'type':'digest','user':'azure','pass':'wrong'}).run, self.conn
        )
        
        # Wrong username
        self.assertRaisesRegex(
           r.RqlRuntimeError, self.err_string('GET', url, 'status code 401'),
           r.http(url, redirects=5, auth={'type':'digest','user':'fake','pass':'hunter2'}).run, self.conn
        )
        
        # Wrong authentication type
        self.assertRaisesRegex(
           r.RqlRuntimeError, self.err_string('GET', url, 'status code 401'),
           r.http(url, redirects=5, auth={'type':'basic','user':'azure','pass':'hunter2'}).run, self.conn
        )
        
        # Correct credentials
        res = r.http(url, redirects=5,auth={'type':'digest','user':'azure','pass':'hunter2'}).run(self.conn)
        self.assertEqual(res, {'authenticated': True, 'user': 'azure'})
    
    def test_file_protocol(self):
        '''file:// urls should be rejected'''
        url = 'file:///bad/path'
        self.assertRaisesRegex(
            r.errors.RqlRuntimeError,
            self.err_string('GET', url, 'Unsupported protocol.'),
            r.http(url).run, self.conn
        )
    
    def test_binary(self):
        res = r.http('http://%s:%d/quickstart.png' % (self.host, self.targetServer.httpPort)) \
           .do(lambda row: [row.type_of(), row.count().gt(0)]) \
           .run(self.conn)
        self.assertEqual(res, ['PTYPE<BINARY>', True])
    
        res = r.http(self.getHttpBinURL('get'), result_format='binary') \
           .do(lambda row: [row.type_of(), row.slice(0,1).coerce_to("string")]) \
           .run(self.conn)
        self.assertEqual(res, ['PTYPE<BINARY>', '{'])
    
    def bad_https_helper(self, url):
        self.assertRaisesRegex(
            r.RqlRuntimeError,
            self.err_string('HEAD', url, 'Peer certificate cannot be authenticated with given CA certificates'),
            r.http(url, method='HEAD', verify=True, redirects=5).run,
            self.conn
        )
        
        res = r.http(url, method='HEAD', verify=False, redirects=5).run(self.conn)
        self.assertEqual(res, None)
    
    def test_redirect_http_to_bad_https(self):
        self.bad_https_helper('http://%s:%d/redirect' % (self.host, self.targetServer.httpPort)) # 302 redirection to https port
    
    def test_bad_https(self):
        self.bad_https_helper('https://%s:%d' % (self.host, self.targetServer.sslPort))

if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0]])

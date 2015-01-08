#!/usr/bin/env python

__all__ = ['HttpTargetServer']

import atexit, datetime, os, re, subprocess, sys, tempfile, time, warnings

sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, os.pardir, os.pardir, "common"))
import utils

# --

def __runServer(httpbinPort=0, httpPort=0, sslPort=0):
    
    import twisted.internet
    import twisted.internet.ssl
    import twisted.web
    import twisted.web.static
    import twisted.web.wsgi
    
    sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))
    import httpbin
    
    # -- generate self signed SSL certificate
    
    privateKey = tempfile.NamedTemporaryFile(suffix='.pem')
    subprocess.check_call(['openssl', 'genrsa'], stdout=privateKey, stderr=subprocess.PIPE)
    privateKey.flush()
    
    certificate = tempfile.NamedTemporaryFile(suffix='.pem')
    certificateSettings = tempfile.NamedTemporaryFile(suffix='.pem', mode='w+')
    certificateSettings.write("US\nST=CA\nL=Mountain View\n.\n.\n.\n%s\n.\n" % datetime.datetime.now().strftime('%Y%m%d'))
    certificateSettings.seek(0)
    subprocess.check_call(['openssl', 'req', '-new', '-x509', '-key', privateKey.name, '-days', '1000'], stdin=certificateSettings, stdout=certificate, stderr=subprocess.PIPE)
    certificate.flush()
    
    def printStrtupInfo():
        sys.stdout.write('''Testing server is running
	httpbin running on:          %d http://localhost:%d
	http redirect to https:      %d http://localhost:%d/redirect
	ssl self-signed certificate: %d https://localhost:%d/quickstart.png
''' % (httpbinPort, httpbinPort, httpPort, httpPort, sslPort, sslPort))
        sys.stdout.flush()
    
    twisted.internet.reactor.callWhenRunning(printStrtupInfo) 
    
    # -- setup endpoints
    
    httpbinEndpoint = twisted.web.wsgi.WSGIResource(twisted.internet.reactor, twisted.internet.reactor.getThreadPool(), httpbin.app)
    
    class LocalContent(twisted.web.resource.Resource):
        
        def getChild(self, name, request):
            if name == '':
                return self
            else:
                return twisted.web.resource.Resource.getChild(self, name, request)
        
        def render_GET(self, request):
            request.setHeader("content-type", "text/plain")
            return "This is the message you came here to see! Woo! Hoo!"
    
    localContentInstance = LocalContent()
    
    # -- setup the listeners
    
    httpbinListner = twisted.internet.reactor.listenTCP(
        port=httpbinPort, # port
        factory=twisted.web.server.Site(httpbinEndpoint)
    )
    httpbinPort = httpbinListner.getHost().port
    
    redirectListener = twisted.internet.reactor.listenTCP(
        port=httpPort, # port
        factory=twisted.web.server.Site(localContentInstance)
    )
    httpPort = redirectListener.getHost().port
    
    sslListener = twisted.internet.reactor.listenSSL(
        port=sslPort, # port
        factory=twisted.web.server.Site(localContentInstance),
        contextFactory = twisted.internet.ssl.DefaultOpenSSLContextFactory(privateKey.name, certificate.name)
    )
    sslPort = sslListener.getHost().port
    
    # -- add the local content
    
    localContentInstance.putChild('quickstart.png', twisted.web.static.File(os.path.join(os.path.dirname(os.path.realpath(__file__)), 'quickstart.png')))
    localContentInstance.putChild('redirect',  twisted.web.util.Redirect('https://localhost:%d' % sslPort))
    
    # -- start the server
    
    twisted.internet.reactor.run()

# --

class HttpTargetServer(object):
    
    __serverProcess = None
    __serverOutput = None
    
    httpbinPort = None
    httpPort = None
    sslPort = None
    
    def __init__(self, httpbinPort=0, httpPort=0, sslPort=0, startupTimeout=5):
        '''Start a server, using subprocess to do it out-of-process'''
        
        # -- startup server
        
        runableFile = __file__.rstrip('c')
        
        self.__serverOutput = tempfile.NamedTemporaryFile(mode='w+')
        self.__serverProcess = subprocess.Popen([runableFile, '--httpbin-port', str(httpbinPort), '--http-port', str(httpPort), '--ssl-port', str(sslPort)], stdout=self.__serverOutput, preexec_fn=os.setpgrp)
        
        # -- read port numbers
        
        portRegex = re.compile('^\s+(?P<name>\w+).+:\s+(?P<port>\d+)\shttp\S+$')
        
        deadline = startupTimeout + time.time()
        serverLines = utils.nonblocking_readline(self.__serverOutput)
        while deadline > time.time():
            line = next(serverLines)
            if line is None:
                time.sleep(.1)
                continue
            parsedLine = portRegex.match(line)
            if parsedLine is not None:
                if parsedLine.group('name') == 'httpbin':
                    self.httpbinPort = int(parsedLine.group('port'))
                elif parsedLine.group('name') == 'http':
                    self.httpPort = int(parsedLine.group('port'))
                elif parsedLine.group('name') == 'ssl':
                    self.sslPort = int(parsedLine.group('port'))
            if all([self.httpbinPort, self.httpPort, self.sslPort]):
                utils.wait_for_port(self.httpPort, timeout=(deadline - time.time()))
                break
        else:
            raise Exception('Timed out waiting %.2f secs for the http server to start' % startupTimeout)
        
        # -- set an at-exit to make sure we shut ourselves down
        
        atexit.register(self.endServer)
    
    def checkOnServer(self):
        '''Check that the server is still running, throwing an error if it is not'''
        
        if self.__serverProcess.poll() is not None:
            self.__serverOutput.seek(0)
            output = self.__serverOutput.read().decode('utf-8')
            returnCode = self.__serverProcess.returncode
            self.endServer()
            raise Exception('http server died with signal %d. Output was:\n%s\n' % (returnCode, output))
        utils.wait_for_port(self.httpPort)
    
    def endServer(self):
        '''Shutdown the server'''
        
        try:
            if self.__serverProcess is not None and self.__serverProcess.poll() is None:
                utils.kill_process_group(self.__serverProcess.pid)
        except Exception: pass
        
        self.__serverProcess = None
        self.__serverOutput = None
        
        self.httpbinPort = None
        self.httpPort = None
        self.sslPort = None

if __name__ == '__main__':
    import optparse
    
    parser = optparse.OptionParser()
    parser.add_option('-b', '--httpbin-port', dest='httpbinPort', type='int', default=0)
    parser.add_option('-p', '--http-port', dest='httpPort', type='int', default=0)
    parser.add_option('-s', '--ssl-port', dest='sslPort', type='int',  default=0)
    
    options, args = parser.parse_args()
    __runServer(options.httpbinPort, options.httpPort, options.sslPort)
    
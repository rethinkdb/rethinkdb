import subprocess
import urllib2

def application(e, r):
  if e['PATH_INFO'] == '/set_password':
      q = {}
      for x in e['wsgi.input'].read(int(e['CONTENT_LENGTH'])).split('&', 1):
          a = x.split('=',1)
          if len(a) == 2:
              q[a[0]] = urllib2.unquote(a[1])
      if not q.has_key('password'):
          r('302 Redirect', [('Location', '/')])
          return []
      subprocess.call(['bash', '-c', 'nohup bash firstrun.sh "$0" &', q['password']])
      r('200 OK', [('Content-Type', 'text/html')])
      return [file('wait.html').read()]
  r('200 OK', [('Content-Type', 'text/html')])
  return [file('index.html').read()]

import subprocess
import urllib2

def application(e, r):
  if e['PATH_INFO'] == '/action/set_password':
      q = {}
      for x in e['wsgi.input'].read(int(e['CONTENT_LENGTH'])).split('&', 1):
          a = x.split('=',1)
          if len(a) == 2:
              q[a[0]] = urllib2.unquote(a[1])
      if not q.has_key('password'):
          r('302 Redirect', [('Location', '/settings.html')])
          return []
      subprocess.call(['bash', '-c', 'nohup bash firstrun.sh "$0" "$1" &',
                       q['password'],
                       q.get('join','')])
      r('302 Redirect', [('Location', '/wait.html')])
      return []
  r('404 Not Found', [])
  return []

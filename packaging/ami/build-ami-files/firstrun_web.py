import subprocess
import urllib2

# This web application gets launched by firstrun-init
# The form in settings.html posts to /action/set_password
# This application then launches firstrun.sh with the given parameters

def application(env, reply):
  if env['PATH_INFO'] == '/action/set_password':

      # Parse the query string from the POST data
      query = {}
      post_data = env['wsgi.input'].read(int(env['CONTENT_LENGTH']))
      for elem in post_data.split('&'):
          pair = elem.split('=',1)
          if len(pair) == 2:
              query[pair[0]] = urllib2.unquote(pair[1])

      if not query.has_key('password'):
          reply('302 Redirect', [('Location', '/settings.html')])
          return []

      # Launch firstrun.sh
      # bash -c is invoked in this contorted way to background the process
      subprocess.call(['bash', '-c', 'sleep 2; nohup bash firstrun.sh "$1" "$2" &','-',
                       query['password'],
                       query.get('join','')])

      reply('302 Redirect', [('Location', '/wait.html')])
      return []

  reply('404 Not Found', [])
  return ["404 Not Found ", env['PATH_INFO']]

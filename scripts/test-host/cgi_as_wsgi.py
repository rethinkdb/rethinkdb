# Copyright 2010-2012 RethinkDB, all rights reserved.
# This file was originally part of Paste, but Tim Maxwell pulled it out as a
# separate file in 2011 and made some modifications. The original license text
# was:
#     (c) 2005 Ian Bicking and contributors; written for Paste (http://pythonpaste.org)
#     Licensed under the MIT license: http://www.opensource.org/licenses/mit-license.php

"""
Application that runs a CGI script.
"""
import os
import sys
import subprocess32
import urllib
try:
    import select
except ImportError:
    select = None

__all__ = ['CGIError', 'CGIApplication']

class CGIError(Exception):
    """
    Raised when the CGI script can't be found or doesn't
    act like a proper CGI script.
    """

class CGIApplication(object):

    def __init__(self,
                 script,
                 include_os_environ=True,
                 query_string=None):
        if '?' in script:
            assert query_string is None, (
                "You cannot have '?' in your script name (%r) and also "
                "give a query_string (%r)" % (script, query_string))
            script, query_string = script.split('?', 1)
        assert os.access(script, os.X_OK)
        self.script = os.path.abspath(script)
        self.include_os_environ = include_os_environ
        self.query_string = query_string

    def __call__(self, environ, start_response):
        if 'REQUEST_URI' not in environ:
            environ['REQUEST_URI'] = (
                urllib.quote(environ.get('SCRIPT_NAME', ''))
                + urllib.quote(environ.get('PATH_INFO', '')))
        if self.include_os_environ:
            cgi_environ = os.environ.copy()
        else:
            cgi_environ = {}
        for name in environ:
            # Should unicode values be encoded?
            if (name.upper() == name
                and isinstance(environ[name], str)):
                cgi_environ[name] = environ[name]
        if self.query_string is not None:
            old = cgi_environ.get('QUERY_STRING', '')
            if old:
                old += '&'
            cgi_environ['QUERY_STRING'] = old + self.query_string
        cgi_environ['SCRIPT_FILENAME'] = self.script
        proc = subprocess32.Popen(
            [self.script],
            stdin=subprocess32.PIPE,
            stdout=subprocess32.PIPE,
            stderr=subprocess32.PIPE,
            env=cgi_environ,
            cwd=os.path.dirname(self.script),
            )
        writer = CGIWriter(environ, start_response)
        if select and sys.platform != 'win32':
            for block in proc_communicate_yielding_stdout(
                    proc,
                    stdin=StdinReader.from_environ(environ),
                    stderr=environ['wsgi.errors']):
                for block2 in writer.write(block):
                    yield block2
        else:
            stdout, stderr = proc.communicate(StdinReader.from_environ(environ).read())
            if stderr:
                environ['wsgi.errors'].write(stderr)
            for block in writer.write(stdout):
                yield block
        yield ""
        if not writer.headers_finished:
            start_response(writer.status, writer.headers)

class CGIWriter(object):

    def __init__(self, environ, start_response):
        self.environ = environ
        self.start_response = start_response
        self.status = '200 OK'
        self.headers = []
        self.headers_finished = False
        self.buffer = ''

    def write(self, data):
        if self.headers_finished:
            return [data]
        self.buffer += data
        while '\n' in self.buffer:
            if '\r\n' in self.buffer and self.buffer.find('\r\n') < self.buffer.find('\n'):
                line1, self.buffer = self.buffer.split('\r\n', 1)
            else:
                line1, self.buffer = self.buffer.split('\n', 1)
            if not line1:
                self.headers_finished = True
                self.start_response(
                    self.status, self.headers)
                buffer = self.buffer
                del self.buffer
                del self.headers
                del self.status
                return [buffer]
            elif ':' not in line1:
                raise CGIError(
                    "Bad header line: %r" % line1)
            else:
                name, value = line1.split(':', 1)
                value = value.lstrip()
                name = name.strip()
                if name.lower() == 'status':
                    if ' ' not in value:
                        # WSGI requires this space, sometimes CGI scripts don't set it:
                        value = '%s General' % value
                    self.status = value
                else:
                    self.headers.append((name, value))
        return []

class StdinReader(object):

    def __init__(self, stdin, content_length):
        self.stdin = stdin
        self.content_length = content_length

    def from_environ(cls, environ):
        length = environ.get('CONTENT_LENGTH')
        if length:
            length = int(length)
        else:
            length = 0
        return cls(environ['wsgi.input'], length)

    from_environ = classmethod(from_environ)

    def read(self, size=None):
        if not self.content_length:
            return ''
        if size is None:
            text = self.stdin.read(self.content_length)
        else:
            text = self.stdin.read(min(self.content_length, size))
        self.content_length -= len(text)
        return text

def proc_communicate_yielding_stdout(proc, stdin=None, stderr=None):
    """
    Run the given process, piping input and errors to/from the given
    file-like objects (which need not be actual file objects, unlike
    the arguments passed to Popen). Yield data written to stdout as a
    series of strings. Wait for process to terminate.

    Note: this is taken from the posix version of
    subprocess32.Popen.communicate, but made more general through the
    use of file-like objects.
    """
    read_set = []
    write_set = []
    input_buffer = ''
    trans_nl = proc.universal_newlines and hasattr(open, 'newlines')

    if proc.stdin:
        # Flush stdio buffer.  This might block, if the user has
        # been writing to .stdin in an uncontrolled fashion.
        proc.stdin.flush()
        if input:
            write_set.append(proc.stdin)
        else:
            proc.stdin.close()
    else:
        assert stdin is None
    if proc.stdout:
        read_set.append(proc.stdout)
    if proc.stderr:
        read_set.append(proc.stderr)
    else:
        assert stderr is None

    while read_set or write_set:
        rlist, wlist, xlist = select.select(read_set, write_set, [])

        if proc.stdin in wlist:
            # When select has indicated that the file is writable,
            # we can write up to PIPE_BUF bytes without risk
            # blocking.  POSIX defines PIPE_BUF >= 512
            next, input_buffer = input_buffer, ''
            next_len = 512-len(next)
            if next_len:
                next += stdin.read(next_len)
            if not next:
                proc.stdin.close()
                write_set.remove(proc.stdin)
            else:
                bytes_written = os.write(proc.stdin.fileno(), next)
                if bytes_written < len(next):
                    input_buffer = next[bytes_written:]

        if proc.stdout in rlist:
            data = os.read(proc.stdout.fileno(), 1024)
            if data == "":
                proc.stdout.close()
                read_set.remove(proc.stdout)
            if trans_nl:
                data = proc._translate_newlines(data)
            yield data

        if proc.stderr in rlist:
            data = os.read(proc.stderr.fileno(), 1024)
            if data == "":
                proc.stderr.close()
                read_set.remove(proc.stderr)
            if trans_nl:
                data = proc._translate_newlines(data)
            stderr.write(data)

    try:
        proc.wait()
    except OSError, e:
        if e.errno != 10:
            raise

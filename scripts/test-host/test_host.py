# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python

import shelve, threading, subprocess32, os, sys, atexit, time, traceback, cStringIO, signal
import cgi_as_wsgi, send_gmail
import flask

assert __name__ == "__main__"

app = flask.Flask(__name__)

try:
    root_dir = os.environ["TEST_HOST_ROOT"]
except KeyError:
    raise ValueError("You must set environment variable `TEST_HOST_ROOT` to "
        "the directory where the test host should store tests.")
if not os.path.exists(root_dir):
    os.mkdir(root_dir)

try:
    emailer, emailer_password = os.environ["EMAILER"].split(":")
except KeyError:
    raise ValueError("You must set environment variable `EMAILER` to a string "
        "of the form `username:password`, where `username` is a GMail-hosted "
        "email address and `password` is the password for that address.")

# Directory structure:
# 
# $(TEST_HOST_ROOT)/
#  +-database:
#  |     Metadata about all tests, in the form of a python `shelve` database.
#  +-test-%d/
#      +-output.txt:
#      |     Things that this test printed to `stdout` or `stderr`.
#      +-tarball-temp.tar:
#      |     The tarball that we unpacked into `box`. This only exists for a
#      |     brief period, if at all.
#      +-box/
#          +-renderer.py
#          |     The module that we use to format the HTML for the test.
#          +-...
#                The test can put whatever it wants in `box/`.

database = shelve.open(os.path.join(root_dir, "database"), "c")
atexit.register(database.close)
database_lock = threading.Lock()

# Database keys:
#
# `next_test_id`
#     Integer; one greater than highest test ID assigned so far.
# `%d.result`
#     One of the following:
#         `{ "status": "starting" }`:
#             The test is unpacking the tarball or creating directories, or it
#             was doing that when it was interrupted by the server shutting
#             down. Check the `running_tasks` dictionary for more information.
#         `{ "status": "running" }`:
#             The test is running, or was running when it was interrupted by the
#             server shutting down. Again, check `running_tasks`.
#         `{ "status": "done", "return_code": return_code, "end_time": timestamp }`:
#             The test has finished. `return_code` is the return code from the
#             test command. `timestamp` is the time when the test finished.
#         `{ "status": "bug", "traceback": string }`:
#             Something went wrong in the test host. `message` is a Python
#             traceback in string form.
# `%d.result`
#     A dictionary with the following keys:
#         `"title"`:
#             A human-readable title string that will be used to identify this
#             task on the task-list page. E.g. "Nightly test". There is no need
#             for this to be unique.
#         `"start_time"`:
#             The time when this task started.
#         `"command"`:
#             The command line for this test.
#         `"emailees"`:
#             Who got emailed when this test was run.

running_tasks = {}

# `running_tasks` has an entry for every task that is currently running. Each
# entry is a dictionary with the following keys:
#
# `proc`: The `subprocess32.Popen` object associated with the task.

def run_test(test_id, command):
    try:
        test_dir = os.path.join(root_dir, "test-%d" % test_id)
        box_dir = os.path.join(test_dir, "box")
        with database_lock:
            database["%d.result" % test_id] = { "status": "running" }
            database.sync()
        with file(os.path.join(test_dir, "output.txt"), "w") as output_file:
            proc = subprocess32.Popen(command, shell = True, stdout = output_file, stderr = output_file, cwd = box_dir, start_new_session = True)
            running_tasks[test_id]["proc"] = proc
            rc = proc.wait()
        with database_lock:
            database["%d.result" % test_id] = { "status": "done", "return_code": rc, "end_time": time.time() }
            database.sync()
    except Exception, e:
        with database_lock:
            database["%d.result" % test_id] = { "status": "bug", "traceback": traceback.format_exc() }
            database.sync()
    finally:
        del running_tasks[test_id]

@app.route("/spawn", methods = ["POST"])
@app.route("/spawn/", methods = ["POST"])
def spawn():
    tarball_file = flask.request.files["tarball"]
    command = flask.request.form["command"]
    title = flask.request.form.get("title", "Unnamed test")
    emailees = flask.request.form.getlist("emailee")
    for emailee in emailees:
        assert "@" in emailee
    with database_lock:
        test_id = database.get("next_test_id", 0)
        database["next_test_id"] = test_id + 1
        database["%d.metadata" % test_id] = {
            "title": title,
            "start_time": time.time(),
            "command": command,
            "emailees": emailees
            }
        database.sync()
    try:
        running_tasks[test_id] = { }
        try:
            with database_lock:
                database["%d.result" % test_id] = { "status": "starting" }
                database.sync()
            test_dir = os.path.join(root_dir, "test-%d" % test_id)
            os.mkdir(test_dir)
            box_dir = os.path.join(test_dir, "box")
            os.mkdir(box_dir)
            try:
                tar_cmd = ["tar", "--extract", "-z", "-C", box_dir]
                if hasattr(tarball_file, "fileno"):
                    subprocess32.check_output(tar_cmd + ["--file=-"], stdin = tarball_file, stderr = subprocess32.STDOUT)
                else:
                    tarball_path = os.path.join(test_dir, "tarball-temp.tar")
                    tarball_file.save(tarball_path)
                    subprocess32.check_output(tar_cmd + ["--file", tarball_path], stderr = subprocess32.STDOUT)
                    os.remove(tarball_path)
            except subprocess32.CalledProcessError, e:
                raise ValueError("Bad tarball: " + e.output)
            if not os.access(os.path.join(box_dir, "renderer"), os.X_OK):
                raise ValueError("renderer is missing or not executable")
            for emailee in emailees:
                send_gmail.send_gmail(
                    subject = title,
                    body = flask.url_for("view", test_id = test_id, _external = True),
                    sender = (emailer, emailer_password),
                    receiver = emailee)
        except Exception, e:
            del running_tasks[test_id]
            raise
        threading.Thread(target = run_test, args = (test_id, command)).start()
    except Exception, e:
        with database_lock:
            database["%d.result" % test_id] = { "status": "bug", "traceback": traceback.format_exc() }
            database.sync()
        raise
    return "%d\n" % test_id

# If you have a WSGI application hosted at `/foo/bar`, and someone requests
# `/foo/bar/baz`, then the WSGI application should get `SCRIPT_NAME` set to
# `/foo/bar` and `PATH_INFO` set to `/baz`. Unfortunately, Flask doesn't seem to
# do this correctly; it tells the application that `SCRIPT_NAME` is empty and
# `PATH_INFO` is `/foo/bar/baz`. `PathInfoFixMiddleware` is a tiny bit of WSGI
# middleware that fixes this problem.
class PathInfoFixMiddleware(object):
    def __init__(self, script_path, subapplication):
        self.script_path = script_path
        self.subapplication = subapplication
    def __call__(self, environ, start_response):
        environ = environ.copy()
        assert environ["PATH_INFO"].startswith(self.script_path)
        environ["SCRIPT_NAME"] = os.path.join(environ["SCRIPT_NAME"], self.script_path)
        environ["PATH_INFO"] = environ["PATH_INFO"][len(self.script_path):]
        return self.subapplication(environ, start_response)

# If something goes wrong in the renderer script, then the person who wrote the
# renderer script should probably know about it. This bit of middleware catches
# things written to `wsgi.errors` and reports them in the HTTP response.
class ErrorReportingMiddleware(object):
    def __init__(self, subapplication):
        self.subapplication = subapplication
    def __call__(self, environ, start_response):
        status_code_and_headers = []
        def start_response_2(code, headers):
            status_code_and_headers[:] = [code, headers]
        error_buffer = cStringIO.StringIO()
        output_buffer = []
        environ_2 = environ.copy()
        environ_2["wsgi.errors"] = error_buffer
        for thing in self.subapplication(environ_2, start_response_2):
            output_buffer.append(thing)
        if status_code_and_headers[0] >= 500 and error_buffer.getvalue() != "":
            start_response(500, [("Content-Type", "text/plain")])
            return ["The renderer script for this test failed. The following error message was printed:\n\n", error_buffer.getvalue()]
        else:
            start_response(status_code_and_headers[0], status_code_and_headers[1])
            return output_buffer

@app.route("/test/<int:test_id>")
@app.route("/test/<int:test_id>/")
@app.route("/test/<int:test_id>/<path:path>")
def view(test_id, path = None):
    test_dir = os.path.join(root_dir, "test-%d" % test_id)
    if not os.path.exists(test_dir):
        flask.abort(404)
    cgi_app = cgi_as_wsgi.CGIApplication(os.path.join(test_dir, "box", "renderer"))
    error_wrapper = ErrorReportingMiddleware(cgi_app)
    return PathInfoFixMiddleware(flask.url_for("view", test_id = test_id), error_wrapper)

debug_template = """
<html>
    <head>
        <title>Test %(test_id)d debug info</title>
    </head>
    <body>
        %(fields)s
    </body>
</html>"""

@app.route("/debug/<int:test_id>")
@app.route("/debug/<int:test_id>/")
def debug(test_id):
    with database_lock:
        metadata = database.get("%d.metadata" % test_id)
        result = database.get("%d.result" % test_id)
    output_path = os.path.join(root_dir, "test-%d" % test_id, "output.txt")
    if not metadata or not result or not os.access(output_path, os.R_OK):
        flask.abort(404)
    task = running_tasks.get(test_id)
    fields = ""
    fields += """<p>Title: %s</p>""" % flask.escape(metadata["title"])
    fields += """<p>Command: <code>%s</code></p>""" % flask.escape(metadata["command"])
    fields += """<p>Start time: %s</p>""" % flask.escape(time.ctime(metadata["start_time"]))
    if metadata["emailees"]:
        fields += """<p>Emailees: %s</p>""" % ", ".join(flask.escape(e) for e in metadata["emailees"])
    else:
        fields += """<p>Emailees: none</p>"""
    if task is None and result["status"] in ("starting", "running"):
        fields += """<p>Status: interrupted (was %s)</p>""" % result["status"]
    else:
        fields += """<p>Status: %s</p>""" % result["status"]
    if result["status"] == "done":
        fields += """<p>Return code: %d</p>""" % result["return_code"]
        fields += """<p>End time: %s</p>""" % flask.escape(time.ctime(result["end_time"]))
    elif result["status"] == "bug":
        fields += """<p>Traceback:</p><p><code><pre>%s</pre></code></p>""" % flask.escape(result["traceback"])
    with open(output_path) as output_file:
        fields += """<p>Output:</p><p><code><pre>%s</pre></code></p>""" % flask.escape(output_file.read())
    if task is not None:
        fields += """<p>PID: %d</p>""" % task["proc"].pid
        fields += """<form method="post" action="%s"><p><input type="submit" value="Kill"></p></form>""" % flask.url_for("kill", test_id = test_id)
    return debug_template % { "test_id": test_id, "fields": fields }

@app.route("/debug/<int:test_id>/kill", methods = ["POST"])
@app.route("/debug/<int:test_id>/kill/", methods = ["POST"])
def kill(test_id):
    if test_id in running_tasks:
        os.killpg(running_tasks[test_id]["proc"].pid, signal.SIGTERM)
    return flask.redirect(flask.url_for("debug", test_id = test_id))

app.run(host = '0.0.0.0', port = int(os.environ.get("PORT", "5000")), debug = True, threaded = True)

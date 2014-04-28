import os, sys, subprocess, re, io
from jinja2 import Template
import itertools
import test_framework
import utils

def format_tests(test_root, test_tree):
  tests = []
  for name, test in test_tree:
      command_line = test.read_file('description')
      failed = test.read_file('fail_message')
      if failed is None:
        status = 'pass'
      else:
        status = 'fail'

      file_infos = []
      for rel_path in test.list_files(text_only=False):
          path = os.path.join(test_root, name, rel_path)
          file_info = { 'name': os.path.join(name, rel_path) }
          if utils.guess_is_text_file(path):
            file_info['contents'] = file_reader(path)
          file_infos.append(file_info)

      tests.append({
        'name': name,
        'id': name.replace('.', '-'),
        'status': status,
        'files': file_infos
      })
  return sorted(tests, key = lambda t: t['name'])

def file_reader(path):
  f = open(path, "rb")
  while True:
    data = f.readline()
    if len(data) == 0:
      break
    data2 = re.sub(b"[\x80-\xff]", lambda x: '\\' + hex(ord(x.group()))[1:], data)
    yield data2

def generate_html(output_file, **kwargs):
  file_out = open(output_file, 'w')
  template = Template(test_report_template)
  file_out.write(template.render(**kwargs))

def gen_report(test_root, tests):
  buildbot = False
  if "BUILD_NUMBER" in os.environ:
      buildbot = {
          'build_id': os.environ['JOB_NAME'] + ' ' + os.environ["BUILD_NUMBER"],
          'build_link': os.environ['BUILD_URL'] }

  git_info = {
    'branch': subprocess.check_output(
      ['bash', '-c', 'git symbolic-ref HEAD 2>/dev/null || echo "HEAD"']),
    'commit': subprocess.check_output(
      ['git', 'rev-parse', 'HEAD']) }

  git_info['message'] = subprocess.check_output(['git', 'show', '-s', '--format=%B'])

  tests_param = format_tests(test_root, tests)
  passed = sum(1 for test in tests_param if test['status'] == 'pass')
  total = len(tests_param)

  # TODO: use `rethinkdb --version' instead
  rethinkdb_version = subprocess.check_output([os.path.dirname(__file__) + "/../../scripts/gen-version.sh"])

  generate_html(test_root + "/test_results.html",
                buildbot = buildbot,
                git_info = git_info,
                tests = tests_param,
                rethinkdb_version = rethinkdb_version,
                passed = passed,
                total = total,
                filter = filter)

  print 'Wrote test report to "' + test_root + "/test_results.html" + '"'

test_report_template = """
<html>
  <head>
    <title>Test Report</title>
    <script src="//ajax.googleapis.com/ajax/libs/jquery/1.10.2/jquery.min.js"></script>
    <style>
td {border:1px solid grey}
.test { background: red }
.test.pass { background: green }
    </style>
  </head>
  <body>
    <h1>Rethinkdb {{ rethinkdb_version }}</h1>
    {% if buildbot %}
    <p>Build: <a href="{{ buildbot.build_link }}">{{ buildbot.build_id }}</a>
    {% endif %}
    <p>Branch: <a href="https://github.com/rethinkdb/rethinkdb/tree/{{ git_info.branch }}">{{ git_info.branch }}</a>
    <p>Commit: <a href="https://github.com/rethinkdb/rethinkdb/commit/{{ git_info.commit }}">{{ git_info.commit }}</a>
    <p>Commit message:
      <pre>{{ git_info.message }}</pre>
    <p>Passed {{ passed }} of  {{ total }} tests</p>
    <table style='width:100%'>
      {% for test in tests %}
      <tr>
        <td>{{ test.name }}</td>
        <td class="test {{ test.status }}">
          <a href='#{{ test.id }}'
             onclick='$("#{{ test.id }}").toggle()'>
            {{ test.status }}
          </a>
        </td>
        <td width='100%'></td></tr>
      <tr id='{{ test.id }}' style='display:none'>
        <td colspan='4'>
          {% for file in test.files %}
          <ul><li><a href="{{ file.name }}">{{ file.name }}</a></ul>
          {% if file.contents %}
          <div style='border: 1px solid black'>
            <pre>{% for line in file.contents %}{{ line }}{% endfor %}</pre>
          </div>
          {% endif %}
          {% endfor %}
      </td></tr>
      {% endfor %}
    </table>
  </body>
</html>
"""

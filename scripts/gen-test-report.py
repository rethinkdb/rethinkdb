#!/usr/bin/env python

import os, sys, subprocess, re, io
from jinja2 import Template
import itertools

def list_tests(path):
  tests = []
  for name in os.listdir(path):
    if not os.path.isdir(os.path.join(path, name)):
      continue
    cl_file = os.path.join(path, name, "command-line")
    if not os.path.exists(cl_file):
      continue
    command_line = file(cl_file).read()
    for n in itertools.count(1):

      code_file = os.path.join(path, name, "return-code-" + str(n))
      if not os.path.exists(code_file):
        break
      code = int(file(code_file).read())
      if code == 0:
        status = 'pass'
      elif code == 124:
        status = 'time'
      else:
        status = 'fail'

      file_infos = []
      for root, _, files in os.walk(os.path.join(path, name)):
        for file_ in (file_ for file_ in files if file_.split('-')[-1] == str(n)):
          full = os.path.join(root, file_)
          file_info = { 'name': os.path.join(name, file_) }
          type_ = subprocess.check_output(['file', '-i', full])
          if re.match(".*text/", type_):
            file_info['contents'] = file_reader(full)
          file_infos.append(file_info)

      tests.append({
        'name': name,
        'repeat': n,
        'status': status,
        'files': file_infos,
      })
  return tests

def file_reader(path):
  f = open(path, "rb")
  while True:
    data = f.readline()
    if len(data) == 0:
      break
    data2 = re.sub(b"[\x80-\xff]", lambda x: '\\' + hex(ord(x.group()))[1:], data)
    yield data2

def generate_html(input_file, output_file, **kwargs):
  file_in = open(input_file, 'r')
  file_out = open(output_file, 'w')
  template = Template(file_in.read())
  file_out.write(template.render(**kwargs))

def main():

  [_, tests_root] = sys.argv

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

  tests = list_tests(tests_root)
  passed = sum(1 for test in tests if test['status'] == 'pass')
  total = len(tests)
  filter = file(tests_root + "/filter").read()

  rethinkdb_version = subprocess.check_output([os.path.dirname(__file__) + "/gen-version.sh"])

  generate_html(os.path.dirname(__file__) + "/test-report-template.html",
                tests_root + "/test_results.html",
                buildbot = buildbot,
                git_info = git_info,
                tests = tests,
                rethinkdb_version = rethinkdb_version,
                passed = passed,
                total = total,
                filter = filter)

if __name__ == "__main__":
    main()

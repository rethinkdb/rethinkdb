# Copyright 2010-2014 RethinkDB, all rights reserved.

from __future__ import print_function

import json, os, re, subprocess

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
          if utils.guess_is_text_file(path) and os.path.getsize(path) > 0:
            file_info['contents'] = open(path, "rb").read()
          file_infos.append(file_info)

      tests.append({
        'name': name,
        'id': name.replace('.', '-'),
        'status': status,
        'files': file_infos
      })
  return sorted(tests, key = lambda t: t['name'])

def generate_html(output_file, reportData):
  file_out = open(output_file, 'w')
  mustachePath = os.path.realpath(os.path.join(os.path.dirname(__file__), 'mustache', 'mustache.js'))
  mustacheContent = open(mustachePath).read()
  pageHTML = test_report_template % {"pagedata": json.dumps(reportData, separators=(',', ':')), 'mustacheContents': mustacheContent}
  file_out.write(pageHTML)

def gen_report(test_root, tests):
  buildbot = False
  if "BUILD_NUMBER" in os.environ:
    buildbot = {
      'build_id': os.environ['JOB_NAME'] + ' ' + os.environ["BUILD_NUMBER"],
      'build_link': os.environ['BUILD_URL']
    }

  git_info = {
    'branch': subprocess.check_output(['git symbolic-ref HEAD 2>/dev/null || echo "HEAD"'], shell=True),
    'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD']),
    'message': subprocess.check_output(['git', 'show', '-s', '--format=%B'])
  }

  tests_param = format_tests(test_root, tests)
  passed = sum(1 for test in tests_param if test['status'] == 'pass')
  total = len(tests_param)

  # TODO: use `rethinkdb --version' instead
  rethinkdb_version = subprocess.check_output([os.path.dirname(__file__) + "/../../scripts/gen-version.sh"])
  
  reportData = {
    "buildbot": buildbot,
    "tests": tests_param,
    "rethinkdb_version": rethinkdb_version,
    "git_info": git_info,
    "passed_test_count": passed,
    "total_test_count": total
  }
  
  generate_html(test_root + "/test_results.html", reportData)
  print('Wrote test report to "%s/test_results.html"' % os.path.realpath(test_root))

test_report_template = """
<html>
  <head>
    <title>Test Report</title>
    <style>
        td {border:1px solid grey}
        .test { background: red }
        .test.pass { background: green }
    </style>
    <script>
%(mustacheContents)s
    </script>
    <script>
        function toggleVisibility(targetId) {
            var target = document.getElementById(targetId);
            if (target != null) {
                if (target.style.display == "none") {
                    target.style.display = null;
                } else {
                    target.style.display = "none";
                }
            }
        }
        
        pageData = %(pagedata)s;
        
        function displayData() {
            var template = document.getElementById("handlebars-template").textContent;
            document.body.innerHTML = Mustache.to_html(template, pageData);
        }
    </script>
  </head>
  <body onload="displayData()">
    This should be replaced by the conetent in a moment.
    <script id="handlebars-template" type="text/x-handlebars-template">
        <h1>Rethinkdb {{ rethinkdb_version }}</h1>
        {{ #buildbot }}
        <p>Build: <a href="{{ buildbot.build_link }}">{{ buildbot.build_id }}</a>
        {{ /buildbot }}
        <p>Branch: <a href="https://github.com/rethinkdb/rethinkdb/tree/{{ git_info.branch }}">{{ git_info.branch }}</a>
        <p>Commit: <a href="https://github.com/rethinkdb/rethinkdb/commit/{{ git_info.commit }}">{{ git_info.commit }}</a>
        <p>Commit message:
          <pre>{{ git_info.message }}</pre>
        <p>Passed {{ passed }} of  {{ total }} tests</p>
        <table style='width:100%%'>
          {{#tests}}
          <tr>
            <td>{{name}}</td>
            <td class="test {{ status }}">
              <a href='#{{ id }}' onclick='toggleVisibility("{{ id }}")'>{{status}}</a>
            </td>
            <td width='100%%'></td></tr>
          <tr id='{{ id }}' style='display:none'>
            <td colspan='4'>
              {{ #files }}
              <ul><li><a href="{{ name }}">{{ name }}</a></ul>
              <div style='border: 1px solid black'>
                <pre>{{ contents }}</pre>
              </div>
              {{ /files }}
          </td></tr>
          {{ /tests }}
        </table>
    </script>
  </body>
</html>
"""

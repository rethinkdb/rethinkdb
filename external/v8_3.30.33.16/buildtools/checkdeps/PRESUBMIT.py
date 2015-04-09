# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for checkdeps tool.
"""


def CheckChange(input_api, output_api):
  results = []
  results.extend(input_api.canned_checks.RunUnitTests(
      input_api, output_api,
      [input_api.os_path.join(input_api.PresubmitLocalPath(),
                              'checkdeps_test.py')]))
  return results


# Mandatory entrypoint.
def CheckChangeOnUpload(input_api, output_api):
  return CheckChange(input_api, output_api)


# Mandatory entrypoint.
def CheckChangeOnCommit(input_api, output_api):
  return CheckChange(input_api, output_api)

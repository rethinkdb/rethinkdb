#!/usr/bin/env python
#
# Copyright 2008 The Closure Linter Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Linter error handler class that accumulates an array of errors."""

__author__ = ('robbyw@google.com (Robert Walker)',
              'ajp@google.com (Andy Perelson)')


from closure_linter.common import errorhandler


class ErrorAccumulator(errorhandler.ErrorHandler):
  """Error handler object that accumulates errors in a list."""

  def __init__(self):
    self._errors = []

  def HandleError(self, error):
    """Append the error to the list.

    Args:
      error: The error object
    """
    self._errors.append((error.token.line_number, error.code))

  def GetErrors(self):
    """Returns the accumulated errors.

    Returns:
      A sequence of errors.
    """
    return self._errors

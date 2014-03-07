#!/usr/bin/env python
#
# Copyright 2007 The Closure Linter Authors. All Rights Reserved.
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

"""Error codes for JavaScript style checker."""

__author__ = ('robbyw@google.com (Robert Walker)',
              'ajp@google.com (Andy Perelson)')

def ByName(name):
  """Get the error code for the given error name.

  Args:
    name: The name of the error

  Returns:
    The error code
  """
  return globals()[name]


# "File-fatal" errors - these errors stop further parsing of a single file
FILE_NOT_FOUND = -1
FILE_DOES_NOT_PARSE = -2

# Spacing
EXTRA_SPACE = 1
MISSING_SPACE = 2
EXTRA_LINE = 3
MISSING_LINE = 4
ILLEGAL_TAB = 5
WRONG_INDENTATION = 6
WRONG_BLANK_LINE_COUNT = 7

# Semicolons
MISSING_SEMICOLON = 10
MISSING_SEMICOLON_AFTER_FUNCTION = 11
ILLEGAL_SEMICOLON_AFTER_FUNCTION = 12
REDUNDANT_SEMICOLON = 13

# Miscellaneous
ILLEGAL_PROTOTYPE_MEMBER_VALUE = 100
LINE_TOO_LONG = 110
LINE_STARTS_WITH_OPERATOR = 120
COMMA_AT_END_OF_LITERAL = 121
MULTI_LINE_STRING = 130
UNNECESSARY_DOUBLE_QUOTED_STRING = 131

# Requires, provides
GOOG_REQUIRES_NOT_ALPHABETIZED = 140
GOOG_PROVIDES_NOT_ALPHABETIZED = 141
MISSING_GOOG_REQUIRE = 142
MISSING_GOOG_PROVIDE = 143
EXTRA_GOOG_REQUIRE = 144

# JsDoc
INVALID_JSDOC_TAG = 200
INVALID_USE_OF_DESC_TAG = 201
NO_BUG_NUMBER_AFTER_BUG_TAG = 202
MISSING_PARAMETER_DOCUMENTATION = 210
EXTRA_PARAMETER_DOCUMENTATION = 211
WRONG_PARAMETER_DOCUMENTATION = 212
MISSING_JSDOC_TAG_TYPE = 213
MISSING_JSDOC_TAG_DESCRIPTION = 214
MISSING_JSDOC_PARAM_NAME = 215
OUT_OF_ORDER_JSDOC_TAG_TYPE = 216
MISSING_RETURN_DOCUMENTATION = 217
UNNECESSARY_RETURN_DOCUMENTATION = 218
MISSING_BRACES_AROUND_TYPE = 219
MISSING_MEMBER_DOCUMENTATION = 220
MISSING_PRIVATE = 221
EXTRA_PRIVATE = 222
INVALID_OVERRIDE_PRIVATE = 223
INVALID_INHERIT_DOC_PRIVATE = 224
MISSING_JSDOC_TAG_THIS = 225
UNNECESSARY_BRACES_AROUND_INHERIT_DOC = 226
INVALID_AUTHOR_TAG_DESCRIPTION = 227
JSDOC_PREFER_QUESTION_TO_PIPE_NULL = 230
JSDOC_ILLEGAL_QUESTION_WITH_PIPE = 231
JSDOC_TAG_DESCRIPTION_ENDS_WITH_INVALID_CHARACTER = 240
# TODO(robbyw): Split this in to more specific syntax problems.
INCORRECT_SUPPRESS_SYNTAX = 250
INVALID_SUPPRESS_TYPE = 251
UNNECESSARY_SUPPRESS = 252

# File ending
FILE_MISSING_NEWLINE = 300
FILE_IN_BLOCK = 301

# Interfaces
INTERFACE_CONSTRUCTOR_CANNOT_HAVE_PARAMS = 400
INTERFACE_METHOD_CANNOT_HAVE_CODE = 401

# ActionScript specific errors:
# TODO(user): move these errors to their own file and move all JavaScript
# specific errors to their own file as well.
# All ActionScript specific errors should have error number at least 1000.
FUNCTION_MISSING_RETURN_TYPE = 1132
PARAMETER_MISSING_TYPE = 1133
VAR_MISSING_TYPE = 1134
PARAMETER_MISSING_DEFAULT_VALUE = 1135
IMPORTS_NOT_ALPHABETIZED = 1140
IMPORT_CONTAINS_WILDCARD = 1141
UNUSED_IMPORT = 1142
INVALID_TRACE_SEVERITY_LEVEL = 1250
MISSING_TRACE_SEVERITY_LEVEL = 1251
MISSING_TRACE_MESSAGE = 1252
REMOVE_TRACE_BEFORE_SUBMIT = 1253
REMOVE_COMMENT_BEFORE_SUBMIT = 1254
# End of list of ActionScript specific errors.

NEW_ERRORS = frozenset([
    # Errors added after 2.0.2:
    WRONG_INDENTATION,
    MISSING_SEMICOLON,
    # Errors added after 2.2.5:
    WRONG_BLANK_LINE_COUNT,
    EXTRA_GOOG_REQUIRE,
    ])

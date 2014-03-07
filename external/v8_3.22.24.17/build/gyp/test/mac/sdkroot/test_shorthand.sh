#!/bin/bash
# Copyright (c) 2012 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

if ! expected=$(xcodebuild -version -sdk macosx10.6 Path 2>/dev/null) ; then
  expected=$(xcodebuild -version -sdk macosx10.7 Path)
fi

test $SDKROOT = $expected

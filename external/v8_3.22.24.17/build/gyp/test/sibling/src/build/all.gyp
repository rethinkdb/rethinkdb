# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        '../prog1/prog1.gyp:*',
        '../prog2/prog2.gyp:*',
      ],
    },
  ],
}

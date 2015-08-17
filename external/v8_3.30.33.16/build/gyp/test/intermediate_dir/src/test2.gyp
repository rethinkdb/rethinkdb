# Copyright (c) 2011 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'target2',
      'type': 'none',
      'actions': [
        {
          'action_name': 'intermediate',
          'inputs': [],
          'outputs': [
            '<(INTERMEDIATE_DIR)/intermediate_out.txt',
            'outfile.txt',
          ],
          'action': [
            'python', 'script.py', 'target2', '<(_outputs)',
          ],
          # Allows the test to run without hermetic cygwin on windows.
          'msvs_cygwin_shell': 0,
        },
        {
          'action_name': 'shared_intermediate',
          'inputs': [
            'shared_infile.txt',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/intermediate_out.txt',
            'shared_outfile.txt',
          ],
          'action': [
            'python', 'script.py', 'shared_target2', '<(_outputs)',
          ],
          # Allows the test to run without hermetic cygwin on windows.
          'msvs_cygwin_shell': 0,
        },
      ],
    },
  ],
}

# Copyright 2012 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{
  'variables': {
    'v8_code': 1,
    'console%': '',
    # Enable support for Intel VTune. Supported on ia32/x64 only
    'v8_enable_vtunejit%': 0,
    'v8_enable_i18n_support%': 1,
  },
  'includes': ['../build/toolchain.gypi', '../build/features.gypi'],
  'targets': [
    {
      'target_name': 'd8',
      'type': 'executable',
      'dependencies': [
        '../tools/gyp/v8.gyp:v8',
      ],
      # Generated source files need this explicitly:
      'include_dirs+': [
        '../src',
      ],
      'sources': [
        'd8.cc',
      ],
      'conditions': [
        [ 'console=="readline"', {
          'libraries': [ '-lreadline', ],
          'sources': [ 'd8-readline.cc' ],
        }],
        [ 'component!="shared_library"', {
          'sources': [ 'd8-debug.cc', '<(SHARED_INTERMEDIATE_DIR)/d8-js.cc', ],
          'conditions': [
            [ 'want_separate_host_toolset==1', {
              'dependencies': [
                'd8_js2c#host',
              ],
            }, {
              'dependencies': [
                'd8_js2c',
              ],
            }],
            ['(OS=="linux" or OS=="mac" or OS=="freebsd" or OS=="netbsd" \
               or OS=="openbsd" or OS=="solaris" or OS=="android")', {
              'sources': [ 'd8-posix.cc', ]
            }],
            [ 'OS=="win"', {
              'sources': [ 'd8-windows.cc', ]
            }],
          ],
        }],
        ['v8_enable_vtunejit==1', {
          'dependencies': [
            '../src/third_party/vtune/v8vtune.gyp:v8_vtune',
          ],
        }],
        ['v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icui18n',
            '<(icu_gyp_path):icuuc',
          ],
        }],
        ['OS=="win" and v8_enable_i18n_support==1', {
          'dependencies': [
            '<(icu_gyp_path):icudata',
          ],
        }],
      ],
    },
    {
      'target_name': 'd8_js2c',
      'type': 'none',
      'variables': {
        'js_files': [
          'd8.js',
          'macros.py',
        ],
      },
      'conditions': [
        [ 'want_separate_host_toolset==1', {
          'toolsets': ['host'],
        }, {
          'toolsets': ['target'],
        }]
      ],
      'actions': [
        {
          'action_name': 'd8_js2c',
          'inputs': [
            '../tools/js2c.py',
            '<@(js_files)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/d8-js.cc',
          ],
          'action': [
            'python',
            '../tools/js2c.py',
            '<@(_outputs)',
            'D8',
            'off',  # compress startup data
            '<@(js_files)'
          ],
        },
      ],
    }
  ],
}

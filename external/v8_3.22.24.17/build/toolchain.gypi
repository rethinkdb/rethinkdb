# Copyright 2013 the V8 project authors. All rights reserved.
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

# Shared definitions for all V8-related targets.

{
  'variables': {
    'msvs_use_common_release': 0,
    'gcc_version%': 'unknown',
    'CXX%': '${CXX:-$(which g++)}',  # Used to assemble a shell command.
    'v8_target_arch%': '<(target_arch)',
    # Native Client builds currently use the V8 ARM JIT and
    # arm/simulator-arm.cc to defer the significant effort required
    # for NaCl JIT support. The nacl_target_arch variable provides
    # the 'true' target arch for places in this file that need it.
    # TODO(bradchen): get rid of nacl_target_arch when someday
    # NaCl V8 builds stop using the ARM simulator
    'nacl_target_arch%': 'none',     # must be set externally

    # Setting 'v8_can_use_vfp32dregs' to 'true' will cause V8 to use the VFP
    # registers d16-d31 in the generated code, both in the snapshot and for the
    # ARM target. Leaving the default value of 'false' will avoid the use of
    # these registers in the snapshot and use CPU feature probing when running
    # on the target.
    'v8_can_use_vfp32dregs%': 'false',
    'arm_test%': 'off',

    # Similar to vfp but on MIPS.
    'v8_can_use_fpu_instructions%': 'true',

    # Similar to the ARM hard float ABI but on MIPS.
    'v8_use_mips_abi_hardfloat%': 'true',

    # Default arch variant for MIPS.
    'mips_arch_variant%': 'mips32r2',

    'v8_enable_backtrace%': 0,

    # Enable profiling support. Only required on Windows.
    'v8_enable_prof%': 0,

    # Some versions of GCC 4.5 seem to need -fno-strict-aliasing.
    'v8_no_strict_aliasing%': 0,

    # Chrome needs this definition unconditionally. For standalone V8 builds,
    # it's handled in build/standalone.gypi.
    'want_separate_host_toolset%': 1,

    'host_os%': '<(OS)',
    'werror%': '-Werror',
    # For a shared library build, results in "libv8-<(soname_version).so".
    'soname_version%': '',

    # Allow to suppress the array bounds warning (default is no suppression).
    'wno_array_bounds%': '',
  },
  'target_defaults': {
    'conditions': [
      ['v8_target_arch=="arm"', {
        'defines': [
          'V8_TARGET_ARCH_ARM',
        ],
        'target_conditions': [
          ['_toolset=="host"', {
            'variables': {
              'armcompiler': '<!($(echo ${CXX_host:-$(which g++)}) -v 2>&1 | grep -q "^Target: arm" && echo "yes" || echo "no")',
            },
            'conditions': [
              ['armcompiler=="yes"', {
                'conditions': [
                  [ 'armv7==1', {
                    'cflags': ['-march=armv7-a',],
                  }],
                  [ 'armv7==1 or armv7=="default"', {
                    'conditions': [
                      [ 'arm_neon==1', {
                        'cflags': ['-mfpu=neon',],
                      },
                      {
                        'conditions': [
                          [ 'arm_fpu!="default"', {
                            'cflags': ['-mfpu=<(arm_fpu)',],
                          }],
                        ],
                      }],
                    ],
                  }],
                  [ 'arm_float_abi!="default"', {
                    'cflags': ['-mfloat-abi=<(arm_float_abi)',],
                  }],
                  [ 'arm_thumb==1', {
                    'cflags': ['-mthumb',],
                  }],
                  [ 'arm_thumb==0', {
                    'cflags': ['-marm',],
                  }],
                  [ 'arm_test=="on"', {
                    'defines': [
                      'ARM_TEST',
                    ],
                  }],
                ],
              }, {
                # armcompiler=="no"
                'conditions': [
                  [ 'armv7==1 or armv7=="default"', {
                    'defines': [
                      'CAN_USE_ARMV7_INSTRUCTIONS=1',
                    ],
                    'conditions': [
                      [ 'arm_fpu=="default"', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                        ],
                      }],
                      [ 'arm_fpu=="vfpv3-d16"', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                        ],
                      }],
                      [ 'arm_fpu=="vfpv3"', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                          'CAN_USE_VFP32DREGS',
                        ],
                      }],
                      [ 'arm_fpu=="neon" or arm_neon==1', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                          'CAN_USE_VFP32DREGS',
                        ],
                      }],
                    ],
                  }],
                  [ 'arm_float_abi=="hard"', {
                    'defines': [
                      'USE_EABI_HARDFLOAT=1',
                    ],
                  }],
                  [ 'arm_float_abi=="softfp" or arm_float_abi=="default"', {
                    'defines': [
                      'USE_EABI_HARDFLOAT=0',
                    ],
                  }],
                ],
                'defines': [
                  'ARM_TEST',
                ],
              }],
            ],
          }],  # _toolset=="host"
          ['_toolset=="target"', {
            'variables': {
              'armcompiler': '<!($(echo ${CXX_target:-<(CXX)}) -v 2>&1 | grep -q "^Target: arm" && echo "yes" || echo "no")',
            },
            'conditions': [
              ['armcompiler=="yes"', {
                'conditions': [
                  [ 'armv7==1', {
                    'cflags': ['-march=armv7-a',],
                  }],
                  [ 'armv7==1 or armv7=="default"', {
                    'conditions': [
                      [ 'arm_neon==1', {
                        'cflags': ['-mfpu=neon',],
                      },
                      {
                        'conditions': [
                          [ 'arm_fpu!="default"', {
                            'cflags': ['-mfpu=<(arm_fpu)',],
                          }],
                        ],
                      }],
                    ],
                  }],
                  [ 'arm_float_abi!="default"', {
                    'cflags': ['-mfloat-abi=<(arm_float_abi)',],
                  }],
                  [ 'arm_thumb==1', {
                    'cflags': ['-mthumb',],
                  }],
                  [ 'arm_thumb==0', {
                    'cflags': ['-marm',],
                  }],
                  [ 'arm_test=="on"', {
                    'defines': [
                      'ARM_TEST',
                    ],
                  }],
                ],
              }, {
                # armcompiler=="no"
                'conditions': [
                  [ 'armv7==1 or armv7=="default"', {
                    'defines': [
                      'CAN_USE_ARMV7_INSTRUCTIONS=1',
                    ],
                    'conditions': [
                      [ 'arm_fpu=="default"', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                        ],
                      }],
                      [ 'arm_fpu=="vfpv3-d16"', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                        ],
                      }],
                      [ 'arm_fpu=="vfpv3"', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                          'CAN_USE_VFP32DREGS',
                        ],
                      }],
                      [ 'arm_fpu=="neon" or arm_neon==1', {
                        'defines': [
                          'CAN_USE_VFP3_INSTRUCTIONS',
                          'CAN_USE_VFP32DREGS',
                        ],
                      }],
                    ],
                  }],
                  [ 'arm_float_abi=="hard"', {
                    'defines': [
                      'USE_EABI_HARDFLOAT=1',
                    ],
                  }],
                  [ 'arm_float_abi=="softfp" or arm_float_abi=="default"', {
                    'defines': [
                      'USE_EABI_HARDFLOAT=0',
                    ],
                  }],
                ],
                'defines': [
                  'ARM_TEST',
                ],
              }],
            ],
          }],  # _toolset=="target"
        ],
      }],  # v8_target_arch=="arm"
      ['v8_target_arch=="ia32"', {
        'defines': [
          'V8_TARGET_ARCH_IA32',
        ],
      }],  # v8_target_arch=="ia32"
      ['v8_target_arch=="mipsel"', {
        'defines': [
          'V8_TARGET_ARCH_MIPS',
        ],
        'variables': {
          'mipscompiler': '<!($(echo <(CXX)) -v 2>&1 | grep -q "^Target: mips" && echo "yes" || echo "no")',
        },
        'conditions': [
          ['mipscompiler=="yes"', {
            'target_conditions': [
              ['_toolset=="target"', {
                'cflags': ['-EL'],
                'ldflags': ['-EL'],
                'conditions': [
                  [ 'v8_use_mips_abi_hardfloat=="true"', {
                    'cflags': ['-mhard-float'],
                    'ldflags': ['-mhard-float'],
                  }, {
                    'cflags': ['-msoft-float'],
                    'ldflags': ['-msoft-float'],
                  }],
                  ['mips_arch_variant=="mips32r2"', {
                    'cflags': ['-mips32r2', '-Wa,-mips32r2'],
                  }],
                  ['mips_arch_variant=="mips32r1"', {
                    'cflags': ['-mips32', '-Wa,-mips32'],
                 }],
                  ['mips_arch_variant=="loongson"', {
                    'cflags': ['-mips3', '-Wa,-mips3'],
                  }],
                ],
              }],
            ],
          }],
          [ 'v8_can_use_fpu_instructions=="true"', {
            'defines': [
              'CAN_USE_FPU_INSTRUCTIONS',
            ],
          }],
          [ 'v8_use_mips_abi_hardfloat=="true"', {
            'defines': [
              '__mips_hard_float=1',
              'CAN_USE_FPU_INSTRUCTIONS',
            ],
          }, {
            'defines': [
              '__mips_soft_float=1'
            ],
          }],
          ['mips_arch_variant=="mips32r2"', {
            'defines': ['_MIPS_ARCH_MIPS32R2',],
          }],
          ['mips_arch_variant=="loongson"', {
            'defines': ['_MIPS_ARCH_LOONGSON',],
          }],
        ],
      }],  # v8_target_arch=="mipsel"
      ['v8_target_arch=="x64"', {
        'defines': [
          'V8_TARGET_ARCH_X64',
        ],
        'xcode_settings': {
          'ARCHS': [ 'x86_64' ],
        },
        'msvs_settings': {
          'VCLinkerTool': {
            'StackReserveSize': '2097152',
          },
        },
        'msvs_configuration_platform': 'x64',
      }],  # v8_target_arch=="x64"
      ['OS=="win"', {
        'defines': [
          'WIN32',
        ],
        'msvs_configuration_attributes': {
          'OutputDirectory': '<(DEPTH)\\build\\$(ConfigurationName)',
          'IntermediateDirectory': '$(OutDir)\\obj\\$(ProjectName)',
          'CharacterSet': '1',
        },
      }],
      ['OS=="win" and v8_enable_prof==1', {
        'msvs_settings': {
          'VCLinkerTool': {
            'GenerateMapFile': 'true',
          },
        },
      }],
      ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
         or OS=="netbsd"', {
        'conditions': [
          [ 'v8_no_strict_aliasing==1', {
            'cflags': [ '-fno-strict-aliasing' ],
          }],
        ],  # conditions
      }],
      ['OS=="solaris"', {
        'defines': [ '__C99FEATURES__=1' ],  # isinf() etc.
      }],
      ['(OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
         or OS=="netbsd" or OS=="mac" or OS=="android") and \
        (v8_target_arch=="arm" or v8_target_arch=="ia32" or \
         v8_target_arch=="mipsel")', {
        # Check whether the host compiler and target compiler support the
        # '-m32' option and set it if so.
        'target_conditions': [
          ['_toolset=="host"', {
            'variables': {
              'm32flag': '<!((echo | $(echo ${CXX_host:-$(which g++)}) -m32 -E - > /dev/null 2>&1) && echo "-m32" || true)',
            },
            'cflags': [ '<(m32flag)' ],
            'ldflags': [ '<(m32flag)' ],
            'xcode_settings': {
              'ARCHS': [ 'i386' ],
            },
          }],
          ['_toolset=="target"', {
            'variables': {
              'm32flag': '<!((echo | $(echo ${CXX_target:-<(CXX)}) -m32 -E - > /dev/null 2>&1) && echo "-m32" || true)',
              'clang%': 0,
            },
            'conditions': [
              ['(OS!="android" or clang==1) and \
                nacl_target_arch!="nacl_x64"', {
                'cflags': [ '<(m32flag)' ],
                'ldflags': [ '<(m32flag)' ],
              }],
            ],
            'xcode_settings': {
              'ARCHS': [ 'i386' ],
            },
          }],
        ],
      }],
      ['(OS=="linux") and (v8_target_arch=="x64")', {
        # Check whether the host compiler and target compiler support the
        # '-m64' option and set it if so.
        'target_conditions': [
          ['_toolset=="host"', {
            'variables': {
              'm64flag': '<!((echo | $(echo ${CXX_host:-$(which g++)}) -m64 -E - > /dev/null 2>&1) && echo "-m64" || true)',
            },
            'cflags': [ '<(m64flag)' ],
            'ldflags': [ '<(m64flag)' ],
          }],
          ['_toolset=="target"', {
            'variables': {
              'm64flag': '<!((echo | $(echo ${CXX_target:-<(CXX)}) -m64 -E - > /dev/null 2>&1) && echo "-m64" || true)',
            },
            'cflags': [ '<(m64flag)' ],
            'ldflags': [ '<(m64flag)' ],
          }]
        ],
      }],
      ['OS=="freebsd" or OS=="openbsd"', {
        'cflags': [ '-I/usr/local/include' ],
      }],
      ['OS=="netbsd"', {
        'cflags': [ '-I/usr/pkg/include' ],
      }],
    ],  # conditions
    'configurations': {
      'Debug': {
        'defines': [
          'ENABLE_DISASSEMBLER',
          'V8_ENABLE_CHECKS',
          'OBJECT_PRINT',
          'VERIFY_HEAP',
          'DEBUG'
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'conditions': [
              ['v8_optimized_debug==0', {
                'Optimization': '0',
                'conditions': [
                  ['component=="shared_library"', {
                    'RuntimeLibrary': '3',  # /MDd
                  }, {
                    'RuntimeLibrary': '1',  # /MTd
                  }],
                ],
              }],
              ['v8_optimized_debug==1', {
                'Optimization': '1',
                'InlineFunctionExpansion': '2',
                'EnableIntrinsicFunctions': 'true',
                'FavorSizeOrSpeed': '0',
                'StringPooling': 'true',
                'BasicRuntimeChecks': '0',
                'conditions': [
                  ['component=="shared_library"', {
                    'RuntimeLibrary': '3',  # /MDd
                  }, {
                    'RuntimeLibrary': '1',  # /MTd
                  }],
                ],
              }],
              ['v8_optimized_debug==2', {
                'Optimization': '2',
                'InlineFunctionExpansion': '2',
                'EnableIntrinsicFunctions': 'true',
                'FavorSizeOrSpeed': '0',
                'StringPooling': 'true',
                'BasicRuntimeChecks': '0',
                'conditions': [
                  ['component=="shared_library"', {
                    'RuntimeLibrary': '3',  #/MDd
                  }, {
                    'RuntimeLibrary': '1',  #/MTd
                  }],
                  ['v8_target_arch=="x64"', {
                    # TODO(2207): remove this option once the bug is fixed.
                    'WholeProgramOptimization': 'true',
                  }],
                ],
              }],
            ],
          },
          'VCLinkerTool': {
            'conditions': [
              ['v8_optimized_debug==0', {
                'LinkIncremental': '2',
              }],
              ['v8_optimized_debug==1', {
                'LinkIncremental': '2',
              }],
              ['v8_optimized_debug==2', {
                'LinkIncremental': '1',
                'OptimizeReferences': '2',
                'EnableCOMDATFolding': '2',
              }],
            ],
          },
        },
        'conditions': [
          ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="netbsd"', {
            'cflags': [ '-Wall', '<(werror)', '-W', '-Wno-unused-parameter',
                        '-Wnon-virtual-dtor', '-Woverloaded-virtual',
                        '<(wno_array_bounds)' ],
            'conditions': [
              ['v8_optimized_debug==0', {
                'cflags!': [
                  '-O0',
                  '-O3',
                  '-O2',
                  '-O1',
                  '-Os',
                ],
                'cflags': [
                  '-fdata-sections',
                  '-ffunction-sections',
                ],
              }],
              ['v8_optimized_debug==1', {
                'cflags!': [
                  '-O0',
                  '-O3', # TODO(2807) should be -O1.
                  '-O2',
                  '-Os',
                ],
                'cflags': [
                  '-fdata-sections',
                  '-ffunction-sections',
                  '-O1', # TODO(2807) should be -O3.
                ],
              }],
              ['v8_optimized_debug==2', {
                'cflags!': [
                  '-O0',
                  '-O1',
                  '-Os',
                ],
                'cflags': [
                  '-fdata-sections',
                  '-ffunction-sections',
                ],
                'defines': [
                  'OPTIMIZED_DEBUG'
                ],
                'conditions': [
                  # TODO(crbug.com/272548): Avoid -O3 in NaCl
                  ['nacl_target_arch=="none"', {
                    'cflags': ['-O3'],
                    'cflags!': ['-O2'],
                    }, {
                    'cflags': ['-O2'],
                    'cflags!': ['-O3'],
                  }],
                ],
              }],
              ['v8_optimized_debug!=0 and gcc_version==44 and clang==0', {
                'cflags': [
                  # Avoid crashes with gcc 4.4 in the v8 test suite.
                  '-fno-tree-vrp',
                ],
              }],
            ],
          }],
          ['OS=="linux" and v8_enable_backtrace==1', {
            # Support for backtrace_symbols.
            'ldflags': [ '-rdynamic' ],
          }],
          ['OS=="android"', {
            'variables': {
              'android_full_debug%': 1,
            },
            'conditions': [
              ['android_full_debug==0', {
                # Disable full debug if we want a faster v8 in a debug build.
                # TODO(2304): pass DISABLE_DEBUG_ASSERT instead of hiding DEBUG.
                'defines!': [
                  'DEBUG',
                ],
              }],
            ],
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'conditions': [
                 ['v8_optimized_debug==0', {
                   'GCC_OPTIMIZATION_LEVEL': '0',  # -O0
                 }, {
                   'GCC_OPTIMIZATION_LEVEL': '3',  # -O3
                   'GCC_STRICT_ALIASING': 'YES',
                 }],
               ],
            },
          }],
        ],
      },  # Debug
      'Release': {
        'conditions': [
          ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="netbsd"', {
            'cflags!': [
              '-Os',
            ],
            'cflags': [
              '-fdata-sections',
              '-ffunction-sections',
              '<(wno_array_bounds)',
            ],
            'conditions': [
              [ 'gcc_version==44 and clang==0', {
                'cflags': [
                  # Avoid crashes with gcc 4.4 in the v8 test suite.
                  '-fno-tree-vrp',
                ],
              }],
              # TODO(crbug.com/272548): Avoid -O3 in NaCl
              ['nacl_target_arch=="none"', {
                'cflags': ['-O3'],
                'cflags!': ['-O2'],
              }, {
                'cflags': ['-O2'],
                'cflags!': ['-O3'],
              }],
            ],
          }],
          ['OS=="android"', {
            'cflags!': [
              '-O3',
              '-Os',
            ],
            'cflags': [
              '-fdata-sections',
              '-ffunction-sections',
              '-O2',
            ],
            'conditions': [
              [ 'gcc_version==44 and clang==0', {
                'cflags': [
                  # Avoid crashes with gcc 4.4 in the v8 test suite.
                  '-fno-tree-vrp',
                ],
              }],
            ],
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'GCC_OPTIMIZATION_LEVEL': '3',  # -O3

              # -fstrict-aliasing.  Mainline gcc
              # enables this at -O2 and above,
              # but Apple gcc does not unless it
              # is specified explicitly.
              'GCC_STRICT_ALIASING': 'YES',
            },
          }],  # OS=="mac"
          ['OS=="win"', {
            'msvs_settings': {
              'VCCLCompilerTool': {
                'Optimization': '2',
                'InlineFunctionExpansion': '2',
                'EnableIntrinsicFunctions': 'true',
                'FavorSizeOrSpeed': '0',
                'StringPooling': 'true',
                'conditions': [
                  ['component=="shared_library"', {
                    'RuntimeLibrary': '2',  #/MD
                  }, {
                    'RuntimeLibrary': '0',  #/MT
                  }],
                  ['v8_target_arch=="x64"', {
                    # TODO(2207): remove this option once the bug is fixed.
                    'WholeProgramOptimization': 'true',
                  }],
                ],
              },
              'VCLinkerTool': {
                'LinkIncremental': '1',
                'OptimizeReferences': '2',
                'EnableCOMDATFolding': '2',
              },
            },
          }],  # OS=="win"
        ],  # conditions
      },  # Release
    },  # configurations
  },  # target_defaults
}

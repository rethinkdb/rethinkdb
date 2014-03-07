{
  'variables': {
    'visibility%': 'hidden',
    'library%': 'static_library', # allow override to 'shared_library' for DLL/.so builds
    'component%': 'static_library',
    'host_arch%': '',
    'target_arch%': ''
   },

  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {

      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'cflags': [ '-g', '-O0' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'target_conditions': [
              ['library=="static_library"', {
                'RuntimeLibrary': 1 # static debug
              }, {
                'RuntimeLibrary': 3 # DLL debug
              }]
            ],
            'Optimization': 0, # /Od, no optimization
            'MinimalRebuild': 'false',
            'OmitFramePointers': 'false',
            'BasicRuntimeChecks': 3 # /RTC1
          },
          'VCLinkerTool': {
            'LinkIncremental': 2 # enable incremental linking
          }
        },
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '0'
        }
      },

      'Release': {
        'defines': [ 'NDEBUG' ],
        'cflags': [
          '-O3',
          '-fomit-frame-pointer',
          '-fdata-sections',
          '-ffunction-sections'
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'target_conditions': [
              ['library=="static_library"', {
                'RuntimeLibrary': 0, # static release
              }, {
                'RuntimeLibrary': 2, # debug release
              }],
            ],
            'Optimization': 3, # /Ox, full optimization
            'FavorSizeOrSpeed': 1, # /Ot, favour speed over size
            'InlineFunctionExpansion': 2, # /Ob2, inline anything eligible
            'WholeProgramOptimization': 'true', # /GL, whole program optimization, needed for LTCG
            'OmitFramePointers': 'true',
            'EnableFunctionLevelLinking': 'true',
            'EnableIntrinsicFunctions': 'true'
          },
          'VCLibrarianTool': {
            'AdditionalOptions': [
              '/LTCG' # link time code generation
            ]
          },
          'VCLinkerTool': {
            'LinkTimeCodeGeneration': 1, # link-time code generation
            'OptimizeReferences': 2, # /OPT:REF
            'EnableCOMDATFolding': 2, # /OPT:ICF
            'LinkIncremental': 1 # disable incremental linking
          },
        },
      }
    },

    'msvs_settings': {
      'VCCLCompilerTool': {
        'StringPooling': 'true', # pool string literals
        'DebugInformationFormat': 3, # Generate a PDB
        'WarningLevel': 3,
        'BufferSecurityCheck': 'true',
        'ExceptionHandling': 1, # /EHsc
        'SuppressStartupBanner': 'true',
        'WarnAsError': 'false',
        'AdditionalOptions': [
           '/MP', # compile across multiple CPUs
         ],
      },
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
        'RandomizedBaseAddress': 2, # enable ASLR
        'DataExecutionPrevention': 2, # enable DEP
        'AllowIsolation': 'true',
        'SuppressStartupBanner': 'true',
        'target_conditions': [
          ['_type=="executable"', {
            'SubSystem': 1, # console executable
          }],
        ],
      },
    },

    'xcode_settings': {
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'GCC_CW_ASM_SYNTAX': 'NO',                # No -fasm-blocks
      'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',        # -fno-exceptions
      'GCC_ENABLE_CPP_RTTI': 'NO',              # -fno-rtti
      'GCC_ENABLE_PASCAL_STRINGS': 'NO',        # No -mpascal-strings
      # GCC_INLINES_ARE_PRIVATE_EXTERN maps to -fvisibility-inlines-hidden
      'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
      'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',      # -fvisibility=hidden
      'GCC_THREADSAFE_STATICS': 'NO',           # -fno-threadsafe-statics
      'GCC_WARN_ABOUT_MISSING_NEWLINE': 'YES',  # -Wnewline-eof
      'PREBINDING': 'NO',                       # No -Wl,-prebind
      'USE_HEADERMAP': 'NO',
      'WARNING_CFLAGS': [
        '-Wall',
        '-Wendif-labels',
        '-W',
        '-Wno-unused-parameter'
      ]
    },

    'conditions': [
      ['OS == "win"', {
        'msvs_cygwin_shell': 0, # prevent actions from trying to use cygwin
        'defines': [
          'WIN32',
          # we don't want VC++ warning us about how dangerous C functions are.
          '_CRT_SECURE_NO_DEPRECATE',
          # ... or that C implementations shouldn't use POSIX names
          '_CRT_NONSTDC_NO_DEPRECATE'
        ],
      }],

      [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
        'variables': {
          'gcc_version%': '<!(python build/gcc_version.py)>)'
        },
        'cflags': [ '-Wall' ],
        'cflags_cc': [ '-fno-rtti', '-fno-exceptions' ],
        'conditions': [
          [ 'host_arch != target_arch and target_arch=="ia32"', {
            'cflags': [ '-m32' ],
            'ldflags': [ '-m32' ]
          }],
          [ 'OS=="linux"', {
            'cflags': [ '-ansi' ]
          }],
          [ 'visibility=="hidden" and gcc_version >= "4.0.0"', {
            'cflags': [ '-fvisibility=hidden' ]
          }],
        ]
      }]
    ],

    'target_conditions': [
      ['_type!="static_library"', {
        'cflags': [ '-fPIC' ],
        'xcode_settings': {
          'GCC_DYNAMIC_NO_PIC': 'NO', # No -mdynamic-no-pic
                                      # (Equivalent to -fPIC)
          'OTHER_LDFLAGS': [ '-Wl,-search_paths_first' ]
        }
      }]
    ]
  }
}

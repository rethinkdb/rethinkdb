# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    'icu.gypi',
  ],
  'variables': {
    'use_system_icu%': 0,
    'icu_use_data_file_flag%': 0,
    'want_separate_host_toolset%': 1,
  },
  'target_defaults': {
    'direct_dependent_settings': {
      'defines': [
        # Tell ICU to not insert |using namespace icu;| into its headers,
        # so that chrome's source explicitly has to use |icu::|.
        'U_USING_ICU_NAMESPACE=0',
        # We don't use ICU plugins and dyload is only necessary for them.
        # NaCl-related builds also fail looking for dlfcn.h when it's enabled.
        'U_ENABLE_DYLOAD=0',
      ],
    },
    'defines': [
      'U_USING_ICU_NAMESPACE=0',
      'HAVE_DLOPEN=0',
    ],
    'conditions': [
      ['component=="static_library"', {
        'defines': [
          'U_STATIC_IMPLEMENTATION',
        ],
      }],
      ['(OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
         or OS=="netbsd" or OS=="mac" or OS=="android" or OS=="qnx") and \
        (target_arch=="arm" or target_arch=="ia32" or \
         target_arch=="mipsel")', {
        'target_conditions': [
          ['_toolset=="host"', {
            'cflags': [ '-m32' ],
            'ldflags': [ '-m32' ],
            'asflags': [ '-32' ],
            'xcode_settings': {
              'ARCHS': [ 'i386' ],
            },
          }],
        ],
      }],
      ['(OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
         or OS=="netbsd" or OS=="mac" or OS=="android" or OS=="qnx") and \
        (target_arch=="arm64" or target_arch=="x64" or \
         target_arch=="mipsel64")', {
        'target_conditions': [
          ['_toolset=="host"', {
            'cflags': [ '-m64' ],
            'ldflags': [ '-m64' ],
            'asflags': [ '-64' ],
            'xcode_settings': {
              'ARCHS': [ 'x86_64' ],
            },
          }],
        ],
      }],
    ],
    'include_dirs': [
      'source/common',
      'source/i18n',
    ],
    'msvs_disabled_warnings': [4005, 4068, 4355, 4996, 4267],
  },
  'conditions': [
    ['use_system_icu==0 or want_separate_host_toolset==1', {
      'targets': [
        {
          'target_name': 'copy_icudtl_dat',
          'type': 'none',
          # icudtl.dat is the same for both host/target, so this only supports a
          # single toolset. If a target requires that the .dat file be copied
          # to the output directory, it should explicitly depend on this target
          # with the host toolset (like copy_icudtl_dat#host).
          'toolsets': [ 'host' ],
          'copies': [{
            'destination': '<(PRODUCT_DIR)',
            'conditions': [
              ['OS == "android"', {
                'files': [
                  'android/icudtl.dat',
                ],
              } , { # else: OS != android
                'files': [
                  'source/data/in/icudtl.dat',
                ],
              }],
            ],
          }],
        },
        {
          'target_name': 'icudata',
          'type': 'static_library',
          'defines': [
            'U_HIDE_DATA_SYMBOL',
          ],
          'sources': [
             # These are hand-generated, but will do for now.  The linux
             # version is an identical copy of the (mac) icudtl_dat.S file,
             # modulo removal of the .private_extern and .const directives and
             # with no leading underscore on the icudt52_dat symbol.
             'android/icudtl_dat.S',
             'linux/icudtl_dat.S',
             'mac/icudtl_dat.S',
          ],
          'conditions': [
            [ 'use_system_icu==1 and want_separate_host_toolset==1', {
              'toolsets': ['host'],
            }],
            [ 'use_system_icu==0 and want_separate_host_toolset==1', {
              'toolsets': ['host', 'target'],
            }],
            [ 'use_system_icu==0 and want_separate_host_toolset==0', {
              'toolsets': ['target'],
            }],
            [ 'OS == "win" and icu_use_data_file_flag==0', {
              'type': 'none',
              'copies': [
                {
                  'destination': '<(PRODUCT_DIR)',
                  'files': [
                    'windows/icudt.dll',
                  ],
                },
              ],
            }],
            [ 'icu_use_data_file_flag==1', {
              # Remove any assembly data file.
              'sources/': [['exclude', 'icudtl_dat']],
              # Compile in the stub data symbol.
              'sources': ['source/stubdata/stubdata.c'],

              # Make sure any binary depending on this gets the data file.
              'conditions': [
                ['OS != "ios"', {
                  'dependencies': [
                    'copy_icudtl_dat#host',
                  ],
                } , { # else: OS=="ios"
                  'link_settings': {
                    'mac_bundle_resources': [
                      'source/data/in/icudtl.dat',
                    ],
                  },
                }], # OS!=ios
              ], # conditions
            }], # icu_use_data_file_flag
          ], # conditions
          'target_conditions': [
            [ 'OS == "win" or OS == "mac" or OS == "ios" or '
              '(OS == "android" and (_toolset != "host" or host_os != "linux")) or '
              '(OS == "qnx" and (_toolset == "host" and host_os != "linux"))', {
              'sources!': ['linux/icudtl_dat.S'],
            }],
            [ 'OS != "android" or _toolset == "host"', {
              'sources!': ['android/icudtl_dat.S'],
            }],
            [ 'OS != "mac" and OS != "ios" and '
              '((OS != "android" and OS != "qnx") or '
              '_toolset != "host" or host_os != "mac")', {
              'sources!': ['mac/icudtl_dat.S'],
            }],
          ], # target_conditions
        },
        {
          'target_name': 'icui18n',
          'type': '<(component)',
          'sources': [
            '<@(icui18n_sources)',
          ],
          'defines': [
            'U_I18N_IMPLEMENTATION',
          ],
          'dependencies': [
            'icuuc',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'source/i18n',
            ],
          },
          # Since ICU wants to internally use its own deprecated APIs, don't
          # complain about it.
          'cflags': [
            '-Wno-deprecated-declarations',
          ],
          'cflags_cc': [
            '-frtti',
          ],
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES',       # -frtti
          },
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeTypeInfo': 'true',
            },
          },
          'conditions': [
            [ 'use_system_icu==1 and want_separate_host_toolset==1', {
              'toolsets': ['host'],
            }],
            [ 'use_system_icu==0 and want_separate_host_toolset==1', {
              'toolsets': ['host', 'target'],
            }],
            [ 'use_system_icu==0 and want_separate_host_toolset==0', {
              'toolsets': ['target'],
            }],
            ['clang==1', {
              'xcode_settings': {
                'WARNING_CFLAGS': [
                  # ICU uses its own deprecated functions.
                  '-Wno-deprecated-declarations',
                  # ICU prefers `a && b || c` over `(a && b) || c`.
                  '-Wno-logical-op-parentheses',
                  # ICU has some `unsigned < 0` checks.
                  '-Wno-tautological-compare',
                  # Looks like a real issue, see http://crbug.com/114660
                  '-Wno-return-type-c-linkage',
                ],
              },
              'cflags': [
                '-Wno-deprecated-declarations',
                '-Wno-logical-op-parentheses',
                '-Wno-tautological-compare',
                '-Wno-return-type-c-linkage',
              ],
            }],
            ['OS == "android" and clang==0', {
                # Disable sincos() optimization to avoid a linker error since
                # Android's math library doesn't have sincos().  Either
                # -fno-builtin-sin or -fno-builtin-cos works.
                'cflags': [
                    '-fno-builtin-sin',
                ],
            }],
            ['OS == "android" and use_system_stlport == 1', {
              'target_conditions': [
                ['_toolset == "target"', {
                  # ICU requires RTTI, which is not present in the system's
                  # stlport, so we have to include gabi++.
                  'include_dirs': [
                    '<(android_src)/abi/cpp/include',
                  ],
                  'link_settings': {
                    'libraries': [
                      '-lgabi++',
                    ],
                  },
                }],
              ],
            }],
          ], # conditions
        },
        {
          'target_name': 'icuuc',
          'type': '<(component)',
          'sources': [
            '<@(icuuc_sources)',
          ],
          'defines': [
            'U_COMMON_IMPLEMENTATION',
          ],
          'dependencies': [
            'icudata',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              'source/common',
            ],
            'conditions': [
              [ 'component=="static_library"', {
                'defines': [
                  'U_STATIC_IMPLEMENTATION',
                ],
              }],
            ],
          },
          'cflags': [
            # Since ICU wants to internally use its own deprecated APIs,
            # don't complain about it.
            '-Wno-deprecated-declarations',
            '-Wno-unused-function',
          ],
          'cflags_cc': [
            '-frtti',
          ],
          'xcode_settings': {
            'GCC_ENABLE_CPP_RTTI': 'YES',       # -frtti
          },
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeTypeInfo': 'true',
            },
          },
          'all_dependent_settings': {
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalDependencies': [
                  'advapi32.lib',
                ],
              },
            },
          },
          'conditions': [
            [ 'use_system_icu==1 and want_separate_host_toolset==1', {
              'toolsets': ['host'],
            }],
            [ 'use_system_icu==0 and want_separate_host_toolset==1', {
              'toolsets': ['host', 'target'],
            }],
            [ 'use_system_icu==0 and want_separate_host_toolset==0', {
              'toolsets': ['target'],
            }],
            [ 'OS == "win"', {
              'sources': [
                'source/stubdata/stubdata.c',
              ],
            }],
            ['OS == "android" and use_system_stlport == 1', {
              'target_conditions': [
                ['_toolset == "target"', {
                  # ICU requires RTTI, which is not present in the system's
                  # stlport, so we have to include gabi++.
                  'include_dirs': [
                    '<(android_src)/abi/cpp/include',
                  ],
                  'link_settings': {
                    'libraries': [
                      '-lgabi++',
                    ],
                  },
                }],
              ],
            }],
            ['clang==1', {
              'xcode_settings': {
                'WARNING_CFLAGS': [
                  # ICU uses its own deprecated functions.
                  '-Wno-deprecated-declarations',
                  # ICU prefers `a && b || c` over `(a && b) || c`.
                  '-Wno-logical-op-parentheses',
                  # ICU has some `unsigned < 0` checks.
                  '-Wno-tautological-compare',
                  # uresdata.c has switch(RES_GET_TYPE(x)) code. The
                  # RES_GET_TYPE macro returns an UResType enum, but some switch
                  # statement contains case values that aren't part of that
                  # enum (e.g. URES_TABLE32 which is in UResInternalType). This
                  # is on purpose.
                  '-Wno-switch',
                ],
              },
              'cflags': [
                '-Wno-deprecated-declarations',
                '-Wno-logical-op-parentheses',
                '-Wno-tautological-compare',
                '-Wno-switch',
              ],
            }],
          ], # conditions
        },
      ], # targets
    }],
    ['use_system_icu==1', {
      'targets': [
        {
          'target_name': 'system_icu',
          'type': 'none',
          'conditions': [
            ['OS=="android"', {
              'direct_dependent_settings': {
                'include_dirs': [
                  '<(android_src)/external/icu4c/common',
                  '<(android_src)/external/icu4c/i18n',
                ],
              },
              'link_settings': {
                'libraries': [
                  '-licui18n',
                  '-licuuc',
                ],
              },
            }],
            ['OS=="qnx"', {
              'link_settings': {
                'libraries': [
                  '-licui18n',
                  '-licuuc',
                ],
              },
            }],
            ['OS!="android" and OS!="qnx"', {
              'link_settings': {
                'ldflags': [
                  '<!@(icu-config --ldflags)',
                ],
                'libraries': [
                  '<!@(icu-config --ldflags-libsonly)',
                ],
              },
            }],
          ],
        },
        {
          'target_name': 'icudata',
          'type': 'none',
          'dependencies': ['system_icu'],
          'export_dependent_settings': ['system_icu'],
          'toolsets': ['target'],
        },
        {
          'target_name': 'icui18n',
          'type': 'none',
          'dependencies': ['system_icu'],
          'export_dependent_settings': ['system_icu'],
          'variables': {
            'headers_root_path': 'source/i18n',
            'header_filenames': [
              # This list can easily be updated using the command below:
              # find third_party/icu/source/i18n/unicode -iname '*.h' \
              # -printf "'%p',\n" | \
              # sed -e 's|third_party/icu/source/i18n/||' | sort -u
              'unicode/alphaindex.h',
              'unicode/basictz.h',
              'unicode/calendar.h',
              'unicode/choicfmt.h',
              'unicode/coleitr.h',
              'unicode/coll.h',
              'unicode/compactdecimalformat.h',
              'unicode/curramt.h',
              'unicode/currpinf.h',
              'unicode/currunit.h',
              'unicode/datefmt.h',
              'unicode/dcfmtsym.h',
              'unicode/decimfmt.h',
              'unicode/dtfmtsym.h',
              'unicode/dtitvfmt.h',
              'unicode/dtitvinf.h',
              'unicode/dtptngen.h',
              'unicode/dtrule.h',
              'unicode/fieldpos.h',
              'unicode/fmtable.h',
              'unicode/format.h',
              'unicode/fpositer.h',
              'unicode/gender.h',
              'unicode/gregocal.h',
              'unicode/locdspnm.h',
              'unicode/measfmt.h',
              'unicode/measunit.h',
              'unicode/measure.h',
              'unicode/msgfmt.h',
              'unicode/numfmt.h',
              'unicode/numsys.h',
              'unicode/plurfmt.h',
              'unicode/plurrule.h',
              'unicode/rbnf.h',
              'unicode/rbtz.h',
              'unicode/regex.h',
              'unicode/region.h',
              'unicode/search.h',
              'unicode/selfmt.h',
              'unicode/simpletz.h',
              'unicode/smpdtfmt.h',
              'unicode/sortkey.h',
              'unicode/stsearch.h',
              'unicode/tblcoll.h',
              'unicode/timezone.h',
              'unicode/tmunit.h',
              'unicode/tmutamt.h',
              'unicode/tmutfmt.h',
              'unicode/translit.h',
              'unicode/tzfmt.h',
              'unicode/tznames.h',
              'unicode/tzrule.h',
              'unicode/tztrans.h',
              'unicode/ucal.h',
              'unicode/ucoleitr.h',
              'unicode/ucol.h',
              'unicode/ucsdet.h',
              'unicode/ucurr.h',
              'unicode/udateintervalformat.h',
              'unicode/udat.h',
              'unicode/udatpg.h',
              'unicode/udisplaycontext.h',
              'unicode/uformattable.h',
              'unicode/ugender.h',
              'unicode/uldnames.h',
              'unicode/ulocdata.h',
              'unicode/umsg.h',
              'unicode/unirepl.h',
              'unicode/unum.h',
              'unicode/unumsys.h',
              'unicode/upluralrules.h',
              'unicode/uregex.h',
              'unicode/uregion.h',
              'unicode/usearch.h',
              'unicode/uspoof.h',
              'unicode/utmscale.h',
              'unicode/utrans.h',
              'unicode/vtzone.h',
            ],
          },
          'includes': [
            '../../build/shim_headers.gypi',
          ],
          'toolsets': ['target'],
        },
        {
          'target_name': 'icuuc',
          'type': 'none',
          'dependencies': ['system_icu'],
          'export_dependent_settings': ['system_icu'],
          'variables': {
            'headers_root_path': 'source/common',
            'header_filenames': [
              # This list can easily be updated using the command below:
              # find third_party/icu/source/common/unicode -iname '*.h' \
              # -printf "'%p',\n" | \
              # sed -e 's|third_party/icu/source/common/||' | sort -u
              'unicode/appendable.h',
              'unicode/brkiter.h',
              'unicode/bytestream.h',
              'unicode/bytestriebuilder.h',
              'unicode/bytestrie.h',
              'unicode/caniter.h',
              'unicode/chariter.h',
              'unicode/dbbi.h',
              'unicode/docmain.h',
              'unicode/dtintrv.h',
              'unicode/enumset.h',
              'unicode/errorcode.h',
              'unicode/icudataver.h',
              'unicode/icuplug.h',
              'unicode/idna.h',
              'unicode/listformatter.h',
              'unicode/localpointer.h',
              'unicode/locid.h',
              'unicode/messagepattern.h',
              'unicode/normalizer2.h',
              'unicode/normlzr.h',
              'unicode/parseerr.h',
              'unicode/parsepos.h',
              'unicode/platform.h',
              'unicode/ptypes.h',
              'unicode/putil.h',
              'unicode/rbbi.h',
              'unicode/rep.h',
              'unicode/resbund.h',
              'unicode/schriter.h',
              'unicode/std_string.h',
              'unicode/strenum.h',
              'unicode/stringpiece.h',
              'unicode/stringtriebuilder.h',
              'unicode/symtable.h',
              'unicode/ubidi.h',
              'unicode/ubrk.h',
              'unicode/ucasemap.h',
              'unicode/ucat.h',
              'unicode/uchar.h',
              'unicode/ucharstriebuilder.h',
              'unicode/ucharstrie.h',
              'unicode/uchriter.h',
              'unicode/uclean.h',
              'unicode/ucnv_cb.h',
              'unicode/ucnv_err.h',
              'unicode/ucnv.h',
              'unicode/ucnvsel.h',
              'unicode/uconfig.h',
              'unicode/udata.h',
              'unicode/uenum.h',
              'unicode/uidna.h',
              'unicode/uiter.h',
              'unicode/uloc.h',
              'unicode/umachine.h',
              'unicode/umisc.h',
              'unicode/unifilt.h',
              'unicode/unifunct.h',
              'unicode/unimatch.h',
              'unicode/uniset.h',
              'unicode/unistr.h',
              'unicode/unorm2.h',
              'unicode/unorm.h',
              'unicode/uobject.h',
              'unicode/urename.h',
              'unicode/urep.h',
              'unicode/ures.h',
              'unicode/uscript.h',
              'unicode/uset.h',
              'unicode/usetiter.h',
              'unicode/ushape.h',
              'unicode/usprep.h',
              'unicode/ustring.h',
              'unicode/ustringtrie.h',
              'unicode/utext.h',
              'unicode/utf16.h',
              'unicode/utf32.h',
              'unicode/utf8.h',
              'unicode/utf.h',
              'unicode/utf_old.h',
              'unicode/utrace.h',
              'unicode/utypes.h',
              'unicode/uvernum.h',
              'unicode/uversion.h',
            ],
          },
          'includes': [
            '../../build/shim_headers.gypi',
          ],
          'toolsets': ['target'],
        },
      ], # targets
    }],
  ], # conditions
}

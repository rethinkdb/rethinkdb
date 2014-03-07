# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
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
      ],
    },
    'defines': [
      'U_USING_ICU_NAMESPACE=0',
    ],
    'conditions': [
      ['component=="static_library"', {
        'defines': [
          'U_STATIC_IMPLEMENTATION',
        ],
      }],
      ['(OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris" \
         or OS=="netbsd" or OS=="mac" or OS=="android") and \
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
          'target_name': 'icudata',
          'type': 'static_library',
          'defines': [
            'U_HIDE_DATA_SYMBOL',
          ],
          'sources': [
             # These are hand-generated, but will do for now.  The linux
             # version is an identical copy of the (mac) icudt46l_dat.S file,
             # modulo removal of the .private_extern and .const directives and
             # with no leading underscore on the icudt46_dat symbol.
             'android/icudt46l_dat.S',
             'linux/icudt46l_dat.S',
             'mac/icudt46l_dat.S',
          ],
          'conditions': [
            [ 'use_system_icu==1', {
              'toolsets': ['host'],
            }, {
              'toolsets': ['host', 'target'],
            }],
            [ 'OS == "win"', {
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
            [ 'OS != "win" and icu_use_data_file_flag', {
              # Remove any assembly data file.
              'sources/': [['exclude', 'icudt46l_dat']],
              # Compile in the stub data symbol.
              'sources': ['source/stubdata/stubdata.c'],
              # Make sure any binary depending on this gets the data file.
              'link_settings': {
                'target_conditions': [
                  ['(OS == "mac" and _mac_bundle) or OS=="ios"', {
                    'mac_bundle_resources': [
                      'source/data/in/icudt46l.dat',
                    ],
                  }, {
                    'copies': [{
                      'destination': '<(PRODUCT_DIR)',
                      'files': [
                        'source/data/in/icudt46l.dat',
                      ],
                    }],
                  }],
                ],  # target_conditions
              },  # link_settings
            }],
          ],
          'target_conditions': [
            [ 'OS == "win" or OS == "mac" or OS == "ios" or '
              '(OS == "android" and (_toolset == "target" or host_os != "linux"))', {
              'sources!': ['linux/icudt46l_dat.S'],
            }],
            [ 'OS != "android" or _toolset == "host"', {
              'sources!': ['android/icudt46l_dat.S'],
            }],
            [ 'OS != "mac" and OS != "ios" and '
              '(OS != "android" or _toolset != "host" or host_os != "mac")', {
              'sources!': ['mac/icudt46l_dat.S'],
            }],
          ],
        },
        {
          'target_name': 'icui18n',
          'type': '<(component)',
          'sources': [
            'source/i18n/anytrans.cpp',
            'source/i18n/astro.cpp',
            'source/i18n/basictz.cpp',
            'source/i18n/bms.cpp',
            'source/i18n/bmsearch.cpp',
            'source/i18n/bocsu.c',
            'source/i18n/brktrans.cpp',
            'source/i18n/buddhcal.cpp',
            'source/i18n/calendar.cpp',
            'source/i18n/casetrn.cpp',
            'source/i18n/cecal.cpp',
            'source/i18n/chnsecal.cpp',
            'source/i18n/choicfmt.cpp',
            'source/i18n/coleitr.cpp',
            'source/i18n/coll.cpp',
            'source/i18n/colldata.cpp',
            'source/i18n/coptccal.cpp',
            'source/i18n/cpdtrans.cpp',
            'source/i18n/csdetect.cpp',
            'source/i18n/csmatch.cpp',
            'source/i18n/csr2022.cpp',
            'source/i18n/csrecog.cpp',
            'source/i18n/csrmbcs.cpp',
            'source/i18n/csrsbcs.cpp',
            'source/i18n/csrucode.cpp',
            'source/i18n/csrutf8.cpp',
            'source/i18n/curramt.cpp',
            'source/i18n/currfmt.cpp',
            'source/i18n/currpinf.cpp',
            'source/i18n/currunit.cpp',
            'source/i18n/datefmt.cpp',
            'source/i18n/dcfmtsym.cpp',
            'source/i18n/decContext.c',
            'source/i18n/decNumber.c',
            'source/i18n/decimfmt.cpp',
            'source/i18n/digitlst.cpp',
            'source/i18n/dtfmtsym.cpp',
            'source/i18n/dtitvfmt.cpp',
            'source/i18n/dtitvinf.cpp',
            'source/i18n/dtptngen.cpp',
            'source/i18n/dtrule.cpp',
            'source/i18n/esctrn.cpp',
            'source/i18n/ethpccal.cpp',
            'source/i18n/fmtable.cpp',
            'source/i18n/fmtable_cnv.cpp',
            'source/i18n/format.cpp',
            'source/i18n/fphdlimp.cpp',
            'source/i18n/fpositer.cpp',
            'source/i18n/funcrepl.cpp',
            'source/i18n/gregocal.cpp',
            'source/i18n/gregoimp.cpp',
            'source/i18n/hebrwcal.cpp',
            'source/i18n/indiancal.cpp',
            'source/i18n/inputext.cpp',
            'source/i18n/islamcal.cpp',
            'source/i18n/japancal.cpp',
            'source/i18n/locdspnm.cpp',
            'source/i18n/measfmt.cpp',
            'source/i18n/measure.cpp',
            'source/i18n/msgfmt.cpp',
            'source/i18n/name2uni.cpp',
            'source/i18n/nfrs.cpp',
            'source/i18n/nfrule.cpp',
            'source/i18n/nfsubs.cpp',
            'source/i18n/nortrans.cpp',
            'source/i18n/nultrans.cpp',
            'source/i18n/numfmt.cpp',
            'source/i18n/numsys.cpp',
            'source/i18n/olsontz.cpp',
            'source/i18n/persncal.cpp',
            'source/i18n/plurfmt.cpp',
            'source/i18n/plurrule.cpp',
            'source/i18n/quant.cpp',
            'source/i18n/rbnf.cpp',
            'source/i18n/rbt.cpp',
            'source/i18n/rbt_data.cpp',
            'source/i18n/rbt_pars.cpp',
            'source/i18n/rbt_rule.cpp',
            'source/i18n/rbt_set.cpp',
            'source/i18n/rbtz.cpp',
            'source/i18n/regexcmp.cpp',
            'source/i18n/regexst.cpp',
            'source/i18n/regextxt.cpp',
            'source/i18n/reldtfmt.cpp',
            'source/i18n/rematch.cpp',
            'source/i18n/remtrans.cpp',
            'source/i18n/repattrn.cpp',
            'source/i18n/search.cpp',
            'source/i18n/selfmt.cpp',
            'source/i18n/simpletz.cpp',
            'source/i18n/smpdtfmt.cpp',
            'source/i18n/sortkey.cpp',
            'source/i18n/strmatch.cpp',
            'source/i18n/strrepl.cpp',
            'source/i18n/stsearch.cpp',
            'source/i18n/taiwncal.cpp',
            'source/i18n/tblcoll.cpp',
            'source/i18n/timezone.cpp',
            'source/i18n/titletrn.cpp',
            'source/i18n/tmunit.cpp',
            'source/i18n/tmutamt.cpp',
            'source/i18n/tmutfmt.cpp',
            'source/i18n/tolowtrn.cpp',
            'source/i18n/toupptrn.cpp',
            'source/i18n/translit.cpp',
            'source/i18n/transreg.cpp',
            'source/i18n/tridpars.cpp',
            'source/i18n/tzrule.cpp',
            'source/i18n/tztrans.cpp',
            'source/i18n/ucal.cpp',
            'source/i18n/ucln_in.c',
            'source/i18n/ucol.cpp',
            'source/i18n/ucol_bld.cpp',
            'source/i18n/ucol_cnt.cpp',
            'source/i18n/ucol_elm.cpp',
            'source/i18n/ucol_res.cpp',
            'source/i18n/ucol_sit.cpp',
            'source/i18n/ucol_tok.cpp',
            'source/i18n/ucol_wgt.cpp',
            'source/i18n/ucoleitr.cpp',
            'source/i18n/ucsdet.cpp',
            'source/i18n/ucurr.cpp',
            'source/i18n/udat.cpp',
            'source/i18n/udatpg.cpp',
            'source/i18n/ulocdata.c',
            'source/i18n/umsg.cpp',
            'source/i18n/unesctrn.cpp',
            'source/i18n/uni2name.cpp',
            'source/i18n/unum.cpp',
            'source/i18n/uregex.cpp',
            'source/i18n/uregexc.cpp',
            'source/i18n/usearch.cpp',
            'source/i18n/uspoof.cpp',
            'source/i18n/uspoof_build.cpp',
            'source/i18n/uspoof_conf.cpp',
            'source/i18n/uspoof_impl.cpp',
            'source/i18n/uspoof_wsconf.cpp',
            'source/i18n/utmscale.c',
            'source/i18n/utrans.cpp',
            'source/i18n/vtzone.cpp',
            'source/i18n/vzone.cpp',
            'source/i18n/windtfmt.cpp',
            'source/i18n/winnmfmt.cpp',
            'source/i18n/wintzimpl.cpp',
            'source/i18n/zonemeta.cpp',
            'source/i18n/zrule.cpp',
            'source/i18n/zstrfmt.cpp',
            'source/i18n/ztrans.cpp',
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
          'conditions': [
            [ 'use_system_icu==1', {
              'toolsets': ['host'],
            }, {
              'toolsets': ['host', 'target'],
            }],
            [ 'os_posix == 1 and OS != "mac" and OS != "ios"', {
              # Since ICU wants to internally use its own deprecated APIs, don't
              # complain about it.
              'cflags': [
                '-Wno-deprecated-declarations',
              ],
              'cflags_cc': [
                '-frtti',
              ],
            }],
            ['OS == "mac" or OS == "ios"', {
              'xcode_settings': {
                'GCC_ENABLE_CPP_RTTI': 'YES',       # -frtti
              },
            }],
            ['OS == "win"', {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeTypeInfo': 'true',
                },
              }
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
                  # uspoof.h has a U_NAMESPACE_USE macro. That's a bug,
                  # the header should use U_NAMESPACE_BEGIN instead.
                  # http://bugs.icu-project.org/trac/ticket/9054
                  '-Wno-header-hygiene',
                  # Looks like a real issue, see http://crbug.com/114660
                  '-Wno-return-type-c-linkage',
                ],
              },
              'cflags': [
                '-Wno-deprecated-declarations',
                '-Wno-logical-op-parentheses',
                '-Wno-tautological-compare',
                '-Wno-header-hygiene',
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
          ],
        },
        {
          'target_name': 'icuuc',
          'type': '<(component)',
          'sources': [
            'source/common/bmpset.cpp',
            'source/common/brkeng.cpp',
            'source/common/brkiter.cpp',
            'source/common/bytestream.cpp',
            'source/common/caniter.cpp',
            'source/common/chariter.cpp',
            'source/common/charstr.cpp',
            'source/common/cmemory.c',
            'source/common/cstring.c',
            'source/common/cwchar.c',
            'source/common/dictbe.cpp',
            'source/common/dtintrv.cpp',
            'source/common/errorcode.cpp',
            'source/common/filterednormalizer2.cpp',
            'source/common/icudataver.c',
            'source/common/icuplug.c',
            'source/common/locavailable.cpp',
            'source/common/locbased.cpp',
            'source/common/locdispnames.cpp',
            'source/common/locid.cpp',
            'source/common/loclikely.cpp',
            'source/common/locmap.c',
            'source/common/locresdata.cpp',
            'source/common/locutil.cpp',
            'source/common/mutex.cpp',
            'source/common/normalizer2.cpp',
            'source/common/normalizer2impl.cpp',
            'source/common/normlzr.cpp',
            'source/common/parsepos.cpp',
            'source/common/propname.cpp',
            'source/common/propsvec.c',
            'source/common/punycode.c',
            'source/common/putil.c',
            'source/common/rbbi.cpp',
            'source/common/rbbidata.cpp',
            'source/common/rbbinode.cpp',
            'source/common/rbbirb.cpp',
            'source/common/rbbiscan.cpp',
            'source/common/rbbisetb.cpp',
            'source/common/rbbistbl.cpp',
            'source/common/rbbitblb.cpp',
            'source/common/resbund.cpp',
            'source/common/resbund_cnv.cpp',
            'source/common/ruleiter.cpp',
            'source/common/schriter.cpp',
            'source/common/serv.cpp',
            'source/common/servlk.cpp',
            'source/common/servlkf.cpp',
            'source/common/servls.cpp',
            'source/common/servnotf.cpp',
            'source/common/servrbf.cpp',
            'source/common/servslkf.cpp',
            'source/common/stringpiece.cpp',
            'source/common/triedict.cpp',
            'source/common/uarrsort.c',
            'source/common/ubidi.c',
            'source/common/ubidi_props.c',
            'source/common/ubidiln.c',
            'source/common/ubidiwrt.c',
            'source/common/ubrk.cpp',
            'source/common/ucase.c',
            'source/common/ucasemap.c',
            'source/common/ucat.c',
            'source/common/uchar.c',
            'source/common/uchriter.cpp',
            'source/common/ucln_cmn.c',
            'source/common/ucmndata.c',
            'source/common/ucnv.c',
            'source/common/ucnv2022.c',
            'source/common/ucnv_bld.c',
            'source/common/ucnv_cb.c',
            'source/common/ucnv_cnv.c',
            'source/common/ucnv_err.c',
            'source/common/ucnv_ext.c',
            'source/common/ucnv_io.c',
            'source/common/ucnv_lmb.c',
            'source/common/ucnv_set.c',
            'source/common/ucnv_u16.c',
            'source/common/ucnv_u32.c',
            'source/common/ucnv_u7.c',
            'source/common/ucnv_u8.c',
            'source/common/ucnvbocu.c',
            'source/common/ucnvdisp.c',
            'source/common/ucnvhz.c',
            'source/common/ucnvisci.c',
            'source/common/ucnvlat1.c',
            'source/common/ucnvmbcs.c',
            'source/common/ucnvscsu.c',
            'source/common/ucnvsel.cpp',
            'source/common/ucol_swp.cpp',
            'source/common/udata.cpp',
            'source/common/udatamem.c',
            'source/common/udataswp.c',
            'source/common/uenum.c',
            'source/common/uhash.c',
            'source/common/uhash_us.cpp',
            'source/common/uidna.cpp',
            'source/common/uinit.c',
            'source/common/uinvchar.c',
            'source/common/uiter.cpp',
            'source/common/ulist.c',
            'source/common/uloc.c',
            'source/common/uloc_tag.c',
            'source/common/umapfile.c',
            'source/common/umath.c',
            'source/common/umutex.c',
            'source/common/unames.c',
            'source/common/unifilt.cpp',
            'source/common/unifunct.cpp',
            'source/common/uniset.cpp',
            'source/common/uniset_props.cpp',
            'source/common/unisetspan.cpp',
            'source/common/unistr.cpp',
            'source/common/unistr_case.cpp',
            'source/common/unistr_cnv.cpp',
            'source/common/unistr_props.cpp',
            'source/common/unorm.cpp',
            'source/common/unorm_it.c',
            'source/common/unormcmp.cpp',
            'source/common/uobject.cpp',
            'source/common/uprops.cpp',
            'source/common/ures_cnv.c',
            'source/common/uresbund.c',
            'source/common/uresdata.c',
            'source/common/usc_impl.c',
            'source/common/uscript.c',
            'source/common/uset.cpp',
            'source/common/uset_props.cpp',
            'source/common/usetiter.cpp',
            'source/common/ushape.c',
            'source/common/usprep.cpp',
            'source/common/ustack.cpp',
            'source/common/ustr_cnv.c',
            'source/common/ustr_wcs.c',
            'source/common/ustrcase.c',
            'source/common/ustrenum.cpp',
            'source/common/ustrfmt.c',
            'source/common/ustring.c',
            'source/common/ustrtrns.c',
            'source/common/utext.cpp',
            'source/common/utf_impl.c',
            'source/common/util.cpp',
            'source/common/util_props.cpp',
            'source/common/utrace.c',
            'source/common/utrie.c',
            'source/common/utrie2.cpp',
            'source/common/utrie2_builder.c',
            'source/common/uts46.cpp',
            'source/common/utypes.c',
            'source/common/uvector.cpp',
            'source/common/uvectr32.cpp',
            'source/common/uvectr64.cpp',
            'source/common/wintz.c',
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
          'conditions': [
            [ 'use_system_icu==1', {
              'toolsets': ['host'],
            }, {
              'toolsets': ['host', 'target'],
            }],
            [ 'OS == "win"', {
              'sources': [
                'source/stubdata/stubdata.c',
              ],
            }],
            [ 'os_posix == 1 and OS != "mac" and OS != "ios"', {
              'cflags': [
                # Since ICU wants to internally use its own deprecated APIs,
                # don't complain about it.
                '-Wno-deprecated-declarations',
                '-Wno-unused-function',
              ],
              'cflags_cc': [
                '-frtti',
              ],
            }],
            ['OS == "mac" or OS == "ios"', {
              'xcode_settings': {
                'GCC_ENABLE_CPP_RTTI': 'YES',       # -frtti
              },
            }],
            ['OS == "win"', {
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'RuntimeTypeInfo': 'true',
                },
              },
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
          ],
        },
      ],
    }],
    ['use_system_icu==1', {
      'targets': [
        {
          'target_name': 'system_icu',
          'type': 'none',
          'conditions': [
            ['want_separate_host_toolset==1', {
              'toolsets': ['target'],
            }, {
              'toolsets': ['host', 'target'],
            }],
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
            },{ # OS!="android"
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
          'conditions': [
            ['want_separate_host_toolset==1', {
              'toolsets': ['target'],
            }, {
              'toolsets': ['host', 'target'],
            }],
          ],
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
              'unicode/basictz.h',
              'unicode/bmsearch.h',
              'unicode/bms.h',
              'unicode/calendar.h',
              'unicode/choicfmt.h',
              'unicode/coleitr.h',
              'unicode/colldata.h',
              'unicode/coll.h',
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
              'unicode/tzrule.h',
              'unicode/tztrans.h',
              'unicode/ucal.h',
              'unicode/ucoleitr.h',
              'unicode/ucol.h',
              'unicode/ucsdet.h',
              'unicode/ucurr.h',
              'unicode/udat.h',
              'unicode/udatpg.h',
              'unicode/uldnames.h',
              'unicode/ulocdata.h',
              'unicode/umsg.h',
              'unicode/unirepl.h',
              'unicode/unum.h',
              'unicode/uregex.h',
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
          'conditions': [
            ['want_separate_host_toolset==1', {
              'toolsets': ['target'],
            }, {
              'toolsets': ['host', 'target'],
            }],
          ],
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
              'unicode/brkiter.h',
              'unicode/bytestream.h',
              'unicode/caniter.h',
              'unicode/chariter.h',
              'unicode/dbbi.h',
              'unicode/docmain.h',
              'unicode/dtintrv.h',
              'unicode/errorcode.h',
              'unicode/icudataver.h',
              'unicode/icuplug.h',
              'unicode/idna.h',
              'unicode/localpointer.h',
              'unicode/locid.h',
              'unicode/normalizer2.h',
              'unicode/normlzr.h',
              'unicode/pandroid.h',
              'unicode/parseerr.h',
              'unicode/parsepos.h',
              'unicode/pfreebsd.h',
              'unicode/plinux.h',
              'unicode/pmac.h',
              'unicode/popenbsd.h',
              'unicode/ppalmos.h',
              'unicode/ptypes.h',
              'unicode/putil.h',
              'unicode/pwin32.h',
              'unicode/rbbi.h',
              'unicode/rep.h',
              'unicode/resbund.h',
              'unicode/schriter.h',
              'unicode/std_string.h',
              'unicode/strenum.h',
              'unicode/stringpiece.h',
              'unicode/symtable.h',
              'unicode/ubidi.h',
              'unicode/ubrk.h',
              'unicode/ucasemap.h',
              'unicode/ucat.h',
              'unicode/uchar.h',
              'unicode/uchriter.h',
              'unicode/uclean.h',
              'unicode/ucnv_cb.h',
              'unicode/ucnv_err.h',
              'unicode/ucnv.h',
              'unicode/ucnvsel.h',
              'unicode/uconfig.h',
              'unicode/udata.h',
              'unicode/udeprctd.h',
              'unicode/udraft.h',
              'unicode/uenum.h',
              'unicode/uidna.h',
              'unicode/uintrnal.h',
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
              'unicode/uobslete.h',
              'unicode/urename.h',
              'unicode/urep.h',
              'unicode/ures.h',
              'unicode/uscript.h',
              'unicode/uset.h',
              'unicode/usetiter.h',
              'unicode/ushape.h',
              'unicode/usprep.h',
              'unicode/ustring.h',
              'unicode/usystem.h',
              'unicode/utext.h',
              'unicode/utf16.h',
              'unicode/utf32.h',
              'unicode/utf8.h',
              'unicode/utf.h',
              'unicode/utf_old.h',
              'unicode/utrace.h',
              'unicode/utypeinfo.h',
              'unicode/utypes.h',
              'unicode/uvernum.h',
              'unicode/uversion.h',
            ],
          },
          'includes': [
            '../../build/shim_headers.gypi',
          ],
          'conditions': [
            ['want_separate_host_toolset==1', {
              'toolsets': ['target'],
            }, {
              'toolsets': ['host', 'target'],
            }],
          ],
        },
      ],
    }],
  ],
}

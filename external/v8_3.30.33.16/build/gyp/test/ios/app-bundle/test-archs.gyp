# Copyright (c) 2013 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'make_global_settings': [
    ['CC', '/usr/bin/clang'],
  ],
  'targets': [
    {
      'target_name': 'test_no_archs',
      'product_name': 'Test No Archs',
      'type': 'executable',
      'product_extension': 'bundle',
      'mac_bundle': 1,
      'sources': [
        'TestApp/main.m',
        'TestApp/only-compile-in-32-bits.m',
      ],
      'mac_bundle_resources': [
        'TestApp/English.lproj/InfoPlist.strings',
        'TestApp/English.lproj/MainMenu.xib',
      ],
      'link_settings': {
        'libraries': [
          '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
          '$(SDKROOT)/System/Library/Frameworks/UIKit.framework',
        ],
      },
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-fobjc-abi-version=2',
        ],
        'SDKROOT': 'iphonesimulator',  # -isysroot
        'TARGETED_DEVICE_FAMILY': '1,2',
        'INFOPLIST_FILE': 'TestApp/TestApp-Info.plist',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'CONFIGURATION_BUILD_DIR':'build/Default',
      },
    },
    {
      'target_name': 'test_archs_i386',
      'product_name': 'Test Archs i386',
      'type': 'executable',
      'product_extension': 'bundle',
      'mac_bundle': 1,
      'sources': [
        'TestApp/main.m',
        'TestApp/only-compile-in-32-bits.m',
      ],
      'mac_bundle_resources': [
        'TestApp/English.lproj/InfoPlist.strings',
        'TestApp/English.lproj/MainMenu.xib',
      ],
      'link_settings': {
        'libraries': [
          '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
          '$(SDKROOT)/System/Library/Frameworks/UIKit.framework',
        ],
      },
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-fobjc-abi-version=2',
        ],
        'SDKROOT': 'iphonesimulator',  # -isysroot
        'TARGETED_DEVICE_FAMILY': '1,2',
        'INFOPLIST_FILE': 'TestApp/TestApp-Info.plist',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'CONFIGURATION_BUILD_DIR':'build/Default',
        'ARCHS': ['i386'],
      },
    },
    {
      'target_name': 'test_archs_x86_64',
      'product_name': 'Test Archs x86_64',
      'type': 'executable',
      'product_extension': 'bundle',
      'mac_bundle': 1,
      'sources': [
        'TestApp/main.m',
        'TestApp/only-compile-in-64-bits.m',
      ],
      'mac_bundle_resources': [
        'TestApp/English.lproj/InfoPlist.strings',
        'TestApp/English.lproj/MainMenu.xib',
      ],
      'link_settings': {
        'libraries': [
          '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
          '$(SDKROOT)/System/Library/Frameworks/UIKit.framework',
        ],
      },
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-fobjc-abi-version=2',
        ],
        'SDKROOT': 'iphonesimulator',  # -isysroot
        'TARGETED_DEVICE_FAMILY': '1,2',
        'INFOPLIST_FILE': 'TestApp/TestApp-Info.plist',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'CONFIGURATION_BUILD_DIR':'build/Default',
        'ARCHS': ['x86_64'],
      },
    },
  ],
}

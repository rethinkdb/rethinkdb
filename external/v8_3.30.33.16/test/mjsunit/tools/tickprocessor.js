// Copyright 2009 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Load implementations from <project root>/tools.
// Files: tools/splaytree.js tools/codemap.js tools/csvparser.js
// Files: tools/consarray.js tools/profile.js tools/profile_view.js
// Files: tools/logreader.js tools/tickprocessor.js
// Env: TEST_FILE_NAME


(function testArgumentsProcessor() {
  var p_default = new ArgumentsProcessor([]);
  assertTrue(p_default.parse());
  assertEquals(ArgumentsProcessor.DEFAULTS, p_default.result());

  var p_logFile = new ArgumentsProcessor(['logfile.log']);
  assertTrue(p_logFile.parse());
  assertEquals('logfile.log', p_logFile.result().logFileName);

  var p_platformAndLog = new ArgumentsProcessor(['--windows', 'winlog.log']);
  assertTrue(p_platformAndLog.parse());
  assertEquals('windows', p_platformAndLog.result().platform);
  assertEquals('winlog.log', p_platformAndLog.result().logFileName);

  var p_flags = new ArgumentsProcessor(['--gc', '--separate-ic']);
  assertTrue(p_flags.parse());
  assertEquals(TickProcessor.VmStates.GC, p_flags.result().stateFilter);
  assertTrue(p_flags.result().separateIc);

  var p_nmAndLog = new ArgumentsProcessor(['--nm=mn', 'nmlog.log']);
  assertTrue(p_nmAndLog.parse());
  assertEquals('mn', p_nmAndLog.result().nm);
  assertEquals('nmlog.log', p_nmAndLog.result().logFileName);

  var p_bad = new ArgumentsProcessor(['--unknown', 'badlog.log']);
  assertFalse(p_bad.parse());
})();


(function testUnixCppEntriesProvider() {
  var oldLoadSymbols = UnixCppEntriesProvider.prototype.loadSymbols;

  // shell executable
  UnixCppEntriesProvider.prototype.loadSymbols = function(libName) {
    this.symbols = [[
      '         U operator delete[](void*)@@GLIBCXX_3.4',
      '08049790 T _init',
      '08049f50 T _start',
      '08139150 00000b4b t v8::internal::Runtime_StringReplaceRegExpWithString(v8::internal::Arguments)',
      '08139ca0 000003f1 T v8::internal::Runtime::GetElementOrCharAt(v8::internal::Handle<v8::internal::Object>, unsigned int)',
      '0813a0b0 00000855 t v8::internal::Runtime_DebugGetPropertyDetails(v8::internal::Arguments)',
      '0818b220 00000036 W v8::internal::RegExpMacroAssembler::CheckPosition(int, v8::internal::Label*)',
      '         w __gmon_start__',
      '081f08a0 00000004 B stdout\n'
    ].join('\n'), ''];
  };

  var shell_prov = new UnixCppEntriesProvider();
  var shell_syms = [];
  shell_prov.parseVmSymbols('shell', 0x08048000, 0x081ee000,
      function (name, start, end) {
        shell_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [['_init', 0x08049790, 0x08049f50],
       ['_start', 0x08049f50, 0x08139150],
       ['v8::internal::Runtime_StringReplaceRegExpWithString(v8::internal::Arguments)', 0x08139150, 0x08139150 + 0xb4b],
       ['v8::internal::Runtime::GetElementOrCharAt(v8::internal::Handle<v8::internal::Object>, unsigned int)', 0x08139ca0, 0x08139ca0 + 0x3f1],
       ['v8::internal::Runtime_DebugGetPropertyDetails(v8::internal::Arguments)', 0x0813a0b0, 0x0813a0b0 + 0x855],
       ['v8::internal::RegExpMacroAssembler::CheckPosition(int, v8::internal::Label*)', 0x0818b220, 0x0818b220 + 0x36]],
      shell_syms);

  // libc library
  UnixCppEntriesProvider.prototype.loadSymbols = function(libName) {
    this.symbols = [[
        '000162a0 00000005 T __libc_init_first',
        '0002a5f0 0000002d T __isnan',
        '0002a5f0 0000002d W isnan',
        '0002aaa0 0000000d W scalblnf',
        '0002aaa0 0000000d W scalbnf',
        '0011a340 00000048 T __libc_thread_freeres',
        '00128860 00000024 R _itoa_lower_digits\n'].join('\n'), ''];
  };
  var libc_prov = new UnixCppEntriesProvider();
  var libc_syms = [];
  libc_prov.parseVmSymbols('libc', 0xf7c5c000, 0xf7da5000,
      function (name, start, end) {
        libc_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  var libc_ref_syms = [['__libc_init_first', 0x000162a0, 0x000162a0 + 0x5],
       ['__isnan', 0x0002a5f0, 0x0002a5f0 + 0x2d],
       ['scalblnf', 0x0002aaa0, 0x0002aaa0 + 0xd],
       ['__libc_thread_freeres', 0x0011a340, 0x0011a340 + 0x48]];
  for (var i = 0; i < libc_ref_syms.length; ++i) {
    libc_ref_syms[i][1] += 0xf7c5c000;
    libc_ref_syms[i][2] += 0xf7c5c000;
  }
  assertEquals(libc_ref_syms, libc_syms);

  UnixCppEntriesProvider.prototype.loadSymbols = oldLoadSymbols;
})();


(function testMacCppEntriesProvider() {
  var oldLoadSymbols = MacCppEntriesProvider.prototype.loadSymbols;

  // shell executable
  MacCppEntriesProvider.prototype.loadSymbols = function(libName) {
    this.symbols = [[
      '         U operator delete[]',
      '00001000 A __mh_execute_header',
      '00001b00 T start',
      '00001b40 t dyld_stub_binding_helper',
      '0011b710 T v8::internal::RegExpMacroAssembler::CheckPosition',
      '00134250 t v8::internal::Runtime_StringReplaceRegExpWithString',
      '00137220 T v8::internal::Runtime::GetElementOrCharAt',
      '00137400 t v8::internal::Runtime_DebugGetPropertyDetails',
      '001c1a80 b _private_mem\n'
    ].join('\n'), ''];
  };

  var shell_prov = new MacCppEntriesProvider();
  var shell_syms = [];
  shell_prov.parseVmSymbols('shell', 0x00001b00, 0x00163156,
      function (name, start, end) {
        shell_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [['start', 0x00001b00, 0x00001b40],
       ['dyld_stub_binding_helper', 0x00001b40, 0x0011b710],
       ['v8::internal::RegExpMacroAssembler::CheckPosition', 0x0011b710, 0x00134250],
       ['v8::internal::Runtime_StringReplaceRegExpWithString', 0x00134250, 0x00137220],
       ['v8::internal::Runtime::GetElementOrCharAt', 0x00137220, 0x00137400],
       ['v8::internal::Runtime_DebugGetPropertyDetails', 0x00137400, 0x00163156]],
      shell_syms);

  // stdc++ library
  MacCppEntriesProvider.prototype.loadSymbols = function(libName) {
    this.symbols = [[
        '0000107a T __gnu_cxx::balloc::__mini_vector<std::pair<__gnu_cxx::bitmap_allocator<char>::_Alloc_block*, __gnu_cxx::bitmap_allocator<char>::_Alloc_block*> >::__mini_vector',
        '0002c410 T std::basic_streambuf<char, std::char_traits<char> >::pubseekoff',
        '0002c488 T std::basic_streambuf<char, std::char_traits<char> >::pubseekpos',
        '000466aa T ___cxa_pure_virtual\n'].join('\n'), ''];
  };
  var stdc_prov = new MacCppEntriesProvider();
  var stdc_syms = [];
  stdc_prov.parseVmSymbols('stdc++', 0x95728fb4, 0x95770005,
      function (name, start, end) {
        stdc_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  var stdc_ref_syms = [['__gnu_cxx::balloc::__mini_vector<std::pair<__gnu_cxx::bitmap_allocator<char>::_Alloc_block*, __gnu_cxx::bitmap_allocator<char>::_Alloc_block*> >::__mini_vector', 0x0000107a, 0x0002c410],
       ['std::basic_streambuf<char, std::char_traits<char> >::pubseekoff', 0x0002c410, 0x0002c488],
       ['std::basic_streambuf<char, std::char_traits<char> >::pubseekpos', 0x0002c488, 0x000466aa],
       ['___cxa_pure_virtual', 0x000466aa, 0x95770005 - 0x95728fb4]];
  for (var i = 0; i < stdc_ref_syms.length; ++i) {
    stdc_ref_syms[i][1] += 0x95728fb4;
    stdc_ref_syms[i][2] += 0x95728fb4;
  }
  assertEquals(stdc_ref_syms, stdc_syms);

  MacCppEntriesProvider.prototype.loadSymbols = oldLoadSymbols;
})();


(function testWindowsCppEntriesProvider() {
  var oldLoadSymbols = WindowsCppEntriesProvider.prototype.loadSymbols;

  WindowsCppEntriesProvider.prototype.loadSymbols = function(libName) {
    this.symbols = [
      ' Start         Length     Name                   Class',
      ' 0001:00000000 000ac902H .text                   CODE',
      ' 0001:000ac910 000005e2H .text$yc                CODE',
      '  Address         Publics by Value              Rva+Base       Lib:Object',
      ' 0000:00000000       __except_list              00000000     <absolute>',
      ' 0001:00000000       ?ReadFile@@YA?AV?$Handle@VString@v8@@@v8@@PBD@Z 00401000 f   shell.obj',
      ' 0001:000000a0       ?Print@@YA?AV?$Handle@VValue@v8@@@v8@@ABVArguments@2@@Z 004010a0 f   shell.obj',
      ' 0001:00001230       ??1UTF8Buffer@internal@v8@@QAE@XZ 00402230 f   v8_snapshot:scanner.obj',
      ' 0001:00001230       ??1Utf8Value@String@v8@@QAE@XZ 00402230 f   v8_snapshot:api.obj',
      ' 0001:000954ba       __fclose_nolock            004964ba f   LIBCMT:fclose.obj',
      ' 0002:00000000       __imp__SetThreadPriority@8 004af000     kernel32:KERNEL32.dll',
      ' 0003:00000418       ?in_use_list_@PreallocatedStorage@internal@v8@@0V123@A 00544418     v8_snapshot:allocation.obj',
      ' Static symbols',
      ' 0001:00000b70       ?DefaultFatalErrorHandler@v8@@YAXPBD0@Z 00401b70 f   v8_snapshot:api.obj',
      ' 0001:000010b0       ?EnsureInitialized@v8@@YAXPBD@Z 004020b0 f   v8_snapshot:api.obj',
      ' 0001:000ad17b       ??__Fnomem@?5???2@YAPAXI@Z@YAXXZ 004ae17b f   LIBCMT:new.obj'
    ].join('\r\n');
  };
  var shell_prov = new WindowsCppEntriesProvider();
  var shell_syms = [];
  shell_prov.parseVmSymbols('shell.exe', 0x00400000, 0x0057c000,
      function (name, start, end) {
        shell_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [['ReadFile', 0x00401000, 0x004010a0],
       ['Print', 0x004010a0, 0x00402230],
       ['v8::String::?1Utf8Value', 0x00402230, 0x004964ba],
       ['v8::DefaultFatalErrorHandler', 0x00401b70, 0x004020b0],
       ['v8::EnsureInitialized', 0x004020b0, 0x0057c000]],
      shell_syms);

  WindowsCppEntriesProvider.prototype.loadSymbols = oldLoadSymbols;
})();


// http://code.google.com/p/v8/issues/detail?id=427
(function testWindowsProcessExeAndDllMapFile() {
  function exeSymbols(exeName) {
    return [
      ' 0000:00000000       ___ImageBase               00400000     <linker-defined>',
      ' 0001:00000780       ?RunMain@@YAHHQAPAD@Z      00401780 f   shell.obj',
      ' 0001:00000ac0       _main                      00401ac0 f   shell.obj',
      ''
    ].join('\r\n');
  }

  function dllSymbols(dllName) {
    return [
      ' 0000:00000000       ___ImageBase               01c30000     <linker-defined>',
      ' 0001:00000780       _DllMain@12                01c31780 f   libcmt:dllmain.obj',
      ' 0001:00000ac0       ___DllMainCRTStartup       01c31ac0 f   libcmt:dllcrt0.obj',
      ''
    ].join('\r\n');
  }

  var oldRead = read;

  read = exeSymbols;
  var exe_exe_syms = [];
  (new WindowsCppEntriesProvider()).parseVmSymbols(
      'chrome.exe', 0x00400000, 0x00472000,
      function (name, start, end) {
        exe_exe_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [['RunMain', 0x00401780, 0x00401ac0],
       ['_main', 0x00401ac0, 0x00472000]],
      exe_exe_syms, '.exe with .exe symbols');

  read = dllSymbols;
  var exe_dll_syms = [];
  (new WindowsCppEntriesProvider()).parseVmSymbols(
      'chrome.exe', 0x00400000, 0x00472000,
      function (name, start, end) {
        exe_dll_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [],
      exe_dll_syms, '.exe with .dll symbols');

  read = dllSymbols;
  var dll_dll_syms = [];
  (new WindowsCppEntriesProvider()).parseVmSymbols(
      'chrome.dll', 0x01c30000, 0x02b80000,
      function (name, start, end) {
        dll_dll_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [['_DllMain@12', 0x01c31780, 0x01c31ac0],
       ['___DllMainCRTStartup', 0x01c31ac0, 0x02b80000]],
      dll_dll_syms, '.dll with .dll symbols');

  read = exeSymbols;
  var dll_exe_syms = [];
  (new WindowsCppEntriesProvider()).parseVmSymbols(
      'chrome.dll', 0x01c30000, 0x02b80000,
      function (name, start, end) {
        dll_exe_syms.push(Array.prototype.slice.apply(arguments, [0]));
      });
  assertEquals(
      [],
      dll_exe_syms, '.dll with .exe symbols');

  read = oldRead;
})();


function CppEntriesProviderMock() {
};


CppEntriesProviderMock.prototype.parseVmSymbols = function(
    name, startAddr, endAddr, symbolAdder) {
  var symbols = {
    'shell':
        [['v8::internal::JSObject::LookupOwnRealNamedProperty(v8::internal::String*, v8::internal::LookupResult*)', 0x080f8800, 0x080f8d90],
         ['v8::internal::HashTable<v8::internal::StringDictionaryShape, v8::internal::String*>::FindEntry(v8::internal::String*)', 0x080f8210, 0x080f8800],
         ['v8::internal::Runtime_Math_exp(v8::internal::Arguments)', 0x08123b20, 0x08123b80]],
    '/lib32/libm-2.7.so':
        [['exp', startAddr + 0x00009e80, startAddr + 0x00009e80 + 0xa3],
         ['fegetexcept', startAddr + 0x000061e0, startAddr + 0x000061e0 + 0x15]],
    'ffffe000-fffff000': []};
  assertTrue(name in symbols);
  var syms = symbols[name];
  for (var i = 0; i < syms.length; ++i) {
    symbolAdder.apply(null, syms[i]);
  }
};


function PrintMonitor(outputOrFileName) {
  var expectedOut = typeof outputOrFileName == 'string' ?
      this.loadExpectedOutput(outputOrFileName) : outputOrFileName;
  var outputPos = 0;
  var diffs = this.diffs = [];
  var realOut = this.realOut = [];
  var unexpectedOut = this.unexpectedOut = null;

  this.oldPrint = print;
  print = function(str) {
    var strSplit = str.split('\n');
    for (var i = 0; i < strSplit.length; ++i) {
      var s = strSplit[i];
      realOut.push(s);
      if (outputPos < expectedOut.length) {
        if (expectedOut[outputPos] != s) {
          diffs.push('line ' + outputPos + ': expected <' +
                     expectedOut[outputPos] + '> found <' + s + '>\n');
        }
        outputPos++;
      } else {
        unexpectedOut = true;
      }
    }
  };
};


PrintMonitor.prototype.loadExpectedOutput = function(fileName) {
  var output = readFile(fileName);
  return output.split('\n');
};


PrintMonitor.prototype.finish = function() {
  print = this.oldPrint;
  if (this.diffs.length > 0 || this.unexpectedOut != null) {
    print(this.realOut.join('\n'));
    assertEquals([], this.diffs);
    assertNull(this.unexpectedOut);
  }
};


function driveTickProcessorTest(
    separateIc, ignoreUnknown, stateFilter, logInput, refOutput) {
  // TEST_FILE_NAME must be provided by test runner.
  assertEquals('string', typeof TEST_FILE_NAME);
  var pathLen = TEST_FILE_NAME.lastIndexOf('/');
  if (pathLen == -1) {
    pathLen = TEST_FILE_NAME.lastIndexOf('\\');
  }
  assertTrue(pathLen != -1);
  var testsPath = TEST_FILE_NAME.substr(0, pathLen + 1);
  var tp = new TickProcessor(new CppEntriesProviderMock(),
                             separateIc,
                             TickProcessor.CALL_GRAPH_SIZE,
                             ignoreUnknown,
                             stateFilter,
                             undefined,
                             "0",
                             "auto,auto");
  var pm = new PrintMonitor(testsPath + refOutput);
  tp.processLogFileInTest(testsPath + logInput);
  tp.printStatistics();
  pm.finish();
};


(function testProcessing() {
  var testData = {
    'Default': [
      false, false, null,
      'tickprocessor-test.log', 'tickprocessor-test.default'],
    'SeparateIc': [
      true, false, null,
      'tickprocessor-test.log', 'tickprocessor-test.separate-ic'],
    'IgnoreUnknown': [
      false, true, null,
      'tickprocessor-test.log', 'tickprocessor-test.ignore-unknown'],
    'GcState': [
      false, false, TickProcessor.VmStates.GC,
      'tickprocessor-test.log', 'tickprocessor-test.gc-state'],
    'FunctionInfo': [
      false, false, null,
      'tickprocessor-test-func-info.log', 'tickprocessor-test.func-info']
  };
  for (var testName in testData) {
    print('=== testProcessing-' + testName + ' ===');
    driveTickProcessorTest.apply(null, testData[testName]);
  }
})();

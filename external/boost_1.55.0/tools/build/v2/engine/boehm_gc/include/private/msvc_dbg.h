/*
  Copyright (c) 2004-2005 Andrei Polushin

  Permission is hereby granted, free of charge,  to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction,  including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#ifndef _MSVC_DBG_H
#define _MSVC_DBG_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !MSVC_DBG_DLL
#define MSVC_DBG_EXPORT
#elif MSVC_DBG_BUILD
#define MSVC_DBG_EXPORT __declspec(dllexport)
#else
#define MSVC_DBG_EXPORT __declspec(dllimport)
#endif

#ifndef MAX_SYM_NAME
#define MAX_SYM_NAME 2000
#endif

typedef void*  HANDLE;
typedef struct _CONTEXT CONTEXT;

MSVC_DBG_EXPORT size_t GetStackFrames(size_t skip, void* frames[], size_t maxFrames);
MSVC_DBG_EXPORT size_t GetStackFramesFromContext(HANDLE hProcess, HANDLE hThread, CONTEXT* context, size_t skip, void* frames[], size_t maxFrames);

MSVC_DBG_EXPORT size_t GetModuleNameFromAddress(void* address, char* moduleName, size_t size);
MSVC_DBG_EXPORT size_t GetModuleNameFromStack(size_t skip, char* moduleName, size_t size);

MSVC_DBG_EXPORT size_t GetSymbolNameFromAddress(void* address, char* symbolName, size_t size, size_t* offsetBytes);
MSVC_DBG_EXPORT size_t GetSymbolNameFromStack(size_t skip, char* symbolName, size_t size, size_t* offsetBytes);

MSVC_DBG_EXPORT size_t GetFileLineFromAddress(void* address, char* fileName, size_t size, size_t* lineNumber, size_t* offsetBytes);
MSVC_DBG_EXPORT size_t GetFileLineFromStack(size_t skip, char* fileName, size_t size, size_t* lineNumber, size_t* offsetBytes);

MSVC_DBG_EXPORT size_t GetDescriptionFromAddress(void* address, const char* format, char* description, size_t size);
MSVC_DBG_EXPORT size_t GetDescriptionFromStack(void*const frames[], size_t count, const char* format, char* description[], size_t size);

/* Compatibility with <execinfo.h> */
MSVC_DBG_EXPORT int    backtrace(void* addresses[], int count);
MSVC_DBG_EXPORT char** backtrace_symbols(void*const addresses[], int count);

#ifdef __cplusplus
}
#endif

#endif/*_MSVC_DBG_H*/

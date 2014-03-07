/*
  Copyright (c) 2004 Andrei Polushin

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
#ifndef _M_AMD64

/* X86_64 is ccurrently missing some meachine-dependent code below. */

#include "private/msvc_dbg.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#pragma pack(push, 8)
#include <imagehlp.h>
#pragma pack(pop)

#pragma comment(lib, "dbghelp.lib")
#pragma optimize("gy", off)

#ifdef _WIN64
	typedef ULONG_PTR ULONG_ADDR;
#else
	typedef ULONG     ULONG_ADDR;
#endif

static HANDLE GetSymHandle()
{
	static HANDLE symHandle = NULL;
	if (!symHandle) {
		BOOL bRet = SymInitialize(symHandle = GetCurrentProcess(), NULL, FALSE);
		if (bRet) {
			DWORD dwOptions = SymGetOptions();
			dwOptions &= ~SYMOPT_UNDNAME;
			dwOptions |= SYMOPT_LOAD_LINES;
			SymSetOptions(dwOptions);
		}
	}
	return symHandle;
}

static void* CALLBACK FunctionTableAccess(HANDLE hProcess, ULONG_ADDR dwAddrBase)
{
	return SymFunctionTableAccess(hProcess, dwAddrBase);
}

static ULONG_ADDR CALLBACK GetModuleBase(HANDLE hProcess, ULONG_ADDR dwAddress)
{
	MEMORY_BASIC_INFORMATION memoryInfo;
	ULONG_ADDR dwAddrBase = SymGetModuleBase(hProcess, dwAddress);
	if (dwAddrBase) {
		return dwAddrBase;
	}
	if (VirtualQueryEx(hProcess, (void*)(ULONG_PTR)dwAddress, &memoryInfo, sizeof(memoryInfo))) {
		char filePath[_MAX_PATH];
		char curDir[_MAX_PATH];
		char exePath[_MAX_PATH];
		DWORD size = GetModuleFileNameA((HINSTANCE)memoryInfo.AllocationBase, filePath, sizeof(filePath));

		// Save and restore current directory around SymLoadModule, see KB article Q189780
		GetCurrentDirectoryA(sizeof(curDir), curDir);
		GetModuleFileNameA(NULL, exePath, sizeof(exePath));
		strcat_s(exePath, sizeof(exePath), "\\..");
		SetCurrentDirectoryA(exePath);
#ifdef _DEBUG
		GetCurrentDirectoryA(sizeof(exePath), exePath);
#endif
		SymLoadModule(hProcess, NULL, size ? filePath : NULL, NULL, (ULONG_ADDR)(ULONG_PTR)memoryInfo.AllocationBase, 0);
		SetCurrentDirectoryA(curDir);
	}
	return (ULONG_ADDR)(ULONG_PTR)memoryInfo.AllocationBase;
}

static ULONG_ADDR CheckAddress(void* address)
{
	ULONG_ADDR dwAddress = (ULONG_ADDR)(ULONG_PTR)address;
	GetModuleBase(GetSymHandle(), dwAddress);
	return dwAddress;
}

size_t GetStackFrames(size_t skip, void* frames[], size_t maxFrames)
{
	HANDLE hProcess = GetSymHandle();
	HANDLE hThread = GetCurrentThread();
	CONTEXT context;
	context.ContextFlags = CONTEXT_FULL;
	if (!GetThreadContext(hThread, &context)) {
		return 0;
	}
	// GetThreadContext might return invalid context for the current thread
#if defined(_M_IX86)
    __asm mov context.Ebp, ebp
#endif
	return GetStackFramesFromContext(hProcess, hThread, &context, skip + 1, frames, maxFrames);
}

size_t GetStackFramesFromContext(HANDLE hProcess, HANDLE hThread, CONTEXT* context, size_t skip, void* frames[], size_t maxFrames)
{
	size_t frameIndex;
	DWORD machineType;
	STACKFRAME stackFrame = { 0 };
	stackFrame.AddrPC.Mode      = AddrModeFlat;
#if defined(_M_IX86)
	machineType                 = IMAGE_FILE_MACHINE_I386;
	stackFrame.AddrPC.Offset    = context->Eip;
	stackFrame.AddrStack.Mode   = AddrModeFlat;
	stackFrame.AddrStack.Offset = context->Esp;
	stackFrame.AddrFrame.Mode   = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context->Ebp;
#elif defined(_M_MRX000)
	machineType                 = IMAGE_FILE_MACHINE_R4000;
	stackFrame.AddrPC.Offset    = context->Fir;
#elif defined(_M_ALPHA)
	machineType                 = IMAGE_FILE_MACHINE_ALPHA;
	stackFrame.AddrPC.Offset    = (unsigned long)context->Fir;
#elif defined(_M_PPC)
	machineType                 = IMAGE_FILE_MACHINE_POWERPC;
	stackFrame.AddrPC.Offset    = context->Iar;
#elif defined(_M_IA64)
	machineType                 = IMAGE_FILE_MACHINE_IA64;
	stackFrame.AddrPC.Offset    = context->StIIP;
#elif defined(_M_ALPHA64)
	machineType                 = IMAGE_FILE_MACHINE_ALPHA64;
	stackFrame.AddrPC.Offset    = context->Fir;
#else
#error Unknown CPU
#endif
	for (frameIndex = 0; frameIndex < maxFrames; ) {
		BOOL bRet = StackWalk(machineType, hProcess, hThread, &stackFrame, &context, NULL, FunctionTableAccess, GetModuleBase, NULL);
		if (!bRet) {
			break;
		}
		if (skip) {
			skip--;
		} else {
			frames[frameIndex++] = (void*)(ULONG_PTR)stackFrame.AddrPC.Offset;
		}
	}
	return frameIndex;
}

size_t GetModuleNameFromAddress(void* address, char* moduleName, size_t size)
{
	if (size) *moduleName = 0;
	{
		const char* sourceName;
		IMAGEHLP_MODULE moduleInfo = { sizeof (moduleInfo) };
		if (!SymGetModuleInfo(GetSymHandle(), CheckAddress(address), &moduleInfo)) {
			return 0;
		}
		sourceName = strrchr(moduleInfo.ImageName, '\\');
		if (sourceName) {
			sourceName++;
		} else {
			sourceName = moduleInfo.ImageName;
		}
		if (size) {
			strncpy(moduleName, sourceName, size)[size - 1] = 0;
		}
		return strlen(sourceName);
	}
}

size_t GetModuleNameFromStack(size_t skip, char* moduleName, size_t size)
{
	void* address = NULL;
	GetStackFrames(skip + 1, &address, 1);
	if (address) {
		return GetModuleNameFromAddress(address, moduleName, size);
	}
	return 0;
}

size_t GetSymbolNameFromAddress(void* address, char* symbolName, size_t size, size_t* offsetBytes)
{
	if (size) *symbolName = 0;
	if (offsetBytes) *offsetBytes = 0;
	__try {
		ULONG_ADDR dwOffset = 0;
		union {
			IMAGEHLP_SYMBOL sym;
			char symNameBuffer[sizeof(IMAGEHLP_SYMBOL) + MAX_SYM_NAME];
		} u;
		u.sym.SizeOfStruct  = sizeof(u.sym);
		u.sym.MaxNameLength = sizeof(u.symNameBuffer) - sizeof(u.sym);

		if (!SymGetSymFromAddr(GetSymHandle(), CheckAddress(address), &dwOffset, &u.sym)) {
			return 0;
		} else {
			const char* sourceName = u.sym.Name;
			char undName[1024];
			if (UnDecorateSymbolName(u.sym.Name, undName, sizeof(undName), UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS)) {
				sourceName = undName;
			} else if (SymUnDName(&u.sym, undName, sizeof(undName))) {
				sourceName = undName;
			}
			if (offsetBytes) {
				*offsetBytes = dwOffset;
			}
			if (size) {
				strncpy(symbolName, sourceName, size)[size - 1] = 0;
			}
			return strlen(sourceName);
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		SetLastError(GetExceptionCode());
	}
	return 0;
}

size_t GetSymbolNameFromStack(size_t skip, char* symbolName, size_t size, size_t* offsetBytes)
{
	void* address = NULL;
	GetStackFrames(skip + 1, &address, 1);
	if (address) {
		return GetSymbolNameFromAddress(address, symbolName, size, offsetBytes);
	}
	return 0;
}

size_t GetFileLineFromAddress(void* address, char* fileName, size_t size, size_t* lineNumber, size_t* offsetBytes)
{
	if (size) *fileName = 0;
	if (lineNumber) *lineNumber = 0;
	if (offsetBytes) *offsetBytes = 0;
	{
		char* sourceName;
		IMAGEHLP_LINE line = { sizeof (line) };
		ULONG_PTR dwOffset = 0;
		if (!SymGetLineFromAddr(GetSymHandle(), CheckAddress(address), &dwOffset, &line)) {
			return 0;
		}
		if (lineNumber) {
			*lineNumber = line.LineNumber;
		}
		if (offsetBytes) {
			*offsetBytes = dwOffset;
		}
		sourceName = line.FileName;
		// TODO: resolve relative filenames, found in 'source directories' registered with MSVC IDE.
		if (size) {
			strncpy(fileName, sourceName, size)[size - 1] = 0;
		}
		return strlen(sourceName);
	}
}

size_t GetFileLineFromStack(size_t skip, char* fileName, size_t size, size_t* lineNumber, size_t* offsetBytes)
{
	void* address = NULL;
	GetStackFrames(skip + 1, &address, 1);
	if (address) {
		return GetFileLineFromAddress(address, fileName, size, lineNumber, offsetBytes);
	}
	return 0;
}

size_t GetDescriptionFromAddress(void* address, const char* format, char* buffer, size_t size)
{
	char*const begin = buffer;
	char*const end = buffer + size;
	size_t line_number = 0;
	char   str[128];

	if (size) {
		*buffer = 0;
	}
	buffer += GetFileLineFromAddress(address, buffer, size, &line_number, NULL);
	size = end < buffer ? 0 : end - buffer;

	if (line_number) {
		wsprintf(str, "(%d) : ", line_number);
		if (size) {
			strncpy(buffer, str, size)[size - 1] = 0;
		}
		buffer += strlen(str);
		size = end < buffer ? 0 : end - buffer;
	}

	if (size) {
		strncpy(buffer, "at ", size)[size - 1] = 0;
	}
	buffer += strlen("at ");
	size = end < buffer ? 0 : end - buffer;

	buffer += GetSymbolNameFromAddress(address, buffer, size, NULL);
	size = end < buffer ? 0 : end - buffer;

	if (size) {
		strncpy(buffer, " in ", size)[size - 1] = 0;
	}
	buffer += strlen(" in ");
	size = end < buffer ? 0 : end - buffer;
	
	buffer += GetModuleNameFromAddress(address, buffer, size);
	size = end < buffer ? 0 : end - buffer;
	
	return buffer - begin;
}

size_t GetDescriptionFromStack(void*const frames[], size_t count, const char* format, char* description[], size_t size)
{
	char*const begin = (char*)description;
	char*const end = begin + size;
	char* buffer = begin + (count + 1) * sizeof(char*);
	size_t i;
	for (i = 0; i < count; ++i) {
		if (description) description[i] = buffer;
		size = end < buffer ? 0 : end - buffer;
		buffer += 1 + GetDescriptionFromAddress(frames[i], NULL, buffer, size);
	}
	if (description) description[count] = NULL;
	return buffer - begin;
}

/* Compatibility with <execinfo.h> */

int backtrace(void* addresses[], int count)
{
	return GetStackFrames(1, addresses, count);
}

char** backtrace_symbols(void*const* addresses, int count)
{
	size_t size = GetDescriptionFromStack(addresses, count, NULL, NULL, 0);
	char** symbols = (char**)malloc(size);
	GetDescriptionFromStack(addresses, count, NULL, symbols, size);
	return symbols;
}

#endif /* !_M_AMD64 */

// Copyright 2008 Google Inc. All Rights Reserved.
// Author: ppluzhnikov@google.com (Paul Pluzhnikov)
//
// Allow dynamic symbol lookup in the kernel VDSO page.
//
// VDSO stands for "Virtual Dynamic Shared Object" -- a page of
// executable code, which looks like a shared library, but doesn't
// necessarily exist anywhere on disk, and which gets mmap()ed into
// every process by kernels which support VDSO, such as 2.6.x for 32-bit
// executables, and 2.6.24 and above for 64-bit executables.
//
// More details could be found here:
// http://www.trilithium.com/johan/2005/08/linux-gate/
//
// VDSOSupport -- a class representing kernel VDSO (if present).
//
// Example usage:
//  VDSOSupport vdso;
//  VDSOSupport::SymbolInfo info;
//  typedef (*FN)(unsigned *, void *, void *);
//  FN fn = NULL;
//  if (vdso.LookupSymbol("__vdso_getcpu", "LINUX_2.6", STT_FUNC, &info)) {
//     fn = reinterpret_cast<FN>(info.address);
//  }

#ifndef BASE_VDSO_SUPPORT_H_
#define BASE_VDSO_SUPPORT_H_

#include <config.h>
#ifdef HAVE_FEATURES_H
#include <features.h>   // for __GLIBC__
#endif

// Maybe one day we can rewrite this file not to require the elf
// symbol extensions in glibc, but for right now we need them.
#if defined(__ELF__) && defined(__GLIBC__)

#define HAVE_VDSO_SUPPORT 1

#include <stdlib.h>     // for NULL
#include <link.h>  // for ElfW
#include "base/basictypes.h"

namespace base {

// NOTE: this class may be used from within tcmalloc, and can not
// use any memory allocation routines.
class VDSOSupport {
 public:
  // Sentinel: there could never be a VDSO at this address.
  static const void *const kInvalidBase;

  // Information about a single vdso symbol.
  // All pointers are into .dynsym, .dynstr, or .text of the VDSO.
  // Do not free() them or modify through them.
  struct SymbolInfo {
    const char      *name;      // E.g. "__vdso_getcpu"
    const char      *version;   // E.g. "LINUX_2.6", could be ""
                                // for unversioned symbol.
    const void      *address;   // Relocated symbol address.
    const ElfW(Sym) *symbol;    // Symbol in the dynamic symbol table.
  };

  // Supports iteration over all dynamic symbols.
  class SymbolIterator {
   public:
    friend class VDSOSupport;
    const SymbolInfo *operator->() const;
    const SymbolInfo &operator*() const;
    SymbolIterator& operator++();
    bool operator!=(const SymbolIterator &rhs) const;
    bool operator==(const SymbolIterator &rhs) const;
   private:
    SymbolIterator(const void *const image, int index);
    void Update(int incr);
    SymbolInfo info_;
    int index_;
    const void *const image_;
  };

  VDSOSupport();

  // Answers whether we have a vdso at all.
  bool IsPresent() const { return image_.IsPresent(); }

  // Allow to iterate over all VDSO symbols.
  SymbolIterator begin() const;
  SymbolIterator end() const;

  // Look up versioned dynamic symbol in the kernel VDSO.
  // Returns false if VDSO is not present, or doesn't contain given
  // symbol/version/type combination.
  // If info_out != NULL, additional details are filled in.
  bool LookupSymbol(const char *name, const char *version,
                    int symbol_type, SymbolInfo *info_out) const;

  // Find info about symbol (if any) which overlaps given address.
  // Returns true if symbol was found; false if VDSO isn't present
  // or doesn't have a symbol overlapping given address.
  // If info_out != NULL, additional details are filled in.
  bool LookupSymbolByAddress(const void *address, SymbolInfo *info_out) const;

  // Used only for testing. Replace real VDSO base with a mock.
  // Returns previous value of vdso_base_. After you are done testing,
  // you are expected to call SetBase() with previous value, in order to
  // reset state to the way it was.
  const void *SetBase(const void *s);

  // Computes vdso_base_ and returns it. Should be called as early as
  // possible; before any thread creation, chroot or setuid.
  static const void *Init();

 private:
  // An in-memory ELF image (may not exist on disk).
  class ElfMemImage {
   public:
    explicit ElfMemImage(const void *base);
    void                 Init(const void *base);
    bool                 IsPresent() const { return ehdr_ != NULL; }
    const ElfW(Phdr)*    GetPhdr(int index) const;
    const ElfW(Sym)*     GetDynsym(int index) const;
    const ElfW(Versym)*  GetVersym(int index) const;
    const ElfW(Verdef)*  GetVerdef(int index) const;
    const ElfW(Verdaux)* GetVerdefAux(const ElfW(Verdef) *verdef) const;
    const char*          GetDynstr(ElfW(Word) offset) const;
    const void*          GetSymAddr(const ElfW(Sym) *sym) const;
    const char*          GetVerstr(ElfW(Word) offset) const;
    int                  GetNumSymbols() const;
   private:
    const ElfW(Ehdr) *ehdr_;
    const ElfW(Sym) *dynsym_;
    const ElfW(Versym) *versym_;
    const ElfW(Verdef) *verdef_;
    const ElfW(Word) *hash_;
    const char *dynstr_;
    size_t strsize_;
    size_t verdefnum_;
    ElfW(Addr) link_base_;     // Link-time base (p_vaddr of first PT_LOAD).
  };

  // image_ represents VDSO ELF image in memory.
  // image_.ehdr_ == NULL implies there is no VDSO.
  ElfMemImage image_;

  // Cached value of auxv AT_SYSINFO_EHDR, computed once.
  // This is a tri-state:
  //   kInvalidBase   => value hasn't been determined yet.
  //              0   => there is no VDSO.
  //           else   => vma of VDSO Elf{32,64}_Ehdr.
  static const void *vdso_base_;

  // NOLINT on 'long' because these routines mimic kernel api.
  // The 'cache' parameter may be used by some versions of the kernel,
  // and should be NULL or point to a static buffer containing at
  // least two 'long's.
  static long InitAndGetCPU(unsigned *cpu, void *cache,     // NOLINT 'long'.
                            void *unused);
  static long GetCPUViaSyscall(unsigned *cpu, void *cache,  // NOLINT 'long'.
                               void *unused);
  typedef long (*GetCpuFn)(unsigned *cpu, void *cache,      // NOLINT 'long'.
                           void *unused);

  // This function pointer may point to InitAndGetCPU,
  // GetCPUViaSyscall, or __vdso_getcpu at different stages of initialization.
  static GetCpuFn getcpu_fn_;

  friend int GetCPU(void);  // Needs access to getcpu_fn_.

  DISALLOW_COPY_AND_ASSIGN(VDSOSupport);
};

// Same as sched_getcpu() on later glibc versions.
// Return current CPU, using (fast) __vdso_getcpu@LINUX_2.6 if present,
// otherwise use syscall(SYS_getcpu,...).
// May return -1 with errno == ENOSYS if the kernel doesn't
// support SYS_getcpu.
int GetCPU();
}  // namespace base

#endif  // __ELF__ and __GLIBC__

#endif  // BASE_VDSO_SUPPORT_H_

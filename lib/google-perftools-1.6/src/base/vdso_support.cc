// Copyright (c) 2008, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
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

// ---
// Author: Paul Pluzhnikov
//
// Allow dynamic symbol lookup in the kernel VDSO page.
//
// VDSOSupport -- a class representing kernel VDSO (if present).
//

#include "base/vdso_support.h"

#ifdef HAVE_VDSO_SUPPORT     // defined in vdso_support.h

#include <fcntl.h>

#include "base/atomicops.h"  // for MemoryBarrier
#include "base/linux_syscall_support.h"
#include "base/logging.h"
#include "base/dynamic_annotations.h"
#include "base/basictypes.h"  // for COMPILE_ASSERT

using base::subtle::MemoryBarrier;

#ifndef AT_SYSINFO_EHDR
#define AT_SYSINFO_EHDR 33
#endif

// From binutils/include/elf/common.h (this doesn't appear to be documented
// anywhere else).
//
//   /* This flag appears in a Versym structure.  It means that the symbol
//      is hidden, and is only visible with an explicit version number.
//      This is a GNU extension.  */
//   #define VERSYM_HIDDEN           0x8000
//
//   /* This is the mask for the rest of the Versym information.  */
//   #define VERSYM_VERSION          0x7fff

#define VERSYM_VERSION 0x7fff

namespace base {

namespace {
template <int N> class ElfClass {
 public:
  static const int kElfClass = -1;
  static int ElfBind(const ElfW(Sym) *) {
    CHECK(false); // << "Unexpected word size";
    return 0;
  }
  static int ElfType(const ElfW(Sym) *) {
    CHECK(false); // << "Unexpected word size";
    return 0;
  }
};

template <> class ElfClass<32> {
 public:
  static const int kElfClass = ELFCLASS32;
  static int ElfBind(const ElfW(Sym) *symbol) {
    return ELF32_ST_BIND(symbol->st_info);
  }
  static int ElfType(const ElfW(Sym) *symbol) {
    return ELF32_ST_TYPE(symbol->st_info);
  }
};

template <> class ElfClass<64> {
 public:
  static const int kElfClass = ELFCLASS64;
  static int ElfBind(const ElfW(Sym) *symbol) {
    return ELF64_ST_BIND(symbol->st_info);
  }
  static int ElfType(const ElfW(Sym) *symbol) {
    return ELF64_ST_TYPE(symbol->st_info);
  }
};

typedef ElfClass<__WORDSIZE> CurrentElfClass;

// Extract an element from one of the ELF tables, cast it to desired type.
// This is just a simple arithmetic and a glorified cast.
// Callers are responsible for bounds checking.
template <class T>
const T* GetTableElement(const ElfW(Ehdr) *ehdr,
                         ElfW(Off) table_offset,
                         ElfW(Word) element_size,
                         size_t index) {
  return reinterpret_cast<const T*>(reinterpret_cast<const char *>(ehdr)
                                    + table_offset
                                    + index * element_size);
}
}  // namespace

const void *const VDSOSupport::kInvalidBase =
    reinterpret_cast<const void *>(~0L);

const void *VDSOSupport::vdso_base_ = kInvalidBase;
VDSOSupport::GetCpuFn VDSOSupport::getcpu_fn_ = &InitAndGetCPU;

VDSOSupport::ElfMemImage::ElfMemImage(const void *base) {
  CHECK(base != kInvalidBase);
  Init(base);
}

int VDSOSupport::ElfMemImage::GetNumSymbols() const {
  if (!hash_) {
    return 0;
  }
  // See http://www.caldera.com/developers/gabi/latest/ch5.dynamic.html#hash
  return hash_[1];
}

const ElfW(Sym) *VDSOSupport::ElfMemImage::GetDynsym(int index) const {
  CHECK_LT(index, GetNumSymbols());
  return dynsym_ + index;
}

const ElfW(Versym) *VDSOSupport::ElfMemImage::GetVersym(int index) const {
  CHECK_LT(index, GetNumSymbols());
  return versym_ + index;
}

const ElfW(Phdr) *VDSOSupport::ElfMemImage::GetPhdr(int index) const {
  CHECK_LT(index, ehdr_->e_phnum);
  return GetTableElement<ElfW(Phdr)>(ehdr_,
                                     ehdr_->e_phoff,
                                     ehdr_->e_phentsize,
                                     index);
}

const char *VDSOSupport::ElfMemImage::GetDynstr(ElfW(Word) offset) const {
  CHECK_LT(offset, strsize_);
  return dynstr_ + offset;
}

const void *VDSOSupport::ElfMemImage::GetSymAddr(const ElfW(Sym) *sym) const {
  if (sym->st_shndx == SHN_UNDEF || sym->st_shndx >= SHN_LORESERVE) {
    // Symbol corresponds to "special" (e.g. SHN_ABS) section.
    return reinterpret_cast<const void *>(sym->st_value);
  }
  CHECK_LT(link_base_, sym->st_value);
  return GetTableElement<char>(ehdr_, 0, 1, sym->st_value) - link_base_;
}

const ElfW(Verdef) *VDSOSupport::ElfMemImage::GetVerdef(int index) const {
  CHECK_LE(index, verdefnum_);
  const ElfW(Verdef) *version_definition = verdef_;
  while (version_definition->vd_ndx < index && version_definition->vd_next) {
    const char *const version_definition_as_char =
        reinterpret_cast<const char *>(version_definition);
    version_definition =
        reinterpret_cast<const ElfW(Verdef) *>(version_definition_as_char +
                                               version_definition->vd_next);
  }
  return version_definition->vd_ndx == index ? version_definition : NULL;
}

const ElfW(Verdaux) *VDSOSupport::ElfMemImage::GetVerdefAux(
    const ElfW(Verdef) *verdef) const {
  return reinterpret_cast<const ElfW(Verdaux) *>(verdef+1);
}

const char *VDSOSupport::ElfMemImage::GetVerstr(ElfW(Word) offset) const {
  CHECK_LT(offset, strsize_);
  return dynstr_ + offset;
}

void VDSOSupport::ElfMemImage::Init(const void *base) {
  ehdr_      = NULL;
  dynsym_    = NULL;
  dynstr_    = NULL;
  versym_    = NULL;
  verdef_    = NULL;
  hash_      = NULL;
  strsize_   = 0;
  verdefnum_ = 0;
  link_base_ = ~0L;  // Sentinel: PT_LOAD .p_vaddr can't possibly be this.
  if (!base) {
    return;
  }
  const char *const base_as_char = reinterpret_cast<const char *>(base);
  if (base_as_char[EI_MAG0] != ELFMAG0 || base_as_char[EI_MAG1] != ELFMAG1 ||
      base_as_char[EI_MAG2] != ELFMAG2 || base_as_char[EI_MAG3] != ELFMAG3) {
    RAW_DCHECK(false, "no ELF magic"); // at %p", base);
    return;
  }
  int elf_class = base_as_char[EI_CLASS];
  if (elf_class != CurrentElfClass::kElfClass) {
    DCHECK_EQ(elf_class, CurrentElfClass::kElfClass);
    return;
  }
  switch (base_as_char[EI_DATA]) {
    case ELFDATA2LSB: {
      if (__LITTLE_ENDIAN != __BYTE_ORDER) {
        DCHECK_EQ(__LITTLE_ENDIAN, __BYTE_ORDER); // << ": wrong byte order";
        return;
      }
      break;
    }
    case ELFDATA2MSB: {
      if (__BIG_ENDIAN != __BYTE_ORDER) {
        DCHECK_EQ(__BIG_ENDIAN, __BYTE_ORDER); // << ": wrong byte order";
        return;
      }
      break;
    }
    default: {
      RAW_DCHECK(false, "unexpected data encoding"); // << base_as_char[EI_DATA];
      return;
    }
  }

  ehdr_ = reinterpret_cast<const ElfW(Ehdr) *>(base);
  const ElfW(Phdr) *dynamic_program_header = NULL;
  for (int i = 0; i < ehdr_->e_phnum; ++i) {
    const ElfW(Phdr) *const program_header = GetPhdr(i);
    switch (program_header->p_type) {
      case PT_LOAD:
        if (link_base_ == ~0L) {
          link_base_ = program_header->p_vaddr;
        }
        break;
      case PT_DYNAMIC:
        dynamic_program_header = program_header;
        break;
    }
  }
  if (link_base_ == ~0L || !dynamic_program_header) {
    RAW_DCHECK(~0L != link_base_, "no PT_LOADs in VDSO");
    RAW_DCHECK(dynamic_program_header, "no PT_DYNAMIC in VDSO");
    // Mark this image as not present. Can not recur infinitely.
    Init(0);
    return;
  }
  ptrdiff_t relocation =
      base_as_char - reinterpret_cast<const char *>(link_base_);
  ElfW(Dyn) *dynamic_entry =
      reinterpret_cast<ElfW(Dyn) *>(dynamic_program_header->p_vaddr +
                                    relocation);
  bool fake_vdso = false;  // Assume we are dealing with the real VDSO.
  for (ElfW(Dyn) *de = dynamic_entry; de->d_tag != DT_NULL; ++de) {
    ElfW(Sxword) tag = de->d_tag;
    if (tag == DT_PLTGOT || tag == DT_RELA || tag == DT_JMPREL ||
        tag == DT_NEEDED || tag == DT_RPATH || tag == DT_VERNEED ||
        tag == DT_INIT || tag == DT_FINI) {
      /* Real vdso can not reasonably have any of the above entries.  */
      fake_vdso = true;
      break;
    }
  }
  for (; dynamic_entry->d_tag != DT_NULL; ++dynamic_entry) {
    ElfW(Xword) value = dynamic_entry->d_un.d_val;
    if (fake_vdso) {
      // A complication: in the real VDSO, dynamic entries are not relocated
      // (it wasn't loaded by a dynamic loader). But when testing with a
      // "fake" dlopen()ed vdso library, the loader relocates some (but
      // not all!) of them before we get here.
      if (dynamic_entry->d_tag == DT_VERDEF) {
        // The only dynamic entry (of the ones we care about) libc-2.3.6
        // loader doesn't relocate.
        value += relocation;
      }
    } else {
      // Real VDSO. Everything needs to be relocated.
      value += relocation;
    }
    switch (dynamic_entry->d_tag) {
      case DT_HASH:
        hash_ = reinterpret_cast<ElfW(Word) *>(value);
        break;
      case DT_SYMTAB:
        dynsym_ = reinterpret_cast<ElfW(Sym) *>(value);
        break;
      case DT_STRTAB:
        dynstr_ = reinterpret_cast<const char *>(value);
        break;
      case DT_VERSYM:
        versym_ = reinterpret_cast<ElfW(Versym) *>(value);
        break;
      case DT_VERDEF:
        verdef_ = reinterpret_cast<ElfW(Verdef) *>(value);
        break;
      case DT_VERDEFNUM:
        verdefnum_ = dynamic_entry->d_un.d_val;
        break;
      case DT_STRSZ:
        strsize_ = dynamic_entry->d_un.d_val;
        break;
      default:
        // Unrecognized entries explicitly ignored.
        break;
    }
  }
  if (!hash_ || !dynsym_ || !dynstr_ || !versym_ ||
      !verdef_ || !verdefnum_ || !strsize_) {
    RAW_DCHECK(hash_, "invalid VDSO (no DT_HASH)");
    RAW_DCHECK(dynsym_, "invalid VDSO (no DT_SYMTAB)");
    RAW_DCHECK(dynstr_, "invalid VDSO (no DT_STRTAB)");
    RAW_DCHECK(versym_, "invalid VDSO (no DT_VERSYM)");
    RAW_DCHECK(verdef_, "invalid VDSO (no DT_VERDEF)");
    RAW_DCHECK(verdefnum_, "invalid VDSO (no DT_VERDEFNUM)");
    RAW_DCHECK(strsize_, "invalid VDSO (no DT_STRSZ)");
    // Mark this image as not present. Can not recur infinitely.
    Init(0);
    return;
  }
}

VDSOSupport::VDSOSupport()
    // If vdso_base_ is still set to kInvalidBase, we got here
    // before VDSOSupport::Init has been called. Call it now.
    : image_(vdso_base_ == kInvalidBase ? Init() : vdso_base_) {
}

// NOTE: we can't use GoogleOnceInit() below, because we can be
// called by tcmalloc, and none of the *once* stuff may be functional yet.
//
// In addition, we hope that the VDSOSupportHelper constructor
// causes this code to run before there are any threads, and before
// InitGoogle() has executed any chroot or setuid calls.
//
// Finally, even if there is a race here, it is harmless, because
// the operation should be idempotent.
const void *VDSOSupport::Init() {
  if (vdso_base_ == kInvalidBase) {
    // Valgrind zaps AT_SYSINFO_EHDR and friends from the auxv[]
    // on stack, and so glibc works as if VDSO was not present.
    // But going directly to kernel via /proc/self/auxv below bypasses
    // Valgrind zapping. So we check for Valgrind separately.
    if (RunningOnValgrind()) {
      vdso_base_ = NULL;
      getcpu_fn_ = &GetCPUViaSyscall;
      return NULL;
    }
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd == -1) {
      // Kernel too old to have a VDSO.
      vdso_base_ = NULL;
      getcpu_fn_ = &GetCPUViaSyscall;
      return NULL;
    }
    ElfW(auxv_t) aux;
    while (read(fd, &aux, sizeof(aux)) == sizeof(aux)) {
      if (aux.a_type == AT_SYSINFO_EHDR) {
        COMPILE_ASSERT(sizeof(vdso_base_) == sizeof(aux.a_un.a_val),
                       unexpected_sizeof_pointer_NE_sizeof_a_val);
        vdso_base_ = reinterpret_cast<void *>(aux.a_un.a_val);
        break;
      }
    }
    close(fd);
    if (vdso_base_ == kInvalidBase) {
      // Didn't find AT_SYSINFO_EHDR in auxv[].
      vdso_base_ = NULL;
    }
  }
  GetCpuFn fn = &GetCPUViaSyscall;  // default if VDSO not present.
  if (vdso_base_) {
    VDSOSupport vdso;
    SymbolInfo info;
    if (vdso.LookupSymbol("__vdso_getcpu", "LINUX_2.6", STT_FUNC, &info)) {
      // Casting from an int to a pointer is not legal C++.  To emphasize
      // this, we use a C-style cast rather than a C++-style cast.
      fn = (GetCpuFn)(info.address);
    }
  }
  // Subtle: this code runs outside of any locks; prevent compiler
  // from assigning to getcpu_fn_ more than once.
  MemoryBarrier();
  getcpu_fn_ = fn;
  return vdso_base_;
}

const void *VDSOSupport::SetBase(const void *base) {
  const void *old_base = vdso_base_;
  vdso_base_ = base;
  image_.Init(base);
  // Also reset getcpu_fn_, so GetCPU could be tested with simulated VDSO.
  getcpu_fn_ = &InitAndGetCPU;
  return old_base;
}

bool VDSOSupport::LookupSymbol(const char *name,
                               const char *version,
                               int type,
                               SymbolInfo *info) const {
  for (SymbolIterator it = begin(); it != end(); ++it) {
    if (strcmp(it->name, name) == 0 && strcmp(it->version, version) == 0 &&
        CurrentElfClass::ElfType(it->symbol) == type) {
      if (info) {
        *info = *it;
      }
      return true;
    }
  }
  return false;
}

bool VDSOSupport::LookupSymbolByAddress(const void *address,
                                        SymbolInfo *info_out) const {
  for (SymbolIterator it = begin(); it != end(); ++it) {
    const char *const symbol_start =
        reinterpret_cast<const char *>(it->address);
    const char *const symbol_end = symbol_start + it->symbol->st_size;
    if (symbol_start <= address && address < symbol_end) {
      if (info_out) {
        // Client wants to know details for that symbol (the usual case).
        if (CurrentElfClass::ElfBind(it->symbol) == STB_GLOBAL) {
          // Strong symbol; just return it.
          *info_out = *it;
          return true;
        } else {
          // Weak or local. Record it, but keep looking for a strong one.
          *info_out = *it;
        }
      } else {
        // Client only cares if there is an overlapping symbol.
        return true;
      }
    }
  }
  return false;
}

VDSOSupport::SymbolIterator::SymbolIterator(const void *const image, int index)
    : index_(index), image_(image) {
}

const VDSOSupport::SymbolInfo *VDSOSupport::SymbolIterator::operator->() const {
  return &info_;
}

const VDSOSupport::SymbolInfo& VDSOSupport::SymbolIterator::operator*() const {
  return info_;
}

bool VDSOSupport::SymbolIterator::operator==(const SymbolIterator &rhs) const {
  return this->image_ == rhs.image_ && this->index_ == rhs.index_;
}

bool VDSOSupport::SymbolIterator::operator!=(const SymbolIterator &rhs) const {
  return !(*this == rhs);
}

VDSOSupport::SymbolIterator &VDSOSupport::SymbolIterator::operator++() {
  this->Update(1);
  return *this;
}

VDSOSupport::SymbolIterator VDSOSupport::begin() const {
  SymbolIterator it(&image_, 0);
  it.Update(0);
  return it;
}

VDSOSupport::SymbolIterator VDSOSupport::end() const {
  return SymbolIterator(&image_, image_.GetNumSymbols());
}

void VDSOSupport::SymbolIterator::Update(int increment) {
  const ElfMemImage *image = reinterpret_cast<const ElfMemImage *>(image_);
  CHECK(image->IsPresent() || increment == 0);
  if (!image->IsPresent()) {
    return;
  }
  index_ += increment;
  if (index_ >= image->GetNumSymbols()) {
    index_ = image->GetNumSymbols();
    return;
  }
  const ElfW(Sym)    *symbol = image->GetDynsym(index_);
  const ElfW(Versym) *version_symbol = image->GetVersym(index_);
  CHECK(symbol && version_symbol);
  const char *const symbol_name = image->GetDynstr(symbol->st_name);
  const ElfW(Versym) version_index = version_symbol[0] & VERSYM_VERSION;
  const ElfW(Verdef) *version_definition = NULL;
  const char *version_name = "";
  if (symbol->st_shndx == SHN_UNDEF) {
    // Undefined symbols reference DT_VERNEED, not DT_VERDEF, and
    // version_index could well be greater than verdefnum_, so calling
    // GetVerdef(version_index) may trigger assertion.
  } else {
    version_definition = image->GetVerdef(version_index);
  }
  if (version_definition) {
    // I am expecting 1 or 2 auxiliary entries: 1 for the version itself,
    // optional 2nd if the version has a parent.
    CHECK_LE(1, version_definition->vd_cnt);
    CHECK_LE(version_definition->vd_cnt, 2);
    const ElfW(Verdaux) *version_aux = image->GetVerdefAux(version_definition);
    version_name = image->GetVerstr(version_aux->vda_name);
  }
  info_.name    = symbol_name;
  info_.version = version_name;
  info_.address = image->GetSymAddr(symbol);
  info_.symbol  = symbol;
}

// NOLINT on 'long' because this routine mimics kernel api.
long VDSOSupport::GetCPUViaSyscall(unsigned *cpu, void *, void *) { // NOLINT
#if defined(__NR_getcpu)
  return sys_getcpu(cpu, NULL, NULL);
#else
  // x86_64 never implemented sys_getcpu(), except as a VDSO call.
  errno = ENOSYS;
  return -1;
#endif
}

// Use fast __vdso_getcpu if available.
long VDSOSupport::InitAndGetCPU(unsigned *cpu, void *x, void *y) { // NOLINT
  Init();
  CHECK_NE(getcpu_fn_, &InitAndGetCPU); // << "Init() did not set getcpu_fn_";
  return (*getcpu_fn_)(cpu, x, y);
}

// This function must be very fast, and may be called from very
// low level (e.g. tcmalloc). Hence I avoid things like
// GoogleOnceInit() and ::operator new.
int GetCPU(void) {
  unsigned cpu;
  int ret_code = (*VDSOSupport::getcpu_fn_)(&cpu, NULL, NULL);
  return ret_code == 0 ? cpu : ret_code;
}

// We need to make sure VDSOSupport::Init() is called before
// the main() runs, since it might do something like setuid or
// chroot.  If VDSOSupport
// is used in any global constructor, this will happen, since
// VDSOSupport's constructor calls Init.  But if not, we need to
// ensure it here, with a global constructor of our own.  This
// is an allowed exception to the normal rule against non-trivial
// global constructors.
static class VDSOInitHelper {
 public:
  VDSOInitHelper() { VDSOSupport::Init(); }
} vdso_init_helper;
}

#endif  // HAVE_VDSO_SUPPORT

/* libunwind - a platform-independent unwind library
   Copyright (c) 2003-2005 Hewlett-Packard Development Company, L.P.
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

/* Locate an FDE via the ELF data-structures defined by LSB v1.3
   (http://www.linuxbase.org/spec/).  */

#ifndef UNW_REMOTE_ONLY
#include <link.h>
#endif /* !UNW_REMOTE_ONLY */
#include <stddef.h>
#include <stdio.h>
#include <limits.h>

#include "dwarf_i.h"
#include "dwarf-eh.h"
#include "libunwind_i.h"

struct table_entry
  {
    int32_t start_ip_offset;
    int32_t fde_offset;
  };

#ifndef UNW_REMOTE_ONLY

#ifdef __linux
#include "os-linux.h"
#endif

static int
linear_search (unw_addr_space_t as, unw_word_t ip,
	       unw_word_t eh_frame_start, unw_word_t eh_frame_end,
	       unw_word_t fde_count,
	       unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  unw_accessors_t *a = unw_get_accessors (unw_local_addr_space);
  unw_word_t i = 0, fde_addr, addr = eh_frame_start;
  int ret;

  while (i++ < fde_count && addr < eh_frame_end)
    {
      fde_addr = addr;
      if ((ret = dwarf_extract_proc_info_from_fde (as, a, &addr, pi, 0, 0, arg))
	  < 0)
	return ret;

      if (ip >= pi->start_ip && ip < pi->end_ip)
	{
	  if (!need_unwind_info)
	    return 1;
	  addr = fde_addr;
	  if ((ret = dwarf_extract_proc_info_from_fde (as, a, &addr, pi,
						       need_unwind_info, 0,
						       arg))
	      < 0)
	    return ret;
	  return 1;
	}
    }
  return -UNW_ENOINFO;
}
#endif /* !UNW_REMOTE_ONLY */

#ifdef CONFIG_DEBUG_FRAME
/* Load .debug_frame section from FILE.  Allocates and returns space
   in *BUF, and sets *BUFSIZE to its size.  IS_LOCAL is 1 if using the
   local process, in which case we can search the system debug file
   directory; 0 for other address spaces, in which case we do not; or
   -1 for recursive calls following .gnu_debuglink.  Returns 0 on
   success, 1 on error.  Succeeds even if the file contains no
   .debug_frame.  */
/* XXX: Could use mmap; but elf_map_image keeps tons mapped in.  */

static int
load_debug_frame (const char *file, char **buf, size_t *bufsize, int is_local)
{
  FILE *f;
  Elf_W (Ehdr) ehdr;
  Elf_W (Half) shstrndx;
  Elf_W (Shdr) *sec_hdrs = NULL;
  char *stringtab = NULL;
  unsigned int i;
  size_t linksize = 0;
  char *linkbuf = NULL;
  
  *buf = NULL;
  *bufsize = 0;
  
  f = fopen (file, "r");
  
  if (!f)
    return 1;
  
  if (fread (&ehdr, sizeof (Elf_W (Ehdr)), 1, f) != 1)
    goto file_error;
  
  shstrndx = ehdr.e_shstrndx;
  
  Debug (4, "opened file '%s'. Section header at offset %d\n",
         file, (int) ehdr.e_shoff);

  fseek (f, ehdr.e_shoff, SEEK_SET);
  sec_hdrs = calloc (ehdr.e_shnum, sizeof (Elf_W (Shdr)));
  if (fread (sec_hdrs, sizeof (Elf_W (Shdr)), ehdr.e_shnum, f) != ehdr.e_shnum)
    goto file_error;
  
  Debug (4, "loading string table of size %zd\n",
	   sec_hdrs[shstrndx].sh_size);
  stringtab = malloc (sec_hdrs[shstrndx].sh_size);
  fseek (f, sec_hdrs[shstrndx].sh_offset, SEEK_SET);
  if (fread (stringtab, 1, sec_hdrs[shstrndx].sh_size, f) != sec_hdrs[shstrndx].sh_size)
    goto file_error;
  
  for (i = 1; i < ehdr.e_shnum && *buf == NULL; i++)
    {
      char *secname = &stringtab[sec_hdrs[i].sh_name];

      if (strcmp (secname, ".debug_frame") == 0)
        {
	  *bufsize = sec_hdrs[i].sh_size;
	  *buf = malloc (*bufsize);

	  fseek (f, sec_hdrs[i].sh_offset, SEEK_SET);
	  if (fread (*buf, 1, *bufsize, f) != *bufsize)
	    goto file_error;

	  Debug (4, "read %zd bytes of .debug_frame from offset %zd\n",
		 *bufsize, sec_hdrs[i].sh_offset);
	}
      else if (strcmp (secname, ".gnu_debuglink") == 0)
	{
	  linksize = sec_hdrs[i].sh_size;
	  linkbuf = malloc (linksize);

	  fseek (f, sec_hdrs[i].sh_offset, SEEK_SET);
	  if (fread (linkbuf, 1, linksize, f) != linksize)
	    goto file_error;

	  Debug (4, "read %zd bytes of .gnu_debuglink from offset %zd\n",
		 linksize, sec_hdrs[i].sh_offset);
	}
    }

  free (stringtab);
  free (sec_hdrs);

  fclose (f);

  /* Ignore separate debug files which contain a .gnu_debuglink section. */
  if (linkbuf && is_local == -1)
    {
      free (linkbuf);
      return 1;
    }

  if (*buf == NULL && linkbuf != NULL && memchr (linkbuf, 0, linksize) != NULL)
    {
      char *newname, *basedir, *p;
      static const char *debugdir = "/usr/lib/debug";
      int ret;

      /* XXX: Don't bother with the checksum; just search for the file.  */
      basedir = malloc (strlen (file) + 1);
      newname = malloc (strlen (linkbuf) + strlen (debugdir)
			+ strlen (file) + 9);

      p = strrchr (file, '/');
      if (p != NULL)
	{
	  memcpy (basedir, file, p - file);
	  basedir[p - file] = '\0';
	}
      else
	basedir[0] = 0;

      strcpy (newname, basedir);
      strcat (newname, "/");
      strcat (newname, linkbuf);
      ret = load_debug_frame (newname, buf, bufsize, -1);

      if (ret == 1)
	{
	  strcpy (newname, basedir);
	  strcat (newname, "/.debug/");
	  strcat (newname, linkbuf);
	  ret = load_debug_frame (newname, buf, bufsize, -1);
	}

      if (ret == 1 && is_local == 1)
	{
	  strcpy (newname, debugdir);
	  strcat (newname, basedir);
	  strcat (newname, "/");
	  strcat (newname, linkbuf);
	  ret = load_debug_frame (newname, buf, bufsize, -1);
	}

      free (basedir);
      free (newname);
    }
  free (linkbuf);

  return 0;

/* An error reading image file. Release resources and return error code */
file_error:
  free(stringtab);
  free(sec_hdrs);
  free(linkbuf);
  fclose(f);

  return 1;
}

/* Locate the binary which originated the contents of address ADDR. Return
   the name of the binary in *name (space is allocated by the caller)
   Returns 0 if a binary is successfully found, or 1 if an error occurs.  */

static int
find_binary_for_address (unw_word_t ip, char *name, size_t name_size)
{
#if defined(__linux) && (!UNW_REMOTE_ONLY)
  struct map_iterator mi;
  int found = 0;
  int pid = getpid ();
  unsigned long segbase, mapoff, hi;

  maps_init (&mi, pid);
  while (maps_next (&mi, &segbase, &hi, &mapoff))
    if (ip >= segbase && ip < hi)
      {
	size_t len = strlen (mi.path);

	if (len + 1 <= name_size)
	  {
	    memcpy (name, mi.path, len + 1);
	    found = 1;
	  }
	break;
      }
  maps_close (&mi);
  return !found;
#endif

  return 1;
}

/* Locate and/or try to load a debug_frame section for address ADDR.  Return
   pointer to debug frame descriptor, or zero if not found.  */

static struct unw_debug_frame_list *
locate_debug_info (unw_addr_space_t as, unw_word_t addr, const char *dlname,
		   unw_word_t start, unw_word_t end)
{
  struct unw_debug_frame_list *w, *fdesc = 0;
  char path[PATH_MAX];
  char *name = path;
  int err;
  char *buf;
  size_t bufsize;

  /* First, see if we loaded this frame already.  */

  for (w = as->debug_frames; w; w = w->next)
    {
      Debug (4, "checking %p: %lx-%lx\n", w, (long)w->start, (long)w->end);
      if (addr >= w->start && addr < w->end)
	return w;
    }

  /* If the object name we receive is blank, there's still a chance of locating
     the file by parsing /proc/self/maps.  */

  if (strcmp (dlname, "") == 0)
    {
      err = find_binary_for_address (addr, name, sizeof(path));
      if (err)
        {
	  Debug (15, "tried to locate binary for 0x%" PRIx64 ", but no luck\n",
		 (uint64_t) addr);
          return 0;
	}
    }
  else
    name = (char*) dlname;

  err = load_debug_frame (name, &buf, &bufsize, as == unw_local_addr_space);
  
  if (!err)
    {
      fdesc = malloc (sizeof (struct unw_debug_frame_list));

      fdesc->start = start;
      fdesc->end = end;
      fdesc->debug_frame = buf;
      fdesc->debug_frame_size = bufsize;
      fdesc->index = NULL;
      fdesc->next = as->debug_frames;
      
      as->debug_frames = fdesc;
    }
  
  return fdesc;
}

struct debug_frame_tab
  {
    struct table_entry *tab;
    uint32_t length;
    uint32_t size;
  };

static void
debug_frame_tab_append (struct debug_frame_tab *tab,
			unw_word_t fde_offset, unw_word_t start_ip)
{
  unsigned int length = tab->length;

  if (length == tab->size)
    {
      tab->size *= 2;
      tab->tab = realloc (tab->tab, sizeof (struct table_entry) * tab->size);
    }
  
  tab->tab[length].fde_offset = fde_offset;
  tab->tab[length].start_ip_offset = start_ip;
  
  tab->length = length + 1;
}

static void
debug_frame_tab_shrink (struct debug_frame_tab *tab)
{
  if (tab->size > tab->length)
    {
      tab->tab = realloc (tab->tab, sizeof (struct table_entry) * tab->length);
      tab->size = tab->length;
    }
}

static int
debug_frame_tab_compare (const void *a, const void *b)
{
  const struct table_entry *fa = a, *fb = b;
  
  if (fa->start_ip_offset > fb->start_ip_offset)
    return 1;
  else if (fa->start_ip_offset < fb->start_ip_offset)
    return -1;
  else
    return 0;
}

PROTECTED int
dwarf_find_debug_frame (int found, unw_dyn_info_t *di_debug, unw_word_t ip,
			unw_word_t segbase, const char* obj_name,
			unw_word_t start, unw_word_t end)
{
  unw_dyn_info_t *di;
  struct unw_debug_frame_list *fdesc = 0;
  unw_accessors_t *a;
  unw_word_t addr;

  Debug (15, "Trying to find .debug_frame for %s\n", obj_name);
  di = di_debug;

  fdesc = locate_debug_info (unw_local_addr_space, ip, obj_name, start, end);

  if (!fdesc)
    {
      Debug (15, "couldn't load .debug_frame\n");
      return found;
    }
  else
    {
      char *buf;
      size_t bufsize;
      unw_word_t item_start, item_end = 0;
      uint32_t u32val = 0;
      uint64_t cie_id = 0;
      struct debug_frame_tab tab;

      Debug (15, "loaded .debug_frame\n");

      buf = fdesc->debug_frame;
      bufsize = fdesc->debug_frame_size;

      if (bufsize == 0)
       {
         Debug (15, "zero-length .debug_frame\n");
         return found;
       }

      /* Now create a binary-search table, if it does not already exist.  */
      if (!fdesc->index)
       {
         addr = (unw_word_t) (uintptr_t) buf;

         a = unw_get_accessors (unw_local_addr_space);

         /* Find all FDE entries in debug_frame, and make into a sorted
            index.  */

         tab.length = 0;
         tab.size = 16;
         tab.tab = calloc (tab.size, sizeof (struct table_entry));

         while (addr < (unw_word_t) (uintptr_t) (buf + bufsize))
           {
             uint64_t id_for_cie;
             item_start = addr;

             dwarf_readu32 (unw_local_addr_space, a, &addr, &u32val, NULL);

             if (u32val == 0)
               break;
             else if (u32val != 0xffffffff)
               {
                 uint32_t cie_id32 = 0;
                 item_end = addr + u32val;
                 dwarf_readu32 (unw_local_addr_space, a, &addr, &cie_id32,
                                NULL);
                 cie_id = cie_id32;
                 id_for_cie = 0xffffffff;
               }
             else
               {
                 uint64_t u64val = 0;
                 /* Extended length.  */
                 dwarf_readu64 (unw_local_addr_space, a, &addr, &u64val, NULL);
                 item_end = addr + u64val;

                 dwarf_readu64 (unw_local_addr_space, a, &addr, &cie_id, NULL);
                 id_for_cie = 0xffffffffffffffffull;
               }

             /*Debug (1, "CIE/FDE id = %.8x\n", (int) cie_id);*/

             if (cie_id == id_for_cie)
               ;
             /*Debug (1, "Found CIE at %.8x.\n", item_start);*/
             else
               {
                 unw_word_t fde_addr = item_start;
                 unw_proc_info_t this_pi;
                 int err;

                 /*Debug (1, "Found FDE at %.8x\n", item_start);*/

                 err = dwarf_extract_proc_info_from_fde (unw_local_addr_space,
                                                         a, &fde_addr,
                                                         &this_pi, 0,
                                                         (uintptr_t) buf,
                                                         NULL);
                 if (err == 0)
                   {
                     Debug (15, "start_ip = %lx, end_ip = %lx\n",
                            (long) this_pi.start_ip, (long) this_pi.end_ip);
                     debug_frame_tab_append (&tab,
                                             item_start - (unw_word_t) (uintptr_t) buf,
                                             this_pi.start_ip);
                   }
                 /*else
                   Debug (1, "FDE parse failed\n");*/
               }

             addr = item_end;
           }

         debug_frame_tab_shrink (&tab);
         qsort (tab.tab, tab.length, sizeof (struct table_entry),
                debug_frame_tab_compare);
         /* for (i = 0; i < tab.length; i++)
            {
            fprintf (stderr, "ip %x, fde offset %x\n",
            (int) tab.tab[i].start_ip_offset,
            (int) tab.tab[i].fde_offset);
            }*/
         fdesc->index = tab.tab;
         fdesc->index_size = tab.length;
       }

      di->format = UNW_INFO_FORMAT_TABLE;
      di->start_ip = fdesc->start;
      di->end_ip = fdesc->end;
      di->u.ti.name_ptr = (unw_word_t) (uintptr_t) obj_name;
      di->u.ti.table_data = (unw_word_t *) fdesc;
      di->u.ti.table_len = sizeof (*fdesc) / sizeof (unw_word_t);
      di->u.ti.segbase = segbase;

      found = 1;
      Debug (15, "found debug_frame table `%s': segbase=0x%lx, len=%lu, "
            "gp=0x%lx, table_data=0x%lx\n",
            (char *) (uintptr_t) di->u.ti.name_ptr,
            (long) di->u.ti.segbase, (long) di->u.ti.table_len,
            (long) di->gp, (long) di->u.ti.table_data);
    }
  return found;
}

#endif /* CONFIG_DEBUG_FRAME */

#ifndef UNW_REMOTE_ONLY

/* ptr is a pointer to a dwarf_callback_data structure and, on entry,
   member ip contains the instruction-pointer we're looking
   for.  */
HIDDEN int
dwarf_callback (struct dl_phdr_info *info, size_t size, void *ptr)
{
  struct dwarf_callback_data *cb_data = ptr;
  unw_dyn_info_t *di = &cb_data->di;
  const Elf_W(Phdr) *phdr, *p_eh_hdr, *p_dynamic, *p_text;
  unw_word_t addr, eh_frame_start, eh_frame_end, fde_count, ip;
  Elf_W(Addr) load_base, max_load_addr = 0;
  int ret, need_unwind_info = cb_data->need_unwind_info;
  unw_proc_info_t *pi = cb_data->pi;
  struct dwarf_eh_frame_hdr *hdr;
  unw_accessors_t *a;
  long n;
  int found = 0;
#ifdef CONFIG_DEBUG_FRAME
  unw_word_t start, end;
#endif /* CONFIG_DEBUG_FRAME*/

  ip = cb_data->ip;

  /* Make sure struct dl_phdr_info is at least as big as we need.  */
  if (size < offsetof (struct dl_phdr_info, dlpi_phnum)
	     + sizeof (info->dlpi_phnum))
    return -1;

  Debug (15, "checking %s, base=0x%lx)\n",
	 info->dlpi_name, (long) info->dlpi_addr);

  phdr = info->dlpi_phdr;
  load_base = info->dlpi_addr;
  p_text = NULL;
  p_eh_hdr = NULL;
  p_dynamic = NULL;

  /* See if PC falls into one of the loaded segments.  Find the
     eh-header segment at the same time.  */
  for (n = info->dlpi_phnum; --n >= 0; phdr++)
    {
      if (phdr->p_type == PT_LOAD)
	{
	  Elf_W(Addr) vaddr = phdr->p_vaddr + load_base;

	  if (ip >= vaddr && ip < vaddr + phdr->p_memsz)
	    p_text = phdr;

	  if (vaddr + phdr->p_filesz > max_load_addr)
	    max_load_addr = vaddr + phdr->p_filesz;
	}
      else if (phdr->p_type == PT_GNU_EH_FRAME)
	p_eh_hdr = phdr;
      else if (phdr->p_type == PT_DYNAMIC)
	p_dynamic = phdr;
    }
  
  if (!p_text)
    return 0;

  if (p_eh_hdr)
    {
      if (p_dynamic)
	{
	  /* For dynamicly linked executables and shared libraries,
	     DT_PLTGOT is the value that data-relative addresses are
	     relative to for that object.  We call this the "gp".  */
	  Elf_W(Dyn) *dyn = (Elf_W(Dyn) *)(p_dynamic->p_vaddr + load_base);
	  for (; dyn->d_tag != DT_NULL; ++dyn)
	    if (dyn->d_tag == DT_PLTGOT)
	      {
		/* Assume that _DYNAMIC is writable and GLIBC has
		   relocated it (true for x86 at least).  */
		di->gp = dyn->d_un.d_ptr;
		break;
	      }
	}
      else
	/* Otherwise this is a static executable with no _DYNAMIC.  Assume
	   that data-relative addresses are relative to 0, i.e.,
	   absolute.  */
	di->gp = 0;
      pi->gp = di->gp;

      hdr = (struct dwarf_eh_frame_hdr *) (p_eh_hdr->p_vaddr + load_base);
      if (hdr->version != DW_EH_VERSION)
	{
	  Debug (1, "table `%s' has unexpected version %d\n",
		 info->dlpi_name, hdr->version);
	  return 0;
	}

      a = unw_get_accessors (unw_local_addr_space);
      addr = (unw_word_t) (uintptr_t) (hdr + 1);

      /* (Optionally) read eh_frame_ptr: */
      if ((ret = dwarf_read_encoded_pointer (unw_local_addr_space, a,
					     &addr, hdr->eh_frame_ptr_enc, pi,
					     &eh_frame_start, NULL)) < 0)
	return ret;

      /* (Optionally) read fde_count: */
      if ((ret = dwarf_read_encoded_pointer (unw_local_addr_space, a,
					     &addr, hdr->fde_count_enc, pi,
					     &fde_count, NULL)) < 0)
	return ret;

      if (hdr->table_enc != (DW_EH_PE_datarel | DW_EH_PE_sdata4))
	{
	  /* If there is no search table or it has an unsupported
	     encoding, fall back on linear search.  */
	  if (hdr->table_enc == DW_EH_PE_omit)
	    Debug (4, "table `%s' lacks search table; doing linear search\n",
		   info->dlpi_name);
	  else
	    Debug (4, "table `%s' has encoding 0x%x; doing linear search\n",
		   info->dlpi_name, hdr->table_enc);

	  eh_frame_end = max_load_addr;	/* XXX can we do better? */

	  if (hdr->fde_count_enc == DW_EH_PE_omit)
	    fde_count = ~0UL;
	  if (hdr->eh_frame_ptr_enc == DW_EH_PE_omit)
	    abort ();

	  /* XXX we know how to build a local binary search table for
	     .debug_frame, so we could do that here too.  */
	  cb_data->single_fde = 1;
	  found = linear_search (unw_local_addr_space, ip,
				 eh_frame_start, eh_frame_end, fde_count,
				 pi, need_unwind_info, NULL);
	  if (found != 1)
	    found = 0;
	}
      else
	{
	  di->format = UNW_INFO_FORMAT_REMOTE_TABLE;
	  di->start_ip = p_text->p_vaddr + load_base;
	  di->end_ip = p_text->p_vaddr + load_base + p_text->p_memsz;
	  di->u.rti.name_ptr = (unw_word_t) (uintptr_t) info->dlpi_name;
	  di->u.rti.table_data = addr;
	  assert (sizeof (struct table_entry) % sizeof (unw_word_t) == 0);
	  di->u.rti.table_len = (fde_count * sizeof (struct table_entry)
				 / sizeof (unw_word_t));
	  /* For the binary-search table in the eh_frame_hdr, data-relative
	     means relative to the start of that section... */
	  di->u.rti.segbase = (unw_word_t) (uintptr_t) hdr;

	  found = 1;
	  Debug (15, "found table `%s': segbase=0x%lx, len=%lu, gp=0x%lx, "
		 "table_data=0x%lx\n", (char *) (uintptr_t) di->u.rti.name_ptr,
		 (long) di->u.rti.segbase, (long) di->u.rti.table_len,
		 (long) di->gp, (long) di->u.rti.table_data);
	}
    }

#ifdef CONFIG_DEBUG_FRAME
  /* Find the start/end of the described region by parsing the phdr_info
     structure.  */
  start = (unw_word_t) -1;
  end = 0;

  for (n = 0; n < info->dlpi_phnum; n++)
    {
      if (info->dlpi_phdr[n].p_type == PT_LOAD)
        {
	  unw_word_t seg_start = info->dlpi_addr + info->dlpi_phdr[n].p_vaddr;
          unw_word_t seg_end = seg_start + info->dlpi_phdr[n].p_memsz;

	  if (seg_start < start)
	    start = seg_start;

	  if (seg_end > end)
	    end = seg_end;
	}
    }

  found = dwarf_find_debug_frame (found, &cb_data->di_debug, ip,
				  info->dlpi_addr, info->dlpi_name, start,
				  end);
#endif  /* CONFIG_DEBUG_FRAME */

  return found;
}

HIDDEN int
dwarf_find_proc_info (unw_addr_space_t as, unw_word_t ip,
		      unw_proc_info_t *pi, int need_unwind_info, void *arg)
{
  struct dwarf_callback_data cb_data;
  intrmask_t saved_mask;
  int ret;

  Debug (14, "looking for IP=0x%lx\n", (long) ip);

  memset (&cb_data, 0, sizeof (cb_data));
  cb_data.ip = ip;
  cb_data.pi = pi;
  cb_data.need_unwind_info = need_unwind_info;
  cb_data.di.format = -1;
  cb_data.di_debug.format = -1;

  SIGPROCMASK (SIG_SETMASK, &unwi_full_mask, &saved_mask);
  ret = dl_iterate_phdr (dwarf_callback, &cb_data);
  SIGPROCMASK (SIG_SETMASK, &saved_mask, NULL);

  if (ret <= 0)
    {
      Debug (14, "IP=0x%lx not found\n", (long) ip);
      return -UNW_ENOINFO;
    }

  if (cb_data.single_fde)
    /* already got the result in *pi */
    return 0;

  /* search the table: */
  if (cb_data.di.format != -1)
    ret = dwarf_search_unwind_table (as, ip, &cb_data.di,
				      pi, need_unwind_info, arg);
  else
    ret = -UNW_ENOINFO;

  if (ret == -UNW_ENOINFO && cb_data.di_debug.format != -1)
    ret = dwarf_search_unwind_table (as, ip, &cb_data.di_debug, pi,
				     need_unwind_info, arg);
  return ret;
}

static inline const struct table_entry *
lookup (const struct table_entry *table, size_t table_size, int32_t rel_ip)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  const struct table_entry *e = NULL;
  unsigned long lo, hi, mid;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e = table + mid;
      Debug (15, "e->start_ip_offset = %lx\n", (long) e->start_ip_offset);
      if (rel_ip < e->start_ip_offset)
	hi = mid;
      else
	lo = mid + 1;
    }
  if (hi <= 0)
	return NULL;
  e = table + hi - 1;
  return e;
}

#endif /* !UNW_REMOTE_ONLY */

#ifndef UNW_LOCAL_ONLY

/* Lookup an unwind-table entry in remote memory.  Returns 1 if an
   entry is found, 0 if no entry is found, negative if an error
   occurred reading remote memory.  */
static int
remote_lookup (unw_addr_space_t as,
	       unw_word_t table, size_t table_size, int32_t rel_ip,
	       struct table_entry *e, void *arg)
{
  unsigned long table_len = table_size / sizeof (struct table_entry);
  unw_accessors_t *a = unw_get_accessors (as);
  unsigned long lo, hi, mid;
  unw_word_t e_addr = 0;
  int32_t start;
  int ret;

  /* do a binary search for right entry: */
  for (lo = 0, hi = table_len; lo < hi;)
    {
      mid = (lo + hi) / 2;
      e_addr = table + mid * sizeof (struct table_entry);
      if ((ret = dwarf_reads32 (as, a, &e_addr, &start, arg)) < 0)
	return ret;

      if (rel_ip < start)
	hi = mid;
      else
	lo = mid + 1;
    }
  if (hi <= 0)
    return 0;
  e_addr = table + (hi - 1) * sizeof (struct table_entry);
  if ((ret = dwarf_reads32 (as, a, &e_addr, &e->start_ip_offset, arg)) < 0
   || (ret = dwarf_reads32 (as, a, &e_addr, &e->fde_offset, arg)) < 0)
    return ret;
  return 1;
}

#endif /* !UNW_LOCAL_ONLY */

PROTECTED int
dwarf_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
			   unw_dyn_info_t *di, unw_proc_info_t *pi,
			   int need_unwind_info, void *arg)
{
  const struct table_entry *e = NULL, *table;
  unw_word_t segbase = 0, fde_addr;
  unw_accessors_t *a;
#ifndef UNW_LOCAL_ONLY
  struct table_entry ent;
#endif
  int ret;
  unw_word_t debug_frame_base;
  size_t table_len;

#ifdef UNW_REMOTE_ONLY
  assert (di->format == UNW_INFO_FORMAT_REMOTE_TABLE);
#else
  assert (di->format == UNW_INFO_FORMAT_REMOTE_TABLE
	  || di->format == UNW_INFO_FORMAT_TABLE);
#endif
  assert (ip >= di->start_ip && ip < di->end_ip);

  if (di->format == UNW_INFO_FORMAT_REMOTE_TABLE)
    {
      table = (const struct table_entry *) (uintptr_t) di->u.rti.table_data;
      table_len = di->u.rti.table_len * sizeof (unw_word_t);
      debug_frame_base = 0;
    }
  else
    {
#ifndef UNW_REMOTE_ONLY
      struct unw_debug_frame_list *fdesc = (void *) di->u.ti.table_data;

      /* UNW_INFO_FORMAT_TABLE (i.e. .debug_frame) is read from local address
         space.  Both the index and the unwind tables live in local memory, but
         the address space to check for properties like the address size and
         endianness is the target one.  */
      as = unw_local_addr_space;
      table = fdesc->index;
      table_len = fdesc->index_size * sizeof (struct table_entry);
      debug_frame_base = (uintptr_t) fdesc->debug_frame;
#endif
    }

  a = unw_get_accessors (as);

#ifndef UNW_REMOTE_ONLY
  if (as == unw_local_addr_space)
    {
      segbase = di->u.rti.segbase;
      e = lookup (table, table_len, ip - segbase);
    }
  else
#endif
    {
#ifndef UNW_LOCAL_ONLY
      segbase = di->u.rti.segbase;
      if ((ret = remote_lookup (as, (uintptr_t) table, table_len,
				ip - segbase, &ent, arg)) < 0)
	return ret;
      if (ret)
	e = &ent;
      else
	e = NULL;	/* no info found */
#endif
    }
  if (!e)
    {
      Debug (1, "IP %lx inside range %lx-%lx, but no explicit unwind info found\n",
	     (long) ip, (long) di->start_ip, (long) di->end_ip);
      /* IP is inside this table's range, but there is no explicit
	 unwind info.  */
      return -UNW_ENOINFO;
    }
  Debug (15, "ip=0x%lx, start_ip=0x%lx\n",
	 (long) ip, (long) (e->start_ip_offset));
  if (debug_frame_base)
    fde_addr = e->fde_offset + debug_frame_base;
  else
    fde_addr = e->fde_offset + segbase;
  Debug (1, "e->fde_offset = %lx, segbase = %lx, debug_frame_base = %lx, "
	    "fde_addr = %lx\n", (long) e->fde_offset, (long) segbase,
	    (long) debug_frame_base, (long) fde_addr);
  if ((ret = dwarf_extract_proc_info_from_fde (as, a, &fde_addr, pi,
					       need_unwind_info,
					       debug_frame_base, arg)) < 0)
    return ret;

  /* .debug_frame uses an absolute encoding that does not know about any
     shared library relocation.  */
  if (di->format == UNW_INFO_FORMAT_TABLE)
    {
      pi->start_ip += segbase;
      pi->end_ip += segbase;
      pi->flags = UNW_PI_FLAG_DEBUG_FRAME;
    }

  if (ip < pi->start_ip || ip >= pi->end_ip)
    return -UNW_ENOINFO;

  return 0;
}

HIDDEN void
dwarf_put_unwind_info (unw_addr_space_t as, unw_proc_info_t *pi, void *arg)
{
  return;	/* always a nop */
}

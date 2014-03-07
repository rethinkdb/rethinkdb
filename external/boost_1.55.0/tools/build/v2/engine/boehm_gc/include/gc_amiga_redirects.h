#ifndef GC_AMIGA_REDIRECTS_H

# define GC_AMIGA_REDIRECTS_H

# if ( defined(_AMIGA) && !defined(GC_AMIGA_MAKINGLIB) )
    extern void *GC_amiga_realloc(void *old_object,size_t new_size_in_bytes);
#   define GC_realloc(a,b) GC_amiga_realloc(a,b)
    extern void GC_amiga_set_toany(void (*func)(void));
    extern int GC_amiga_free_space_divisor_inc;
    extern void *(*GC_amiga_allocwrapper_do) \
	(size_t size,void *(*AllocFunction)(size_t size2));
#   define GC_malloc(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc)
#   define GC_malloc_atomic(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc_atomic)
#   define GC_malloc_uncollectable(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc_uncollectable)
#   define GC_malloc_stubborn(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc_stubborn)
#   define GC_malloc_atomic_uncollectable(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc_atomic_uncollectable)
#   define GC_malloc_ignore_off_page(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc_ignore_off_page)
#   define GC_malloc_atomic_ignore_off_page(a) \
	(*GC_amiga_allocwrapper_do)(a,GC_malloc_atomic_ignore_off_page)
# endif /* _AMIGA && !GC_AMIGA_MAKINGLIB */

#endif /* GC_AMIGA_REDIRECTS_H */



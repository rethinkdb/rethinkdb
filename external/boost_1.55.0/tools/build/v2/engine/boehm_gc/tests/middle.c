/*
 * Test at the boundary between small and large objects.
 * Inspired by a test case from Zoltan Varga.
 */
#include <gc.h>
#include <stdio.h>

int main ()
{
        int i;

        GC_all_interior_pointers = 0;
	GC_INIT();

        for (i = 0; i < 20000; ++i) {
                GC_malloc_atomic (4096);
                GC_malloc (4096);
	}
        for (i = 0; i < 20000; ++i) {
                GC_malloc_atomic (2048);
                GC_malloc (2048);
	}
	printf("Final heap size is %ld\n", GC_get_heap_size());
	return 0;
}




/******************************************************************

  AmigaOS-spesific routines for GC.
  This file is normally included from os_dep.c

******************************************************************/


#if !defined(GC_AMIGA_DEF) && !defined(GC_AMIGA_SB) && !defined(GC_AMIGA_DS) && !defined(GC_AMIGA_AM)
# include "gc_priv.h"
# include <stdio.h>
# include <signal.h>
# define GC_AMIGA_DEF
# define GC_AMIGA_SB
# define GC_AMIGA_DS
# define GC_AMIGA_AM
#endif


#ifdef GC_AMIGA_DEF

# ifndef __GNUC__
#   include <exec/exec.h>
# endif
# include <proto/exec.h>
# include <proto/dos.h>
# include <dos/dosextens.h>
# include <workbench/startup.h>

#endif




#ifdef GC_AMIGA_SB

/******************************************************************
   Find the base of the stack.
******************************************************************/

ptr_t GC_get_main_stack_base()
{
    struct Process *proc = (struct Process*)SysBase->ThisTask;
 
    /* Reference: Amiga Guru Book Pages: 42,567,574 */
    if (proc->pr_Task.tc_Node.ln_Type==NT_PROCESS
        && proc->pr_CLI != NULL) {
	/* first ULONG is StackSize */
	/*longPtr = proc->pr_ReturnAddr;
	size = longPtr[0];*/

	return (char *)proc->pr_ReturnAddr + sizeof(ULONG);
    } else {
	return (char *)proc->pr_Task.tc_SPUpper;
    }
}

#if 0 /* old version */
ptr_t GC_get_stack_base()
{
    extern struct WBStartup *_WBenchMsg;
    extern long __base;
    extern long __stack;
    struct Task *task;
    struct Process *proc;
    struct CommandLineInterface *cli;
    long size;

    if ((task = FindTask(0)) == 0) {
	GC_err_puts("Cannot find own task structure\n");
	ABORT("task missing");
    }
    proc = (struct Process *)task;
    cli = BADDR(proc->pr_CLI);

    if (_WBenchMsg != 0 || cli == 0) {
	size = (char *)task->tc_SPUpper - (char *)task->tc_SPLower;
    } else {
	size = cli->cli_DefaultStack * 4;
    }
    return (ptr_t)(__base + GC_max(size, __stack));
}
#endif


#endif


#ifdef GC_AMIGA_DS
/******************************************************************
   Register data segments.
******************************************************************/

   void GC_register_data_segments()
   {
     struct Process	*proc;
     struct CommandLineInterface *cli;
     BPTR myseglist;
     ULONG *data;
 
     int	num;


#    ifdef __GNUC__
        ULONG dataSegSize;
        GC_bool found_segment = FALSE;
	extern char __data_size[];

	dataSegSize=__data_size+8;
	/* Can`t find the Location of __data_size, because
           it`s possible that is it, inside the segment. */

#     endif

	proc= (struct Process*)SysBase->ThisTask;

	/* Reference: Amiga Guru Book Pages: 538ff,565,573
		     and XOper.asm */
	if (proc->pr_Task.tc_Node.ln_Type==NT_PROCESS) {
	  if (proc->pr_CLI == NULL) {
	    myseglist = proc->pr_SegList;
	  } else {
	    /* ProcLoaded	'Loaded as a command: '*/
	    cli = BADDR(proc->pr_CLI);
	    myseglist = cli->cli_Module;
	  }
	} else {
	  ABORT("Not a Process.");
 	}

	if (myseglist == NULL) {
	    ABORT("Arrrgh.. can't find segments, aborting");
 	}

	/* xoper hunks Shell Process */

	num=0;
        for (data = (ULONG *)BADDR(myseglist); data != NULL;
             data = (ULONG *)BADDR(data[0])) {
	  if (((ULONG) GC_register_data_segments < (ULONG) &data[1]) ||
	      ((ULONG) GC_register_data_segments > (ULONG) &data[1] + data[-1])) {
#             ifdef __GNUC__
		if (dataSegSize == data[-1]) {
		  found_segment = TRUE;
		}
# 	      endif
	      GC_add_roots_inner((char *)&data[1],
				 ((char *)&data[1]) + data[-1], FALSE);
          }
          ++num;
        } /* for */
# 	ifdef __GNUC__
	   if (!found_segment) {
	     ABORT("Can`t find correct Segments.\nSolution: Use an newer version of ixemul.library");
	   }
# 	endif
  }

#if 0 /* old version */
  void GC_register_data_segments()
  {
    extern struct WBStartup *_WBenchMsg;
    struct Process	*proc;
    struct CommandLineInterface *cli;
    BPTR myseglist;
    ULONG *data;

    if ( _WBenchMsg != 0 ) {
	if ((myseglist = _WBenchMsg->sm_Segment) == 0) {
	    GC_err_puts("No seglist from workbench\n");
	    return;
	}
    } else {
	if ((proc = (struct Process *)FindTask(0)) == 0) {
	    GC_err_puts("Cannot find process structure\n");
	    return;
	}
	if ((cli = BADDR(proc->pr_CLI)) == 0) {
	    GC_err_puts("No CLI\n");
	    return;
	}
	if ((myseglist = cli->cli_Module) == 0) {
	    GC_err_puts("No seglist from CLI\n");
	    return;
	}
    }

    for (data = (ULONG *)BADDR(myseglist); data != 0;
         data = (ULONG *)BADDR(data[0])) {
#        ifdef AMIGA_SKIP_SEG
           if (((ULONG) GC_register_data_segments < (ULONG) &data[1]) ||
           ((ULONG) GC_register_data_segments > (ULONG) &data[1] + data[-1])) {
#	 else
      	   {
#	 endif /* AMIGA_SKIP_SEG */
          GC_add_roots_inner((char *)&data[1],
          		     ((char *)&data[1]) + data[-1], FALSE);
         }
    }
  }
#endif /* old version */


#endif



#ifdef GC_AMIGA_AM

#ifndef GC_AMIGA_FASTALLOC

void *GC_amiga_allocwrapper(size_t size,void *(*AllocFunction)(size_t size2)){
	return (*AllocFunction)(size);
}

void *(*GC_amiga_allocwrapper_do)(size_t size,void *(*AllocFunction)(size_t size2))
	=GC_amiga_allocwrapper;

#else




void *GC_amiga_allocwrapper_firsttime(size_t size,void *(*AllocFunction)(size_t size2));

void *(*GC_amiga_allocwrapper_do)(size_t size,void *(*AllocFunction)(size_t size2))
	=GC_amiga_allocwrapper_firsttime;


/******************************************************************
   Amiga-spesific routines to obtain memory, and force GC to give
   back fast-mem whenever possible.
	These hacks makes gc-programs go many times faster when
   the amiga is low on memory, and are therefore strictly necesarry.

   -Kjetil S. Matheussen, 2000.
******************************************************************/



/* List-header for all allocated memory. */

struct GC_Amiga_AllocedMemoryHeader{
	ULONG size;
	struct GC_Amiga_AllocedMemoryHeader *next;
};
struct GC_Amiga_AllocedMemoryHeader *GC_AMIGAMEM=(struct GC_Amiga_AllocedMemoryHeader *)(int)~(NULL);



/* Type of memory. Once in the execution of a program, this might change to MEMF_ANY|MEMF_CLEAR */

ULONG GC_AMIGA_MEMF = MEMF_FAST | MEMF_CLEAR;


/* Prevents GC_amiga_get_mem from allocating memory if this one is TRUE. */
#ifndef GC_AMIGA_ONLYFAST
BOOL GC_amiga_dontalloc=FALSE;
#endif

#ifdef GC_AMIGA_PRINTSTATS
int succ=0,succ2=0;
int nsucc=0,nsucc2=0;
int nullretries=0;
int numcollects=0;
int chipa=0;
int allochip=0;
int allocfast=0;
int cur0=0;
int cur1=0;
int cur10=0;
int cur50=0;
int cur150=0;
int cur151=0;
int ncur0=0;
int ncur1=0;
int ncur10=0;
int ncur50=0;
int ncur150=0;
int ncur151=0;
#endif

/* Free everything at program-end. */

void GC_amiga_free_all_mem(void){
	struct GC_Amiga_AllocedMemoryHeader *gc_am=(struct GC_Amiga_AllocedMemoryHeader *)(~(int)(GC_AMIGAMEM));
	struct GC_Amiga_AllocedMemoryHeader *temp;

#ifdef GC_AMIGA_PRINTSTATS
	printf("\n\n"
		"%d bytes of chip-mem, and %d bytes of fast-mem where allocated from the OS.\n",
		allochip,allocfast
	);
	printf(
		"%d bytes of chip-mem were returned from the GC_AMIGA_FASTALLOC supported allocating functions.\n",
		chipa
	);
	printf("\n");
	printf("GC_gcollect was called %d times to avoid returning NULL or start allocating with the MEMF_ANY flag.\n",numcollects);
	printf("%d of them was a success. (the others had to use allocation from the OS.)\n",nullretries);
	printf("\n");
	printf("Succeded forcing %d gc-allocations (%d bytes) of chip-mem to be fast-mem.\n",succ,succ2);
	printf("Failed forcing %d gc-allocations (%d bytes) of chip-mem to be fast-mem.\n",nsucc,nsucc2);
	printf("\n");
	printf(
		"Number of retries before succeding a chip->fast force:\n"
		"0: %d, 1: %d, 2-9: %d, 10-49: %d, 50-149: %d, >150: %d\n",
		cur0,cur1,cur10,cur50,cur150,cur151
	);
	printf(
		"Number of retries before giving up a chip->fast force:\n"
		"0: %d, 1: %d, 2-9: %d, 10-49: %d, 50-149: %d, >150: %d\n",
		ncur0,ncur1,ncur10,ncur50,ncur150,ncur151
	);
#endif

	while(gc_am!=NULL){
		temp=gc_am->next;
		FreeMem(gc_am,gc_am->size);
		gc_am=(struct GC_Amiga_AllocedMemoryHeader *)(~(int)(temp));
	}
}

#ifndef GC_AMIGA_ONLYFAST

/* All memory with address lower than this one is chip-mem. */

char *chipmax;


/*
 * Allways set to the last size of memory tried to be allocated.
 * Needed to ensure allocation when the size is bigger than 100000.
 *
 */
size_t latestsize;

#endif


/*
 * The actual function that is called with the GET_MEM macro.
 *
 */

void *GC_amiga_get_mem(size_t size){
	struct GC_Amiga_AllocedMemoryHeader *gc_am;

#ifndef GC_AMIGA_ONLYFAST
	if(GC_amiga_dontalloc==TRUE){
//		printf("rejected, size: %d, latestsize: %d\n",size,latestsize);
		return NULL;
	}

	// We really don't want to use chip-mem, but if we must, then as little as possible.
	if(GC_AMIGA_MEMF==(MEMF_ANY|MEMF_CLEAR) && size>100000 && latestsize<50000) return NULL;
#endif

	gc_am=AllocMem((ULONG)(size + sizeof(struct GC_Amiga_AllocedMemoryHeader)),GC_AMIGA_MEMF);
	if(gc_am==NULL) return NULL;

	gc_am->next=GC_AMIGAMEM;
	gc_am->size=size + sizeof(struct GC_Amiga_AllocedMemoryHeader);
	GC_AMIGAMEM=(struct GC_Amiga_AllocedMemoryHeader *)(~(int)(gc_am));

//	printf("Allocated %d (%d) bytes at address: %x. Latest: %d\n",size,tot,gc_am,latestsize);

#ifdef GC_AMIGA_PRINTSTATS
	if((char *)gc_am<chipmax){
		allochip+=size;
	}else{
		allocfast+=size;
	}
#endif

	return gc_am+1;

}




#ifndef GC_AMIGA_ONLYFAST

/* Tries very hard to force GC to find fast-mem to return. Done recursively
 * to hold the rejected memory-pointers reachable from the collector in an
 * easy way.
 *
 */
#ifdef GC_AMIGA_RETRY
void *GC_amiga_rec_alloc(size_t size,void *(*AllocFunction)(size_t size2),const int rec){
	void *ret;

	ret=(*AllocFunction)(size);

#ifdef GC_AMIGA_PRINTSTATS
	if((char *)ret>chipmax || ret==NULL){
		if(ret==NULL){
			nsucc++;
			nsucc2+=size;
			if(rec==0) ncur0++;
			if(rec==1) ncur1++;
			if(rec>1 && rec<10) ncur10++;
			if(rec>=10 && rec<50) ncur50++;
			if(rec>=50 && rec<150) ncur150++;
			if(rec>=150) ncur151++;
		}else{
			succ++;
			succ2+=size;
			if(rec==0) cur0++;
			if(rec==1) cur1++;
			if(rec>1 && rec<10) cur10++;
			if(rec>=10 && rec<50) cur50++;
			if(rec>=50 && rec<150) cur150++;
			if(rec>=150) cur151++;
		}
	}
#endif

	if (((char *)ret)<=chipmax && ret!=NULL && (rec<(size>500000?9:size/5000))){
		ret=GC_amiga_rec_alloc(size,AllocFunction,rec+1);
//		GC_free(ret2);
	}

	return ret;
}
#endif


/* The allocating-functions defined inside the amiga-blocks in gc.h is called
 * via these functions.
 */


void *GC_amiga_allocwrapper_any(size_t size,void *(*AllocFunction)(size_t size2)){
	void *ret,*ret2;

	GC_amiga_dontalloc=TRUE;	// Pretty tough thing to do, but its indeed necesarry.
	latestsize=size;

	ret=(*AllocFunction)(size);

	if(((char *)ret) <= chipmax){
		if(ret==NULL){
			//Give GC access to allocate memory.
#ifdef GC_AMIGA_GC
			if(!GC_dont_gc){
				GC_gcollect();
#ifdef GC_AMIGA_PRINTSTATS
				numcollects++;
#endif
				ret=(*AllocFunction)(size);
			}
#endif
			if(ret==NULL){
				GC_amiga_dontalloc=FALSE;
				ret=(*AllocFunction)(size);
				if(ret==NULL){
					WARN("Out of Memory!  Returning NIL!\n", 0);
				}
			}
#ifdef GC_AMIGA_PRINTSTATS
			else{
				nullretries++;
			}
			if(ret!=NULL && (char *)ret<=chipmax) chipa+=size;
#endif
		}
#ifdef GC_AMIGA_RETRY
		else{
			/* We got chip-mem. Better try again and again and again etc., we might get fast-mem sooner or later... */
			/* Using gctest to check the effectiviness of doing this, does seldom give a very good result. */
			/* However, real programs doesn't normally rapidly allocate and deallocate. */
//			printf("trying to force... %d bytes... ",size);
			if(
				AllocFunction!=GC_malloc_uncollectable
#ifdef ATOMIC_UNCOLLECTABLE
				&& AllocFunction!=GC_malloc_atomic_uncollectable
#endif
			){
				ret2=GC_amiga_rec_alloc(size,AllocFunction,0);
			}else{
				ret2=(*AllocFunction)(size);
#ifdef GC_AMIGA_PRINTSTATS
				if((char *)ret2<chipmax || ret2==NULL){
					nsucc++;
					nsucc2+=size;
					ncur0++;
				}else{
					succ++;
					succ2+=size;
					cur0++;
				}
#endif
			}
			if(((char *)ret2)>chipmax){
//				printf("Succeeded.\n");
				GC_free(ret);
				ret=ret2;
			}else{
				GC_free(ret2);
//				printf("But did not succeed.\n");
			}
		}
#endif
	}

	GC_amiga_dontalloc=FALSE;

	return ret;
}



void (*GC_amiga_toany)(void)=NULL;

void GC_amiga_set_toany(void (*func)(void)){
	GC_amiga_toany=func;
}

#endif // !GC_AMIGA_ONLYFAST


void *GC_amiga_allocwrapper_fast(size_t size,void *(*AllocFunction)(size_t size2)){
	void *ret;

	ret=(*AllocFunction)(size);

	if(ret==NULL){
		// Enable chip-mem allocation.
//		printf("ret==NULL\n");
#ifdef GC_AMIGA_GC
		if(!GC_dont_gc){
			GC_gcollect();
#ifdef GC_AMIGA_PRINTSTATS
			numcollects++;
#endif
			ret=(*AllocFunction)(size);
		}
#endif
		if(ret==NULL){
#ifndef GC_AMIGA_ONLYFAST
			GC_AMIGA_MEMF=MEMF_ANY | MEMF_CLEAR;
			if(GC_amiga_toany!=NULL) (*GC_amiga_toany)();
			GC_amiga_allocwrapper_do=GC_amiga_allocwrapper_any;
			return GC_amiga_allocwrapper_any(size,AllocFunction);
#endif
		}
#ifdef GC_AMIGA_PRINTSTATS
		else{
			nullretries++;
		}
#endif
	}

	return ret;
}

void *GC_amiga_allocwrapper_firsttime(size_t size,void *(*AllocFunction)(size_t size2)){
	atexit(&GC_amiga_free_all_mem);
	chipmax=(char *)SysBase->MaxLocMem;		// For people still having SysBase in chip-mem, this might speed up a bit.
	GC_amiga_allocwrapper_do=GC_amiga_allocwrapper_fast;
	return GC_amiga_allocwrapper_fast(size,AllocFunction);
}


#endif //GC_AMIGA_FASTALLOC



/*
 * The wrapped realloc function.
 *
 */
void *GC_amiga_realloc(void *old_object,size_t new_size_in_bytes){
#ifndef GC_AMIGA_FASTALLOC
	return GC_realloc(old_object,new_size_in_bytes);
#else
	void *ret;
	latestsize=new_size_in_bytes;
	ret=GC_realloc(old_object,new_size_in_bytes);
	if(ret==NULL && GC_AMIGA_MEMF==(MEMF_FAST | MEMF_CLEAR)){
		/* Out of fast-mem. */
#ifdef GC_AMIGA_GC
		if(!GC_dont_gc){
			GC_gcollect();
#ifdef GC_AMIGA_PRINTSTATS
			numcollects++;
#endif
			ret=GC_realloc(old_object,new_size_in_bytes);
		}
#endif
		if(ret==NULL){
#ifndef GC_AMIGA_ONLYFAST
			GC_AMIGA_MEMF=MEMF_ANY | MEMF_CLEAR;
			if(GC_amiga_toany!=NULL) (*GC_amiga_toany)();
			GC_amiga_allocwrapper_do=GC_amiga_allocwrapper_any;
			ret=GC_realloc(old_object,new_size_in_bytes);
#endif
		}
#ifdef GC_AMIGA_PRINTSTATS
		else{
			nullretries++;
		}
#endif
	}
	if(ret==NULL){
		WARN("Out of Memory!  Returning NIL!\n", 0);
	}
#ifdef GC_AMIGA_PRINTSTATS
	if(((char *)ret)<chipmax && ret!=NULL){
		chipa+=new_size_in_bytes;
	}
#endif
	return ret;
#endif
}

#endif //GC_AMIGA_AM



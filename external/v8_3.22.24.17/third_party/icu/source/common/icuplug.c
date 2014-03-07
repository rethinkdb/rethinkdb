/*
******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*  FILE NAME : icuplug.c
*
*   Date         Name        Description
*   10/29/2009   sl          New.
******************************************************************************
*/

#include "unicode/icuplug.h"
#include "icuplugimp.h"
#include "cstring.h"
#include "cmemory.h"
#include "putilimp.h"
#include "ucln.h"
#include <stdio.h>

#ifndef UPLUG_TRACE
#define UPLUG_TRACE 0
#endif

#if UPLUG_TRACE
#include <stdio.h>
#define DBG(x) fprintf(stderr, "%s:%d: ",__FILE__,__LINE__); fprintf x
#endif

/**
 * Internal structure of an ICU plugin. 
 */

struct UPlugData {
  UPlugEntrypoint  *entrypoint; /**< plugin entrypoint */
  uint32_t structSize;    /**< initialized to the size of this structure */
  uint32_t token;         /**< must be U_PLUG_TOKEN */
  void *lib;              /**< plugin library, or NULL */
  char libName[UPLUG_NAME_MAX];   /**< library name */
  char sym[UPLUG_NAME_MAX];        /**< plugin symbol, or NULL */
  char config[UPLUG_NAME_MAX];     /**< configuration data */
  void *context;          /**< user context data */
  char name[UPLUG_NAME_MAX];   /**< name of plugin */
  UPlugLevel  level; /**< level of plugin */
  UBool   awaitingLoad; /**< TRUE if the plugin is awaiting a load call */
  UBool   dontUnload; /**< TRUE if plugin must stay resident (leak plugin and lib) */
  UErrorCode pluginStatus; /**< status code of plugin */
};



#define UPLUG_LIBRARY_INITIAL_COUNT 8
#define UPLUG_PLUGIN_INITIAL_COUNT 12

/**
 * Remove an item
 * @param list the full list
 * @param listSize the number of entries in the list
 * @param memberSize the size of one member
 * @param itemToRemove the item number of the member
 * @return the new listsize 
 */
static int32_t uplug_removeEntryAt(void *list, int32_t listSize, int32_t memberSize, int32_t itemToRemove) {
  uint8_t *bytePtr = (uint8_t *)list;
    
  /* get rid of some bad cases first */
  if(listSize<1) {
    return listSize;
  }
    
  /* is there anything to move? */
  if(listSize > itemToRemove+1) {
    memmove(bytePtr+(itemToRemove*memberSize), bytePtr+((itemToRemove+1)*memberSize), memberSize);
  }
    
  return listSize-1;
}




#if U_ENABLE_DYLOAD
/**
 * Library management. Internal. 
 * @internal
 */
struct UPlugLibrary;

/**
 * Library management. Internal. 
 * @internal
 */
typedef struct UPlugLibrary {
  void *lib;                           /**< library ptr */
  char name[UPLUG_NAME_MAX]; /**< library name */
  uint32_t ref;                        /**< reference count */
} UPlugLibrary;

static UPlugLibrary   staticLibraryList[UPLUG_LIBRARY_INITIAL_COUNT];
static UPlugLibrary * libraryList = staticLibraryList;
static int32_t libraryCount = 0;
static int32_t libraryMax = UPLUG_LIBRARY_INITIAL_COUNT;

/**
 * Search for a library. Doesn't lock
 * @param libName libname to search for
 * @return the library's struct
 */
static int32_t searchForLibraryName(const char *libName) {
  int32_t i;
    
  for(i=0;i<libraryCount;i++) {
    if(!uprv_strcmp(libName, libraryList[i].name)) {
      return i;
    }
  }
  return -1;
}

static int32_t searchForLibrary(void *lib) {
  int32_t i;
    
  for(i=0;i<libraryCount;i++) {
    if(lib==libraryList[i].lib) {
      return i;
    }
  }
  return -1;
}

U_INTERNAL char * U_EXPORT2
uplug_findLibrary(void *lib, UErrorCode *status) {
  int32_t libEnt;
  char *ret = NULL;
  if(U_FAILURE(*status)) {
    return NULL;
  }
  libEnt = searchForLibrary(lib);
  if(libEnt!=-1) { 
    ret = libraryList[libEnt].name;
  } else {
    *status = U_MISSING_RESOURCE_ERROR;
  }
  return ret;
}

U_INTERNAL void * U_EXPORT2
uplug_openLibrary(const char *libName, UErrorCode *status) {
  int32_t libEntry = -1;
  void *lib = NULL;
    
  if(U_FAILURE(*status)) return NULL;

  libEntry = searchForLibraryName(libName);
  if(libEntry == -1) {
    libEntry = libraryCount++;
    if(libraryCount >= libraryMax) {
      /* Ran out of library slots. Statically allocated because we can't depend on allocating memory.. */
      *status = U_MEMORY_ALLOCATION_ERROR;
#if UPLUG_TRACE
      DBG((stderr, "uplug_openLibrary() - out of library slots (max %d)\n", libraryMax));
#endif
      return NULL;
    }
    /* Some operating systems don't want 
       DL operations from multiple threads. */
    libraryList[libEntry].lib = uprv_dl_open(libName, status);
#if UPLUG_TRACE
    DBG((stderr, "uplug_openLibrary(%s,%s) libEntry %d, lib %p\n", libName, u_errorName(*status), libEntry, lib));
#endif
        
    if(libraryList[libEntry].lib == NULL || U_FAILURE(*status)) {
      /* cleanup. */
      libraryList[libEntry].lib = NULL; /* failure with open */
      libraryList[libEntry].name[0] = 0;
#if UPLUG_TRACE
      DBG((stderr, "uplug_openLibrary(%s,%s) libEntry %d, lib %p\n", libName, u_errorName(*status), libEntry, lib));
#endif
      /* no need to free - just won't increase the count. */
      libraryCount--;
    } else { /* is it still there? */
      /* link it in */
      uprv_strncpy(libraryList[libEntry].name,libName,UPLUG_NAME_MAX);
      libraryList[libEntry].ref=1;
      lib = libraryList[libEntry].lib;
    }

  } else {
    lib = libraryList[libEntry].lib;
    libraryList[libEntry].ref++;
  }
  return lib;
}

U_INTERNAL void U_EXPORT2
uplug_closeLibrary(void *lib, UErrorCode *status) {
  int32_t i;
    
#if UPLUG_TRACE
  DBG((stderr, "uplug_closeLibrary(%p,%s) list %p\n", lib, u_errorName(*status), (void*)libraryList));
#endif
  if(U_FAILURE(*status)) return;
    
  for(i=0;i<libraryCount;i++) {
    if(lib==libraryList[i].lib) {
      if(--(libraryList[i].ref) == 0) {
        uprv_dl_close(libraryList[i].lib, status);
        libraryCount = uplug_removeEntryAt(libraryList, libraryCount, sizeof(*libraryList), i);
      }
      return;
    }
  }
  *status = U_INTERNAL_PROGRAM_ERROR; /* could not find the entry! */
}

#endif

static UPlugData pluginList[UPLUG_PLUGIN_INITIAL_COUNT];
static int32_t pluginCount = 0;



  
static int32_t uplug_pluginNumber(UPlugData* d) {
  UPlugData *pastPlug = &pluginList[pluginCount];
  if(d<=pluginList) {
    return 0;
  } else if(d>=pastPlug) {
    return pluginCount;
  } else {
    return (d-pluginList)/sizeof(pluginList[0]);
  }
}


U_CAPI UPlugData * U_EXPORT2
uplug_nextPlug(UPlugData *prior) {
  if(prior==NULL) {
    return pluginList;
  } else {
    UPlugData *nextPlug = &prior[1];
    UPlugData *pastPlug = &pluginList[pluginCount];
    
    if(nextPlug>=pastPlug) {
      return NULL;
    } else {
      return nextPlug;
    }
  }
}



/**
 * Call the plugin with some params
 */
static void uplug_callPlug(UPlugData *plug, UPlugReason reason, UErrorCode *status) {
  UPlugTokenReturn token;
  if(plug==NULL||U_FAILURE(*status)) {
    return;
  }
  token = (*(plug->entrypoint))(plug, reason, status);
  if(token!=UPLUG_TOKEN) {
    *status = U_INTERNAL_PROGRAM_ERROR;
  }
}


static void uplug_unloadPlug(UPlugData *plug, UErrorCode *status) {
  if(plug->awaitingLoad) {  /* shouldn't happen. Plugin hasn'tbeen loaded yet.*/
    *status = U_INTERNAL_PROGRAM_ERROR;
    return; 
  }
  if(U_SUCCESS(plug->pluginStatus)) {
    /* Don't unload a plug which has a failing load status - means it didn't actually load. */
    uplug_callPlug(plug, UPLUG_REASON_UNLOAD, status);
  }
}

static void uplug_queryPlug(UPlugData *plug, UErrorCode *status) {
  if(!plug->awaitingLoad || !(plug->level == UPLUG_LEVEL_UNKNOWN) ) {  /* shouldn't happen. Plugin hasn'tbeen loaded yet.*/
    *status = U_INTERNAL_PROGRAM_ERROR;
    return; 
  }
  plug->level = UPLUG_LEVEL_INVALID;
  uplug_callPlug(plug, UPLUG_REASON_QUERY, status);
  if(U_SUCCESS(*status)) { 
    if(plug->level == UPLUG_LEVEL_INVALID) {
      plug->pluginStatus = U_PLUGIN_DIDNT_SET_LEVEL;
      plug->awaitingLoad = FALSE;
    }
  } else {
    plug->pluginStatus = U_INTERNAL_PROGRAM_ERROR;
    plug->awaitingLoad = FALSE;
  }
}


static void uplug_loadPlug(UPlugData *plug, UErrorCode *status) {
  if(!plug->awaitingLoad || (plug->level < UPLUG_LEVEL_LOW) ) {  /* shouldn't happen. Plugin hasn'tbeen loaded yet.*/
    *status = U_INTERNAL_PROGRAM_ERROR;
    return;
  }
  uplug_callPlug(plug, UPLUG_REASON_LOAD, status);
  plug->awaitingLoad = FALSE;
  if(!U_SUCCESS(*status)) {
    plug->pluginStatus = U_INTERNAL_PROGRAM_ERROR;
  }
}

static UPlugData *uplug_allocateEmptyPlug(UErrorCode *status)
{
  UPlugData *plug = NULL;

  if(U_FAILURE(*status)) {
    return NULL;
  }

  if(pluginCount == UPLUG_PLUGIN_INITIAL_COUNT) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }

  plug = &pluginList[pluginCount++];

  plug->token = UPLUG_TOKEN;
  plug->structSize = sizeof(UPlugData);
  plug->name[0]=0;
  plug->level = UPLUG_LEVEL_UNKNOWN; /* initialize to null state */
  plug->awaitingLoad = TRUE;
  plug->dontUnload = FALSE;
  plug->pluginStatus = U_ZERO_ERROR;
  plug->libName[0] = 0;
  plug->config[0]=0;
  plug->sym[0]=0;
  plug->lib=NULL;
  plug->entrypoint=NULL;


  return plug;
}

static UPlugData *uplug_allocatePlug(UPlugEntrypoint *entrypoint, const char *config, void *lib, const char *symName,
                                     UErrorCode *status) {
  UPlugData *plug;

  if(U_FAILURE(*status)) {
    return NULL;
  }

  plug = uplug_allocateEmptyPlug(status);
  if(config!=NULL) {
    uprv_strncpy(plug->config, config, UPLUG_NAME_MAX);
  } else {
    plug->config[0] = 0;
  }
    
  if(symName!=NULL) {
    uprv_strncpy(plug->sym, symName, UPLUG_NAME_MAX);
  } else {
    plug->sym[0] = 0;
  }
    
  plug->entrypoint = entrypoint;
  plug->lib = lib;
  uplug_queryPlug(plug, status);
    
  return plug;
}

static void uplug_deallocatePlug(UPlugData *plug, UErrorCode *status) {
  UErrorCode subStatus = U_ZERO_ERROR;
  if(!plug->dontUnload) {
#if U_ENABLE_DYLOAD
    uplug_closeLibrary(plug->lib, &subStatus);
#endif
  }
  plug->lib = NULL;
  if(U_SUCCESS(*status) && U_FAILURE(subStatus)) {
    *status = subStatus;
  }
  /* shift plugins up and decrement count. */
  if(U_SUCCESS(*status)) {
    /* all ok- remove. */
    pluginCount = uplug_removeEntryAt(pluginList, pluginCount, sizeof(plug[0]), uplug_pluginNumber(plug));
  } else {
    /* not ok- leave as a message. */
    plug->awaitingLoad=FALSE;
    plug->entrypoint=0;
    plug->dontUnload=TRUE;
  }
}

static void uplug_doUnloadPlug(UPlugData *plugToRemove, UErrorCode *status) {
  if(plugToRemove != NULL) {
    uplug_unloadPlug(plugToRemove, status);
    uplug_deallocatePlug(plugToRemove, status);
  }
}

U_CAPI void U_EXPORT2
uplug_removePlug(UPlugData *plug, UErrorCode *status)  {
  UPlugData *cursor = NULL;
  UPlugData *plugToRemove = NULL;
  if(U_FAILURE(*status)) return;
    
  for(cursor=pluginList;cursor!=NULL;) {
    if(cursor==plug) {
      plugToRemove = plug;
      cursor=NULL;
    } else {
      cursor = uplug_nextPlug(cursor);
    }
  }
    
  uplug_doUnloadPlug(plugToRemove, status);
}




U_CAPI void U_EXPORT2 
uplug_setPlugNoUnload(UPlugData *data, UBool dontUnload)
{
  data->dontUnload = dontUnload;
}


U_CAPI void U_EXPORT2
uplug_setPlugLevel(UPlugData *data, UPlugLevel level) {
  data->level = level;
}


U_CAPI UPlugLevel U_EXPORT2
uplug_getPlugLevel(UPlugData *data) {
  return data->level;
}


U_CAPI void U_EXPORT2
uplug_setPlugName(UPlugData *data, const char *name) {
  uprv_strncpy(data->name, name, UPLUG_NAME_MAX);
}


U_CAPI const char * U_EXPORT2
uplug_getPlugName(UPlugData *data) {
  return data->name;
}


U_CAPI const char * U_EXPORT2
uplug_getSymbolName(UPlugData *data) {
  return data->sym;
}

U_CAPI const char * U_EXPORT2
uplug_getLibraryName(UPlugData *data, UErrorCode *status) {
  if(data->libName[0]) {
    return data->libName;
  } else {
#if U_ENABLE_DYLOAD
    return uplug_findLibrary(data->lib, status);
#else
    return NULL;
#endif
  }
}

U_CAPI void * U_EXPORT2
uplug_getLibrary(UPlugData *data) {
  return data->lib;
}

U_CAPI void * U_EXPORT2
uplug_getContext(UPlugData *data) {
  return data->context;
}


U_CAPI void U_EXPORT2
uplug_setContext(UPlugData *data, void *context) {
  data->context = context;
}

U_CAPI const char* U_EXPORT2
uplug_getConfiguration(UPlugData *data) {
  return data->config;
}

U_INTERNAL UPlugData* U_EXPORT2
uplug_getPlugInternal(int32_t n) { 
  if(n <0 || n >= pluginCount) {
    return NULL;
  } else { 
    return &(pluginList[n]);
  }
}


U_CAPI UErrorCode U_EXPORT2
uplug_getPlugLoadStatus(UPlugData *plug) {
  return plug->pluginStatus;
}




/**
 * Initialize a plugin fron an entrypoint and library - but don't load it.
 */
static UPlugData* uplug_initPlugFromEntrypointAndLibrary(UPlugEntrypoint *entrypoint, const char *config, void *lib, const char *sym,
                                                         UErrorCode *status) {
  UPlugData *plug = NULL;

  plug = uplug_allocatePlug(entrypoint, config, lib, sym, status);

  if(U_SUCCESS(*status)) {
    return plug;
  } else {
    uplug_deallocatePlug(plug, status);
    return NULL;
  }
}

U_CAPI UPlugData* U_EXPORT2
uplug_loadPlugFromEntrypoint(UPlugEntrypoint *entrypoint, const char *config, UErrorCode *status) {
  UPlugData* plug = uplug_initPlugFromEntrypointAndLibrary(entrypoint, config, NULL, NULL, status);
  uplug_loadPlug(plug, status);
  return plug;
}


static UPlugData* 
uplug_initErrorPlug(const char *libName, const char *sym, const char *config, const char *nameOrError, UErrorCode loadStatus, UErrorCode *status)
{
  UPlugData *plug = uplug_allocateEmptyPlug(status);
  if(U_FAILURE(*status)) return NULL;

  plug->pluginStatus = loadStatus;
  plug->awaitingLoad = FALSE; /* Won't load. */
  plug->dontUnload = TRUE; /* cannot unload. */

  if(sym!=NULL) {
    uprv_strncpy(plug->sym, sym, UPLUG_NAME_MAX);
  }

  if(libName!=NULL) {
    uprv_strncpy(plug->libName, libName, UPLUG_NAME_MAX);
  }

  if(nameOrError!=NULL) {
    uprv_strncpy(plug->name, nameOrError, UPLUG_NAME_MAX);
  }

  if(config!=NULL) {
    uprv_strncpy(plug->config, config, UPLUG_NAME_MAX);
  }

  return plug;
}

/**
 * Fetch a plugin from DLL, and then initialize it from a library- but don't load it.
 */

#if U_ENABLE_DYLOAD

static UPlugData* 
uplug_initPlugFromLibrary(const char *libName, const char *sym, const char *config, UErrorCode *status) {
  void *lib = NULL;
  UPlugData *plug = NULL;
  if(U_FAILURE(*status)) { return NULL; }
  lib = uplug_openLibrary(libName, status);
  if(lib!=NULL && U_SUCCESS(*status)) {
    UPlugEntrypoint *entrypoint = NULL;
    /*
     * ISO forbids the following cast.
     *  See: http://www.trilithium.com/johan/2004/12/problem-with-dlsym/ 
     */
    entrypoint = (UPlugEntrypoint*)uprv_dl_sym(lib, sym, status);
      
    if(entrypoint!=NULL&&U_SUCCESS(*status)) {
      plug = uplug_initPlugFromEntrypointAndLibrary(entrypoint, config, lib, sym, status);
      if(plug!=NULL&&U_SUCCESS(*status)) {
        plug->lib = lib; /* plug takes ownership of library */
        lib = NULL; /* library is now owned by plugin. */
      }
    } else {
      UErrorCode subStatus = U_ZERO_ERROR;
      plug = uplug_initErrorPlug(libName,sym,config,"ERROR: Could not load entrypoint",(lib==NULL)?U_MISSING_RESOURCE_ERROR:*status,&subStatus);
    }
    if(lib!=NULL) { /* still need to close the lib */
      UErrorCode subStatus = U_ZERO_ERROR;
      uplug_closeLibrary(lib, &subStatus); /* don't care here */
    }
  } else {
    UErrorCode subStatus = U_ZERO_ERROR;
    plug = uplug_initErrorPlug(libName,sym,config,"ERROR: could not load library",(lib==NULL)?U_MISSING_RESOURCE_ERROR:*status,&subStatus);
  }
  return plug;
}

U_CAPI UPlugData* U_EXPORT2
uplug_loadPlugFromLibrary(const char *libName, const char *sym, const char *config, UErrorCode *status) { 
  UPlugData *plug = NULL;
  if(U_FAILURE(*status)) { return NULL; }
  plug = uplug_initPlugFromLibrary(libName, sym, config, status);
  uplug_loadPlug(plug, status);

  return plug;
}

#endif

U_CAPI UPlugLevel U_EXPORT2 uplug_getCurrentLevel() {
  if(cmemory_inUse()) {
    return UPLUG_LEVEL_HIGH;
  } else {
    return UPLUG_LEVEL_LOW;
  }
}

static UBool U_CALLCONV uplug_cleanup(void)
{
  int32_t i;
    
  UPlugData *pluginToRemove;
  /* cleanup plugs */
  for(i=0;i<pluginCount;i++) {
    UErrorCode subStatus = U_ZERO_ERROR;
    pluginToRemove = &pluginList[i];
    /* unload and deallocate */
    uplug_doUnloadPlug(pluginToRemove, &subStatus);
  }
  /* close other held libs? */
  return TRUE;
}

static void uplug_loadWaitingPlugs(UErrorCode *status) {
  int32_t i;
  UPlugLevel currentLevel = uplug_getCurrentLevel();
    
  if(U_FAILURE(*status)) {
    return;
  }
#if UPLUG_TRACE
  DBG((stderr,  "uplug_loadWaitingPlugs() Level: %d\n", currentLevel));
#endif
  /* pass #1: low level plugs */
  for(i=0;i<pluginCount;i++) {
    UErrorCode subStatus = U_ZERO_ERROR;
    UPlugData *pluginToLoad = &pluginList[i];
    if(pluginToLoad->awaitingLoad) {
      if(pluginToLoad->level == UPLUG_LEVEL_LOW) {
        if(currentLevel > UPLUG_LEVEL_LOW) {
          pluginToLoad->pluginStatus = U_PLUGIN_TOO_HIGH;
        } else {
          UPlugLevel newLevel;
          uplug_loadPlug(pluginToLoad, &subStatus);
          newLevel = uplug_getCurrentLevel();
          if(newLevel > currentLevel) {
            pluginToLoad->pluginStatus = U_PLUGIN_CHANGED_LEVEL_WARNING;
            currentLevel = newLevel;
          }
        }
        pluginToLoad->awaitingLoad = FALSE;
      } 
    }
  }
  currentLevel = uplug_getCurrentLevel();
    
  for(i=0;i<pluginCount;i++) {
    UErrorCode subStatus = U_ZERO_ERROR;
    UPlugData *pluginToLoad = &pluginList[i];
        
    if(pluginToLoad->awaitingLoad) {
      if(pluginToLoad->level == UPLUG_LEVEL_INVALID) { 
        pluginToLoad->pluginStatus = U_PLUGIN_DIDNT_SET_LEVEL;
      } else if(pluginToLoad->level == UPLUG_LEVEL_UNKNOWN) {
        pluginToLoad->pluginStatus = U_INTERNAL_PROGRAM_ERROR;
      } else {
        uplug_loadPlug(pluginToLoad, &subStatus);
      }
      pluginToLoad->awaitingLoad = FALSE;
    }
  }
    
#if UPLUG_TRACE
  DBG((stderr,  " Done Loading Plugs. Level: %d\n", (int32_t)uplug_getCurrentLevel()));
#endif
}

#if U_ENABLE_DYLOAD
/* Name of the plugin config file */
static char plugin_file[2048] = "";
#endif

U_INTERNAL const char* U_EXPORT2
uplug_getPluginFile() {
#if U_ENABLE_DYLOAD
  return plugin_file;
#else
  return NULL;
#endif
}


U_CAPI void U_EXPORT2
uplug_init(UErrorCode *status) {
#if !U_ENABLE_DYLOAD
  (void)status; /* unused */
#else
  const char *plugin_dir;

  if(U_FAILURE(*status)) return;
  plugin_dir = getenv("ICU_PLUGINS");

#if defined(DEFAULT_ICU_PLUGINS) 
  if(plugin_dir == NULL || !*plugin_dir) {
    plugin_dir = DEFAULT_ICU_PLUGINS;
  }
#endif

#if UPLUG_TRACE
  DBG((stderr, "ICU_PLUGINS=%s\n", plugin_dir));
#endif

  if(plugin_dir != NULL && *plugin_dir) {
    FILE *f;
        
        
    uprv_strncpy(plugin_file, plugin_dir, 2047);
    uprv_strncat(plugin_file, U_FILE_SEP_STRING,2047);
    uprv_strncat(plugin_file, "icuplugins",2047);
    uprv_strncat(plugin_file, U_ICU_VERSION_SHORT ,2047);
    uprv_strncat(plugin_file, ".txt" ,2047);
        
#if UPLUG_TRACE
    DBG((stderr, "pluginfile= %s\n", plugin_file));
#endif
        
    f = fopen(plugin_file, "r");

    if(f != NULL) {
      char linebuf[1024];
      char *p, *libName=NULL, *symName=NULL, *config=NULL;
      int32_t line = 0;
            
            
      while(fgets(linebuf,1023,f)) {
        line++;

        if(!*linebuf || *linebuf=='#') {
          continue;
        } else {
          p = linebuf;
          while(*p&&isspace(*p))
            p++;
          if(!*p || *p=='#') continue;
          libName = p;
          while(*p&&!isspace(*p)) {
            p++;
          }
          if(!*p || *p=='#') continue; /* no tab after libname */
          *p=0; /* end of libname */
          p++;
          while(*p&&isspace(*p)) {
            p++;
          }
          if(!*p||*p=='#') continue; /* no symname after libname +tab */
          symName = p;
          while(*p&&!isspace(*p)) {
            p++;
          }
                    
          if(*p) { /* has config */
            *p=0;
            ++p;
            while(*p&&isspace(*p)) {
              p++;
            }
            if(*p) {
              config = p;
            }
          }
                    
          /* chop whitespace at the end of the config */
          if(config!=NULL&&*config!=0) {
            p = config+strlen(config);
            while(p>config&&isspace(*(--p))) {
              *p=0;
            }
          }
                
          /* OK, we're good. */
          { 
            UErrorCode subStatus = U_ZERO_ERROR;
            UPlugData *plug = uplug_initPlugFromLibrary(libName, symName, config, &subStatus);
            if(U_FAILURE(subStatus) && U_SUCCESS(*status)) {
              *status = subStatus;
            }
#if UPLUG_TRACE
            DBG((stderr, "PLUGIN libName=[%s], sym=[%s], config=[%s]\n", libName, symName, config));
            DBG((stderr, " -> %p, %s\n", (void*)plug, u_errorName(subStatus)));
#else
            (void)plug; /* unused */
#endif
          }
        }
      }
    } else {
#if UPLUG_TRACE
      DBG((stderr, "Can't open plugin file %s\n", plugin_file));
#endif
    }
  }
  uplug_loadWaitingPlugs(status);
#endif /* U_ENABLE_DYLOAD */
  ucln_registerCleanup(UCLN_UPLUG, uplug_cleanup);
}

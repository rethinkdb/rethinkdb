# This file generates all of man/html pages that we use for documentation.
#
# When hacking this file you need to know that we take .pod files and turn
# them into .pop files. .pop files are 1=1 for man pages, but one .pod
# file may generate many .pop files.
#
#    -Brian
#
#
CLEANFILES= *.1 *.3 *.html *.pop *.tmp

BUILT_SOURCES=

EXTRA_DIST= make_index.pl

AUTO_PAGES= \
	    memcached_increment.pop \
	    memcached_increment_with_initial.pop \
	    memcached_decrement.pop \
	    memcached_decrement_with_initial.pop
BUILT_SOURCES += ${AUTO_PAGES}

BEHAVIOR_PAGES= \
		memcached_behavior_get.pop \
		memcached_behavior_set.pop
BUILT_SOURCES += ${BEHAVIOR_PAGES}

CALLBACK_PAGES= \
		memcached_callback_get.pop \
		memcached_callback_set.pop
BUILT_SOURCES += ${CALLBACK_PAGES}

CREATE_PAGES= \
	      memcached_clone.pop \
	      memcached_create.pop \
	      memcached_free.pop \
	      memcached_servers_reset.pop
BUILT_SOURCES += ${CREATE_PAGES}

DELETE_PAGES= \
	      memcached_delete.pop \
	      memcached_delete_by_key.pop
BUILT_SOURCES += ${DELETE_PAGES}

GENERIC_PAGES= \
	       libmemcached.pop \
	       libmemcached_examples.pop \
	       libmemcachedutil.pop \
	       memcached_analyze.pop \
	       memcached_dump.pop \
	       memcached_flush.pop \
	       memcached_flush_buffers.pop \
	       memcached_generate_hash_value.pop \
	       memcached_quit.pop \
	       memcached_strerror.pop \
	       memcached_verbosity.pop \
	       memcapable.pop \
	       memcat.pop \
	       memcp.pop \
	       memdump.pop \
	       memerror.pop \
	       memflush.pop \
	       memrm.pop \
	       memslap.pop \
	       memstat.pop
BUILT_SOURCES += ${GENERIC_PAGES}

GET_PAGES= \
	   memcached_get.pop \
	   memcached_get_by_key.pop \
	   memcached_fetch_result.pop \
	   memcached_fetch_execute.pop \
	   memcached_mget.pop \
	   memcached_mget_by_key.pop \
	   memcached_mget_execute.pop \
	   memcached_mget_execute_by_key.pop \
	   memcached_fetch.pop
BUILT_SOURCES += ${GET_PAGES}

MEMORY_ALLOCATORS_PAGES= \
			 memcached_get_memory_allocators.pop \
			 memcached_set_memory_allocators.pop \
			 memcached_set_memory_allocators_context.pop
BUILT_SOURCES += ${MEMORY_ALLOCATORS_PAGES}

POOL_PAGES= \
	    memcached_pool_behavior_get.pop \
	    memcached_pool_behavior_set.pop \
	    memcached_pool_create.pop \
	    memcached_pool_destroy.pop \
	    memcached_pool_pop.pop \
	    memcached_pool_push.pop
BUILT_SOURCES += ${POOL_PAGES}

RESULT_PAGES= \
	      memcached_result_cas.pop \
	      memcached_result_create.pop \
	      memcached_result_flags.pop \
	      memcached_result_free.pop \
	      memcached_result_key_length.pop \
	      memcached_result_key_value.pop \
	      memcached_result_length.pop \
	      memcached_result_st.pop \
	      memcached_result_value.pop
BUILT_SOURCES += ${RESULT_PAGES}


SERVER_PAGES= \
	      memcached_server_count.pop \
	      memcached_server_cursor.pop \
	      memcached_server_list.pop \
	      memcached_server_add.pop \
	      memcached_server_add_unix_socket.pop \
	      memcached_server_push.pop
BUILT_SOURCES += ${SERVER_PAGES}

SERVER_ST_PAGES= \
		 memcached_server_list_free.pop \
		 memcached_server_list_count.pop \
		 memcached_server_list_append.pop \
		 memcached_servers_parse.pop
BUILT_SOURCES += ${SERVER_ST_PAGES}

SET_PAGES= \
	   memcached_set.pop \
	   memcached_set_by_key.pop \
	   memcached_cas.pop \
	   memcached_cas_by_key.pop \
	   memcached_replace.pop \
	   memcached_replace_by_key.pop \
	   memcached_add.pop \
	   memcached_add_by_key.pop \
	   memcached_prepend.pop \
	   memcached_prepend_by_key.pop \
	   memcached_append.pop \
	   memcached_append_by_key.pop
BUILT_SOURCES += ${SET_PAGES}

STATS_PAGES= \
	     memcached_stat.pop \
	     memcached_stat_execute.pop\
	     memcached_stat_get_keys.pop\
	     memcached_stat_get_value.pop \
	     memcached_stat_servername.pop
BUILT_SOURCES += ${STATS_PAGES}

USER_DATA_PAGES= \
		 memcached_get_user_data.pop \
		 memcached_set_user_data.pop
BUILT_SOURCES += ${USER_DATA_PAGES}

VERSION_PAGES= \
	       memcached_version.pop \
	       memcached_lib_version.pop
BUILT_SOURCES += ${VERSION_PAGES}


#
# These are for libhashkit
#
HASHKIT_CREATE_PAGES= \
		      hashkit_is_allocated.pop \
		      hashkit_create.pop \
		      hashkit_clone.pop \
		      hashkit_free.pop
BUILT_SOURCES += ${HASHKIT_CREATE_PAGES}

HASHKIT_FUNCTIONS_PAGES= \
			 hashkit_crc32.pop \
			 hashkit_fnv1_32.pop \
			 hashkit_fnv1_64.pop \
			 hashkit_fnv1a_32.pop \
			 hashkit_fnv1a_64.pop \
			 hashkit_functions.pop \
			 hashkit_hsieh.pop \
			 hashkit_jenkins.pop \
			 hashkit_md5.pop \
			 hashkit_murmur.pop
BUILT_SOURCES += ${HASHKIT_FUNCTIONS_PAGES}

HASHKIT_ST_PAGES= \
		  hashkit_value.pop
BUILT_SOURCES += ${HASHKIT_ST_PAGES}


HTML_FILES= \
	    hashkit_clone.html \
	    hashkit_crc32.html \
	    hashkit_create.html \
	    hashkit_fnv1_32.html \
	    hashkit_fnv1_64.html \
	    hashkit_fnv1a_32.html \
	    hashkit_fnv1a_64.html \
	    hashkit_free.html \
	    hashkit_functions.html \
	    hashkit_hsieh.html \
	    hashkit_is_allocated.html \
	    hashkit_jenkins.html \
	    hashkit_md5.html \
	    hashkit_murmur.html \
	    hashkit_value.html \
	    libmemcached_examples.html \
	    libmemcached.html \
	    libmemcachedutil.html \
	    memcached_add_by_key.html \
	    memcached_add.html \
	    memcached_analyze.html \
	    memcached_append_by_key.html \
	    memcached_append.html \
	    memcached_behavior_get.html \
	    memcached_behavior_set.html \
	    memcached_callback_get.html \
	    memcached_callback_set.html \
	    memcached_cas_by_key.html \
	    memcached_cas.html \
	    memcached_clone.html \
	    memcached_create.html \
	    memcached_decrement.html \
	    memcached_decrement_with_initial.html \
	    memcached_delete_by_key.html \
	    memcached_delete.html \
	    memcached_dump.html \
	    memcached_fetch_execute.html \
	    memcached_fetch.html \
	    memcached_fetch_result.html \
	    memcached_flush_buffers.html \
	    memcached_flush.html \
	    memcached_free.html \
	    memcached_generate_hash_value.html \
	    memcached_get_by_key.html \
	    memcached_get_memory_allocators.html \
	    memcached_get.html \
	    memcached_get_user_data.html \
	    memcached_increment.html \
	    memcached_increment_with_initial.html \
	    memcached_lib_version.html \
	    memcached_mget_by_key.html \
	    memcached_mget_execute_by_key.html \
	    memcached_mget_execute.html \
	    memcached_mget.html \
	    memcached_pool_behavior_get.html \
	    memcached_pool_behavior_set.html \
	    memcached_pool_create.html \
	    memcached_pool_destroy.html \
	    memcached_pool_pop.html \
	    memcached_pool_push.html \
	    memcached_prepend_by_key.html \
	    memcached_prepend.html \
	    memcached_quit.html \
	    memcached_replace_by_key.html \
	    memcached_replace.html \
	    memcached_result_cas.html \
	    memcached_result_create.html \
	    memcached_result_flags.html \
	    memcached_result_free.html \
	    memcached_result_key_length.html \
	    memcached_result_key_value.html \
	    memcached_result_length.html \
	    memcached_result_st.html \
	    memcached_result_value.html \
	    memcached_server_add.html \
	    memcached_server_add_unix_socket.html \
	    memcached_server_count.html \
	    memcached_server_cursor.html \
	    memcached_server_list_append.html \
	    memcached_server_list_count.html \
	    memcached_server_list_free.html \
	    memcached_server_list.html \
	    memcached_server_push.html \
	    memcached_servers_parse.html \
	    memcached_set_by_key.html \
	    memcached_set_memory_allocators.html \
	    memcached_set.html \
	    memcached_set_user_data.html \
	    memcached_stat_execute.html \
	    memcached_stat_get_keys.html \
	    memcached_stat_get_value.html \
	    memcached_stat.html \
	    memcached_stat_servername.html \
	    memcached_strerror.html \
	    memcached_verbosity.html \
	    memcached_version.html \
	    memcapable.html \
	    memcat.html \
	    memcp.html \
	    memdump.html \
	    memerror.html \
	    memflush.html \
	    memrm.html \
	    memslap.html \
	    memstat.html

POD_FILES= \
	   hashkit_create.pod \
	   hashkit_functions.pod \
	   hashkit_value.pod \
	   libmemcached.pod \
	   libmemcached_examples.pod \
	   libmemcachedutil.pod \
	   memcached_analyze.pod \
	   memcached_auto.pod \
	   memcached_behavior.pod \
	   memcached_callback.pod \
	   memcached_create.pod \
	   memcached_delete.pod \
	   memcached_dump.pod \
	   memcached_flush.pod \
	   memcached_flush_buffers.pod \
	   memcached_generate_hash_value.pod \
	   memcached_get.pod \
	   memcached_memory_allocators.pod \
	   memcached_pool.pod \
	   memcached_quit.pod \
	   memcached_result_st.pod \
	   memcached_sasl.pod \
	   memcached_server_st.pod \
	   memcached_servers.pod \
	   memcached_set.pod \
	   memcached_stats.pod \
	   memcached_strerror.pod \
	   memcached_user_data.pod \
	   memcached_verbosity.pod \
	   memcached_version.pod \
	   memcapable.pod \
	   memcat.pod \
	   memcp.pod \
	   memdump.pod \
	   memerror.pod \
	   memflush.pod \
	   memrm.pod \
	   memslap.pod \
	   memstat.pod
EXTRA_DIST+= $(POD_FILES)

man_MANS = \
	   hashkit_clone.3 \
	   hashkit_crc32.3 \
	   hashkit_create.3 \
	   hashkit_fnv1_32.3 \
	   hashkit_fnv1_64.3 \
	   hashkit_fnv1a_32.3 \
	   hashkit_fnv1a_64.3 \
	   hashkit_free.3 \
	   hashkit_functions.3 \
	   hashkit_hsieh.3 \
	   hashkit_is_allocated.3 \
	   hashkit_jenkins.3 \
	   hashkit_md5.3 \
	   hashkit_murmur.3 \
	   hashkit_value.3 \
	   libmemcached.3 \
	   libmemcached_examples.3 \
	   memcached_add.3 \
	   memcached_add_by_key.3 \
	   memcached_analyze.3 \
	   memcached_append.3 \
	   memcached_append_by_key.3 \
	   memcached_behavior_get.3 \
	   memcached_behavior_set.3 \
	   memcached_callback_get.3 \
	   memcached_callback_set.3 \
	   memcached_cas.3 \
	   memcached_cas_by_key.3 \
	   memcached_clone.3 \
	   memcached_create.3 \
	   memcached_decrement.3 \
	   memcached_decrement_with_initial.3 \
	   memcached_delete.3 \
	   memcached_delete_by_key.3 \
	   memcached_dump.3 \
	   memcached_fetch.3 \
	   memcached_fetch_execute.3 \
	   memcached_fetch_result.3 \
	   memcached_flush_buffers.3 \
	   memcached_free.3 \
	   memcached_generate_hash_value.3 \
	   memcached_get.3 \
	   memcached_get_by_key.3 \
	   memcached_get_memory_allocators.3 \
	   memcached_get_user_data.3 \
	   memcached_increment.3 \
	   memcached_increment_with_initial.3 \
	   memcached_lib_version.3 \
	   memcached_mget.3 \
	   memcached_mget_by_key.3 \
	   memcached_mget_execute.3  \
	   memcached_mget_execute_by_key.3  \
	   memcached_prepend.3 \
	   memcached_prepend_by_key.3 \
	   memcached_quit.3 \
	   memcached_replace.3 \
	   memcached_replace_by_key.3 \
	   memcached_server_add.3 \
	   memcached_server_count.3 \
	   memcached_server_cursor.3 \
	   memcached_server_list.3 \
	   memcached_server_list_append.3 \
	   memcached_server_list_count.3 \
	   memcached_server_list_free.3 \
	   memcached_server_push.3 \
	   memcached_servers_parse.3 \
	   memcached_set.3 \
	   memcached_set_by_key.3 \
	   memcached_set_memory_allocators.3 \
	   memcached_set_user_data.3 \
	   memcached_stat.3 \
	   memcached_stat_execute.3 \
	   memcached_stat_get_keys.3 \
	   memcached_stat_get_value.3 \
	   memcached_stat_servername.3 \
	   memcached_strerror.3 \
	   memcached_verbosity.3 \
	   memcached_version.3 \
	   memcapable.1 \
	   memcat.1 \
	   memcp.1 \
	   memdump.1 \
	   memerror.1 \
	   memflush.1 \
	   memrm.1 \
	   memslap.1 \
	   memstat.1

if HAVE_SASL
POD_FILES+= memcached_sasl.pod
man_MANS+= \
           memcached_destroy_sasl_auth_data.3 \
           memcached_get_sasl_callbacks.3 \
           memcached_sasl_set_auth_data.3 \
           memcached_set_sasl_callbacks.3

HTML_FILES+= \
           memcached_destroy_sasl_auth_data.html \
           memcached_get_sasl_callbacks.html \
           memcached_sasl_set_auth_data.html \
           memcached_set_sasl_callbacks.html


SASL_PAGES= \
           memcached_destroy_sasl_auth_data.pop \
           memcached_get_sasl_callbacks.pop \
           memcached_sasl_set_auth_data.pop \
           memcached_set_sasl_callbacks.pop
BUILT_SOURCES += ${SASL_PAGES}

${SASL_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_sasl.pod ${top_builddir}/docs/$@

endif

if BUILD_LIBMEMCACHEDUTIL
man_MANS+= \
	  libmemcachedutil.3 \
	  memcached_pool_behavior_set.3 \
	  memcached_pool_behavior_get.3 \
	  memcached_pool_create.3 \
	  memcached_pool_destroy.3 \
	  memcached_pool_push.3 \
	  memcached_pool_pop.3
endif


${CREATE_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_create.pod ${top_builddir}/docs/$@

${SET_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_set.pod ${top_builddir}/docs/$@

${DELETE_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_delete.pod ${top_builddir}/docs/$@

${AUTO_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_auto.pod ${top_builddir}/docs/$@

${GET_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_get.pod ${top_builddir}/docs/$@

${SERVER_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_servers.pod ${top_builddir}/docs/$@

${SERVER_ST_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_server_st.pod ${top_builddir}/docs/$@

${GENERIC_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/$*.pod ${top_builddir}/docs/$@

${BEHAVIOR_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_behavior.pod ${top_builddir}/docs/$@

${CALLBACK_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_callback.pod ${top_builddir}/docs/$@

${STATS_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_stats.pod ${top_builddir}/docs/$@

${RESULT_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_result_st.pod ${top_builddir}/docs/$@

${VERSION_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_version.pod ${top_builddir}/docs/$@

${MEMORY_ALLOCATORS_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_memory_allocators.pod ${top_builddir}/docs/$@

${USER_DATA_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_user_data.pod ${top_builddir}/docs/$@

${POOL_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/memcached_pool.pod ${top_builddir}/docs/$@

${HASHKIT_CREATE_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/hashkit_create.pod ${top_builddir}/docs/$@


${HASHKIT_FUNCTIONS_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/hashkit_functions.pod ${top_builddir}/docs/$@

${HASHKIT_ST_PAGES}:
	@rm -f $@
	ln -s ${top_srcdir}/docs/hashkit_value.pod ${top_builddir}/docs/$@

test-docs: $(POD_FILES)
	${PODCHECKER} $(top_srcdir)/docs/$?

html-local: html-pages html-index

html-pages: $(HTML_FILES)

html-index: html-pages
	perl make_index.pl *.html > index.html

SUFFIXES= .pop .pod .html .1 .3

.pop: ${_set}

.pop.html:
	pod2html --infile=$< >  $@

.pop.1:
	${POD2MAN} -c "$*" -r "" -s 1 $< >  $@

.pop.3:
	${POD2MAN} -c "$*" -r "" -s 3 $< >  $@

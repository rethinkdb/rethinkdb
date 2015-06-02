// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/persist.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <functional>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/thread_pool.hpp"
#include "buffer_cache/alt.hpp"
#include "buffer_cache/blob.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "concurrency/throttled_committer.hpp"
#include "containers/archive/buffer_group_stream.hpp"
#include "clustering/administration/pre_v1_16_metadata.hpp"
#include "clustering/immediate_consistency/branch/history.hpp"
#include "serializer/config.hpp"

pre_v1_16::cluster_semilattice_metadata_t old_metadata;
read_stream_t *s;
auto f = [&]() -> archive_result_t {
	return deserialize<cluster_version_t::v1_13>(s, &old_metadata);
};

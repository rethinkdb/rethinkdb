// Copyright (C) 2004-2006 The Trustees of Indiana University.
// Copyright (C) 2007  Douglas Gregor  <doug.gregor@gmail.com>
// Copyright (C) 2007  Matthias Troyer  <troyer@boost-consulting.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
//           Matthias Troyer
#include <boost/assert.hpp>
#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/lexical_cast.hpp>
#include <memory>
#include <algorithm>

//#define DEBUG 1


//#define MAX_BATCHES 1500
#define PREALLOCATE_BATCHES 250  
// 500 is a better setting for PREALLOCATE_BATCHES if you're not using process 
// subgroups and are building 64-bit binaries.  250 allows all the CTest
// tests to pass in both 32 and 64-bit modes.  If you create multiple process 
// groups with PREALLOCATE_BATCHES at a reasonable level in 32-bit mode you 
// _will_ run out of memory and get "malloc failed" errors

//#define NO_ISEND_BATCHES
//#define NO_IMMEDIATE_PROCESSING
//#define NO_SPLIT_BATCHES
#define IRECV_BATCH

// we cannot keep track of how many we received if we do not process them
#ifdef NO_IMMEDIATE_PROCESSING
#undef IRECV_BATCH
#endif

#ifdef DEBUG
#  include <iostream>
#endif // DEBUG

namespace boost { namespace graph { namespace distributed {

struct mpi_process_group::deallocate_block
{
  explicit deallocate_block(blocks_type* blocks) : blocks(blocks) { }

  void operator()(int* block_num)
  {
    block_type* block = (*blocks)[*block_num];

    // Mark this block as inactive
    (*blocks)[*block_num] = 0;

#ifdef DEBUG
    fprintf(stderr, "Processor %i deallocated block #%i\n", 
            boost::mpi::communicator().rank(), *block_num);
#endif

    // Delete the block and its block number
    delete block_num;
    delete block;
  }

private:
  blocks_type* blocks;
};

mpi_process_group::impl::incoming_messages::incoming_messages()
{
  next_header.push_back(headers.begin());
}

mpi_process_group::impl::impl(std::size_t num_headers, std::size_t buffer_sz, 
                              communicator_type parent_comm)
  : comm(parent_comm, boost::mpi::comm_duplicate),
    oob_reply_comm(parent_comm, boost::mpi::comm_duplicate),
    allocated_tags(boost::mpi::environment::max_tag())
{
  max_sent=0;
  max_received=0;
  int n = comm.size();
  outgoing.resize(n);
  incoming.resize(n);
  
  // no synchronization stage means -1
  // to keep the convention
  synchronizing_stage.resize(n,-1); 
  number_sent_batches.resize(n);
  number_received_batches.resize(n);
  trigger_context = trc_none;
  processing_batches = 0;
  
  // Allocator a placeholder block "0"
  blocks.push_back(new block_type);

  synchronizing = false;

  set_batch_size(num_headers,buffer_sz);
  
  for (int i = 0; i < n; ++i) {
    incoming[i].next_header.front() = incoming[i].headers.end();
    outgoing[i].buffer.reserve(batch_message_size);
  }

#ifdef PREALLOCATE_BATCHES
  batch_pool.resize(PREALLOCATE_BATCHES);
  for (std::size_t i = 0 ; i < batch_pool.size(); ++i) {
    batch_pool[i].buffer.reserve(batch_message_size);
    batch_pool[i].request=MPI_REQUEST_NULL;
    free_batches.push(i);
  }  
#endif
}

void mpi_process_group::impl::set_batch_size(std::size_t header_num, std::size_t buffer_sz)
{
  batch_header_number = header_num;
  batch_buffer_size = buffer_sz;

  // determine batch message size by serializing the largest possible batch
  outgoing_messages msg;
  msg.headers.resize(batch_header_number);
  msg.buffer.resize(batch_buffer_size);
  boost::mpi::packed_oarchive oa(comm);
  oa << const_cast<const outgoing_messages&>(msg);
  batch_message_size = oa.size();
}


mpi_process_group::impl::~impl()
{
  // Delete the placeholder "0" block
  delete blocks.front();
  if (!boost::mpi::environment::finalized()) 
  for (std::vector<MPI_Request>::iterator it=requests.begin(); 
       it != requests.end();++it)
    MPI_Cancel(&(*it));
}

namespace detail {
// global batch handlers
void handle_batch (mpi_process_group const& self, int source, int, 
    mpi_process_group::outgoing_messages& batch,bool out_of_band)
{
#ifdef DEBUG
  std::cerr << "Processing batch trigger\n";
  std::cerr << "BATCH: " << process_id(self) << " <- " << source << " ("
            << batch.headers.size() << " headers, "
            << batch.buffer.size() << " bytes)"
            << std::endl;
#endif
  // If we are not synchronizing, then this must be an early receive
  trigger_receive_context old_context = self.impl_->trigger_context;
  if (self.impl_->trigger_context != trc_in_synchronization)
    self.impl_->trigger_context = trc_early_receive;

  // Receive the batched messages
  self.receive_batch(source,batch);

  // Restore the previous context
  self.impl_->trigger_context = old_context;
}

// synchronization handler
void handle_sync (mpi_process_group const& self, int source, int tag, int val,
                  bool)
{
  // increment the stage for the source
  std::size_t stage = static_cast<std::size_t>(
    ++self.impl_->synchronizing_stage[source]);

  BOOST_ASSERT(source != process_id(self));

#ifdef DEBUG
  std::ostringstream out;
  out << process_id(self) << ": handle_sync from " << source 
      << " (stage = " << self.impl_->synchronizing_stage[source] << ")\n";
  std::cerr << out.str();
#endif

  // record how many still have messages to be sent
  if (self.impl_->synchronizing_unfinished.size()<=stage) {
    BOOST_ASSERT(self.impl_->synchronizing_unfinished.size() == stage);
    self.impl_->synchronizing_unfinished.push_back(val >= 0 ? 1 : 0);
  }
  else
    self.impl_->synchronizing_unfinished[stage]+=(val >= 0 ? 1 : 0);

  // record how many are in that stage
  if (self.impl_->processors_synchronizing_stage.size()<=stage) {
    BOOST_ASSERT(self.impl_->processors_synchronizing_stage.size() == stage);
    self.impl_->processors_synchronizing_stage.push_back(1);
  }
  else
    ++self.impl_->processors_synchronizing_stage[stage];
    
  // subtract how many batches we were supposed to receive
  if (val>0)
    self.impl_->number_received_batches[source] -= val;
}


}

mpi_process_group::mpi_process_group(communicator_type parent_comm)
{
  // 64K messages and 1MB buffer turned out to be a reasonable choice
  impl_.reset(new impl(64*1024,1024*1024,parent_comm));
#ifndef IRECV_BATCH
  global_trigger<outgoing_messages>(msg_batch,&detail::handle_batch);
#else // use irecv version by providing a maximum buffer size
  global_trigger<outgoing_messages>(msg_batch,&detail::handle_batch,
         impl_->batch_message_size);
#endif
  global_trigger<outgoing_messages>(msg_large_batch,&detail::handle_batch);
  global_trigger<int>(msg_synchronizing,&detail::handle_sync);
  rank = impl_->comm.rank();
  size = impl_->comm.size();
  
#ifdef SEND_OOB_BSEND
  // let us try with a default bufferr size of 16 MB
  if (!message_buffer_size())
    set_message_buffer_size(16*1024*1024);
#endif
}

mpi_process_group::mpi_process_group(std::size_t h, std::size_t sz, 
                                     communicator_type parent_comm)
{
  impl_.reset(new impl(h,sz,parent_comm));
#ifndef IRECV_BATCH
  global_trigger<outgoing_messages>(msg_batch,&detail::handle_batch);
#else // use irecv version by providing a maximum buffer size
  global_trigger<outgoing_messages>(msg_batch,&detail::handle_batch,
         impl_->batch_message_size);
#endif
  global_trigger<outgoing_messages>(msg_large_batch,&detail::handle_batch);
  global_trigger<int>(msg_synchronizing,&detail::handle_sync);
  rank = impl_->comm.rank();
  size = impl_->comm.size();
#ifdef SEND_OOB_BSEND
  // let us try with a default bufferr size of 16 MB
  if (!message_buffer_size())
    set_message_buffer_size(16*1024*1024);
#endif
}

mpi_process_group::mpi_process_group(const mpi_process_group& other,
                                     const receiver_type& handler, bool)
  : impl_(other.impl_)
{ 
  rank = impl_->comm.rank();
  size = impl_->comm.size();
  replace_handler(handler); 
}

mpi_process_group::mpi_process_group(const mpi_process_group& other,
                                     attach_distributed_object, bool)
  : impl_(other.impl_)
{ 
  rank = impl_->comm.rank();
  size = impl_->comm.size();
  allocate_block();

  for (std::size_t i = 0; i < impl_->incoming.size(); ++i) {
    if (my_block_number() >= (int)impl_->incoming[i].next_header.size()) {
      impl_->incoming[i].next_header
        .push_back(impl_->incoming[i].headers.begin());
    } else {
      impl_->incoming[i].next_header[my_block_number()] =
        impl_->incoming[i].headers.begin();
    }

#ifdef DEBUG
    if (process_id(*this) == 0) {
      std::cerr << "Allocated tag block " << my_block_number() << std::endl;
    }
#endif
  }
}

mpi_process_group::~mpi_process_group()  {
  /*
  std::string msg = boost::lexical_cast<std::string>(process_id(*this)) + " " +
            boost::lexical_cast<std::string>(impl_->max_received) + " " +
            boost::lexical_cast<std::string>(impl_->max_sent) + "\n";
     std::cerr << msg << "\n";
     */
}


mpi_process_group::communicator_type communicator(const mpi_process_group& pg)
{ return pg.impl_->comm; }

void
mpi_process_group::replace_handler(const receiver_type& handler,
                                   bool out_of_band_receive)
{
  make_distributed_object();

  // Attach the receive handler
  impl_->blocks[my_block_number()]->on_receive = handler;
}

void
mpi_process_group::make_distributed_object()
{
  if (my_block_number() == 0) {
    allocate_block();

    for (std::size_t i = 0; i < impl_->incoming.size(); ++i) {
      if (my_block_number() >= (int)impl_->incoming[i].next_header.size()) {
        impl_->incoming[i].next_header
          .push_back(impl_->incoming[i].headers.begin());
      } else {
        impl_->incoming[i].next_header[my_block_number()] =
          impl_->incoming[i].headers.begin();
      }

#ifdef DEBUG
      if (process_id(*this) == 0) {
        std::cerr << "Allocated tag block " << my_block_number() << std::endl;
      }
#endif
    }
  } else {
    // Clear out the existing triggers
    std::vector<shared_ptr<trigger_base> >()
      .swap(impl_->blocks[my_block_number()]->triggers); 
  }

  // Clear out the receive handler
  impl_->blocks[my_block_number()]->on_receive = 0;
}

void
mpi_process_group::
replace_on_synchronize_handler(const on_synchronize_event_type& handler)
{
  if (my_block_number() > 0)
    impl_->blocks[my_block_number()]->on_synchronize = handler;
}

int mpi_process_group::allocate_block(bool out_of_band_receive)
{
  BOOST_ASSERT(!block_num);
  block_iterator i = impl_->blocks.begin();
  while (i != impl_->blocks.end() && *i) ++i;

  if (i == impl_->blocks.end()) {
    impl_->blocks.push_back(new block_type());
    i = impl_->blocks.end() - 1;
  } else {
    *i = new block_type();
  }

  block_num.reset(new int(i - impl_->blocks.begin()),
                  deallocate_block(&impl_->blocks));

#ifdef DEBUG
  fprintf(stderr,
          "Processor %i allocated block #%i\n", process_id(*this), *block_num);
#endif

  return *block_num;
}

bool mpi_process_group::maybe_emit_receive(int process, int encoded_tag) const
{
  std::pair<int, int> decoded = decode_tag(encoded_tag);

  BOOST_ASSERT (decoded.first < static_cast<int>(impl_->blocks.size()));

  block_type* block = impl_->blocks[decoded.first];
  if (!block) {
    std::cerr << "Received message from process " << process << " with tag "
              << decoded.second << " for non-active block "
              << decoded.first << std::endl;
    std::cerr << "Active blocks are: ";
    for (std::size_t i = 0; i < impl_->blocks.size(); ++i)
      if (impl_->blocks[i])
        std::cerr << i << ' ';
    std::cerr << std::endl;
    BOOST_ASSERT(block);
  }
  
  if (decoded.second < static_cast<int>(block->triggers.size())
      && block->triggers[decoded.second]) {
    // We have a trigger for this message; use it
    trigger_receive_context old_context = impl_->trigger_context;
    impl_->trigger_context = trc_out_of_band;
    block->triggers[decoded.second]->receive(*this, process, decoded.second, 
                                             impl_->trigger_context, 
                                             decoded.first);
    impl_->trigger_context = old_context;
  }
  else
    return false;
  // We receives the message above
  return true;
}

bool mpi_process_group::emit_receive(int process, int encoded_tag) const
{
  std::pair<int, int> decoded = decode_tag(encoded_tag);

  if (decoded.first >= static_cast<int>(impl_->blocks.size()))
    // This must be an out-of-band message destined for
    // send_oob_with_reply; ignore it.
    return false;

  // Find the block that will receive this message
  block_type* block = impl_->blocks[decoded.first];
  BOOST_ASSERT(block);
  if (decoded.second < static_cast<int>(block->triggers.size())
      && block->triggers[decoded.second])
    // We have a trigger for this message; use it
    block->triggers[decoded.second]->receive(*this,process, decoded.second, 
                                             impl_->trigger_context);
  else if (block->on_receive)
    // Fall back to the normal message handler
    block->on_receive(process, decoded.second);
  else
    // There was no handler for this message
    return false;

  // The message was handled above
  return true;
}

void mpi_process_group::emit_on_synchronize() const
{
  for (block_iterator i = impl_->blocks.begin(); i != impl_->blocks.end(); ++i)
    if (*i && (*i)->on_synchronize) (*i)->on_synchronize();
}


optional<std::pair<mpi_process_group::process_id_type, int> >
mpi_process_group::probe() const
{
#ifdef DEBUG
  std::cerr << "PROBE: " << process_id(*this) << ", tag block = "
            << my_block_number() << std::endl;
#endif

  typedef std::pair<process_id_type, int> result_type;

  int tag_block = my_block_number();

  for (std::size_t source = 0; source < impl_->incoming.size(); ++source) {
    impl::incoming_messages& incoming = impl_->incoming[source];
    std::vector<impl::message_header>::iterator& i =
      incoming.next_header[tag_block];
    std::vector<impl::message_header>::iterator end =  incoming.headers.end();

    while (i != end) {
      if (i->tag != -1 && decode_tag(i->tag).first == my_block_number()) {
#ifdef DEBUG
        std::cerr << "PROBE: " << process_id(*this) << " <- " << source
                  << ", block = " << my_block_number() << ", tag = "
                  << decode_tag(i->tag).second << ", bytes = " << i->bytes
                  << std::endl;
#endif
        return result_type(source, decode_tag(i->tag).second);
      }
      ++i;
    }
  }
  return optional<result_type>();
}

void
mpi_process_group::maybe_send_batch(process_id_type dest) const
{
#ifndef NO_SPLIT_BATCHES  
  impl::outgoing_messages& outgoing = impl_->outgoing[dest];
  if (outgoing.buffer.size() >= impl_->batch_buffer_size ||
      outgoing.headers.size() >= impl_->batch_header_number) {
    // we are full and need to send
    outgoing_messages batch;
    batch.buffer.reserve(impl_->batch_buffer_size);
    batch.swap(outgoing);
    if (batch.buffer.size() >= impl_->batch_buffer_size 
         && batch.headers.size()>1 ) {
      // we are too large, keep the last message in the outgoing buffer
      std::copy(batch.buffer.begin()+batch.headers.back().offset,
                batch.buffer.end(),std::back_inserter(outgoing.buffer));
      batch.buffer.resize(batch.headers.back().offset);
      outgoing.headers.push_back(batch.headers.back());
      batch.headers.pop_back();
      outgoing.headers.front().offset=0;
    }
    send_batch(dest,batch);
  }
#endif
}

void
mpi_process_group::send_batch(process_id_type dest) const
{
  impl::outgoing_messages& outgoing = impl_->outgoing[dest];
  if (outgoing.headers.size()) {
    // need to copy to avoid race conditions
    outgoing_messages batch;
    batch.buffer.reserve(impl_->batch_buffer_size);
    batch.swap(outgoing); 
    send_batch(dest,batch);
  }
}


void
mpi_process_group::send_batch(process_id_type dest, 
                              outgoing_messages& outgoing) const
{
  impl_->free_sent_batches();
  process_id_type id = process_id(*this);

  // clear the batch
#ifdef DEBUG
  std::cerr << "Sending batch: " << id << " -> "  << dest << std::endl;
#endif
  // we increment the number of batches sent
  ++impl_->number_sent_batches[dest];
  // and send the batch
  BOOST_ASSERT(outgoing.headers.size() <= impl_->batch_header_number);
  if (id != dest) {
#ifdef NO_ISEND_BATCHES
    impl::batch_request req;
#else
#ifdef PREALLOCATE_BATCHES
    while (impl_->free_batches.empty()) {
      impl_->free_sent_batches();
      poll();
    }
    impl::batch_request& req = impl_->batch_pool[impl_->free_batches.top()];
    impl_->free_batches.pop();
#else
    impl_->sent_batches.push_back(impl::batch_request());
    impl::batch_request& req = impl_->sent_batches.back();
#endif
#endif
    boost::mpi::packed_oarchive oa(impl_->comm,req.buffer);
    oa << outgoing;

    int tag = msg_batch;
    
#ifdef IRECV_BATCH
    if (oa.size() > impl_->batch_message_size)
      tag = msg_large_batch;
#endif

#ifndef NDEBUG // Prevent uninitialized variable warning with NDEBUG is on
    int result =
#endif // !NDEBUG
      MPI_Isend(const_cast<void*>(oa.address()), oa.size(),
                MPI_PACKED, dest, tag, impl_->comm,
                &req.request);
    BOOST_ASSERT(result == MPI_SUCCESS);
    impl_->max_sent = (std::max)(impl_->max_sent,impl_->sent_batches.size());
#ifdef NO_ISEND_BATCHES
    int done=0;
    do {                                                        
        poll();                                                
        MPI_Test(&req.request,&done,MPI_STATUS_IGNORE);               
       } while (!done);                                            
#else
#ifdef MAX_BATCHES
    while (impl_->sent_batches.size() >= MAX_BATCHES-1) {
      impl_->free_sent_batches();
      poll();
    }
#endif
#endif
  }
  else
    receive_batch(id,outgoing);
}

void mpi_process_group::process_batch(int source) const
{
  bool processing_from_queue = !impl_->new_batches.empty();
  impl_->processing_batches++;
  typedef std::vector<impl::message_header>::iterator iterator;

  impl::incoming_messages& incoming = impl_->incoming[source];
  
  // Set up the iterators pointing to the next header in each block
  for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
        incoming.next_header[i] = incoming.headers.begin();

  buffer_type remaining_buffer;
  std::vector<impl::message_header> remaining_headers;

  iterator end = incoming.headers.end();

  for (iterator i = incoming.headers.begin(); i != end; ++i) {
    // This this message has already been received, skip it
    if (i->tag == -1)
      continue;

#ifdef BATCH_DEBUG
    std::cerr << process_id(*this) << ": emit_receive(" << source << ", "
              << decode_tag(i->tag).first << ":" << decode_tag(i->tag).second
              << ")\n";
#endif

    if (!emit_receive(source, i->tag)) {
#ifdef BATCH_DEBUG
      std::cerr << process_id(*this) << ": keeping message # " 
            << remaining_headers.size() << " from " << source << " ("
            << decode_tag(i->tag).first << ":" 
            << decode_tag(i->tag).second << ", " 
            << i->bytes << " bytes)\n";
#endif
      // Hold on to this message until the next stage
      remaining_headers.push_back(*i);
      remaining_headers.back().offset = remaining_buffer.size();
      remaining_buffer.insert(remaining_buffer.end(),
                  &incoming.buffer[i->offset],
                  &incoming.buffer[i->offset] + i->bytes);
    }
  }

  // Swap the remaining messages into the "incoming" set.
  incoming.headers.swap(remaining_headers);
  incoming.buffer.swap(remaining_buffer);

  // Set up the iterators pointing to the next header in each block
  for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
    incoming.next_header[i] = incoming.headers.begin();
  impl_->processing_batches--;
  
  if (!processing_from_queue)
    while (!impl_->new_batches.empty()) {
      receive_batch(impl_->new_batches.front().first,
                    impl_->new_batches.front().second);
      impl_->new_batches.pop();
    }
}


void mpi_process_group::receive_batch(process_id_type source, 
  outgoing_messages& new_messages) const
{
  impl_->free_sent_batches();
  if(!impl_->processing_batches) {
    // increase the number of received batches
    ++impl_->number_received_batches[source];
    // and receive the batch
    impl::incoming_messages& incoming = impl_->incoming[source];
    typedef std::vector<impl::message_header>::iterator iterator;
    iterator end = new_messages.headers.end();
    for (iterator i = new_messages.headers.begin(); i != end; ++i) {
      incoming.headers.push_back(*i);
      incoming.headers.back().offset = incoming.buffer.size();
      incoming.buffer.insert(incoming.buffer.end(),
                  &new_messages.buffer[i->offset],
                  &new_messages.buffer[i->offset] + i->bytes);
    }
    // Set up the iterators pointing to the next header in each block
    for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
      incoming.next_header[i] = incoming.headers.begin();
#ifndef NO_IMMEDIATE_PROCESSING
    process_batch(source);
#endif
  }
  else {
  #ifdef DEBUG
    std::cerr << "Pushing incoming message batch onto queue since we are already processing a batch.\n";
  #endif
    // use swap to avoid copying
    impl_->new_batches.push(std::make_pair(int(source),outgoing_messages()));
    impl_->new_batches.back().second.swap(new_messages);
    impl_->max_received = (std::max)(impl_->max_received,impl_->new_batches.size());
  }
}


void mpi_process_group::pack_headers() const 
{
  for (process_id_type other = 0; other < num_processes(*this); ++other) {
    typedef std::vector<impl::message_header>::iterator iterator;

    impl::incoming_messages& incoming = impl_->incoming[other];

    buffer_type remaining_buffer;
    std::vector<impl::message_header> remaining_headers;

    iterator end = incoming.headers.end();
    for (iterator i = incoming.headers.begin(); i != end; ++i) {
      if (i->tag == -1)
        continue;

      // Hold on to this message until the next stage
      remaining_headers.push_back(*i);
      remaining_headers.back().offset = remaining_buffer.size();
      remaining_buffer.insert(remaining_buffer.end(),
                              &incoming.buffer[i->offset],
                              &incoming.buffer[i->offset] + i->bytes);
    }
    
    // Swap the remaining messages into the "incoming" set.
    incoming.headers.swap(remaining_headers);
    incoming.buffer.swap(remaining_buffer);

    // Set up the iterators pointing to the next header in each block
    for (std::size_t i = 0; i < incoming.next_header.size(); ++i)
      incoming.next_header[i] = incoming.headers.begin();
  }
}

void mpi_process_group::receive_batch(boost::mpi::status& status) const
{
  //std::cerr << "Handling batch\n";
  outgoing_messages batch;
  //impl_->comm.recv(status.source(),status.tag(),batch);

  //receive_oob(*this,status.source(),msg_batch,batch);
    
  // Determine how big the receive buffer should be
#if BOOST_VERSION >= 103600
  int size = status.count<boost::mpi::packed>().get();
#else
  int size;
  MPI_Status mpi_status(status);
  MPI_Get_count(&mpi_status, MPI_PACKED, &size);
#endif

  // Allocate the receive buffer
  boost::mpi::packed_iarchive in(impl_->comm,size);
  
  // Receive the message data
  MPI_Recv(in.address(), size, MPI_PACKED,
           status.source(), status.tag(), 
           impl_->comm, MPI_STATUS_IGNORE);

  // Unpack the message data
  in >> batch;
  receive_batch(status.source(),batch);
}

std::pair<boost::mpi::communicator, int> 
mpi_process_group::actual_communicator_and_tag(int tag, int block) const
{
  if (tag >= max_tags * static_cast<int>(impl_->blocks.size()))
    // Use the out-of-band reply communicator
    return std::make_pair(impl_->oob_reply_comm, tag);
  else
    // Use the normal communicator and translate the tag
    return std::make_pair(impl_->comm, 
                          encode_tag(block == -1? my_block_number() : block,
                                     tag));
}


void mpi_process_group::synchronize() const
{
  // Don't synchronize if we've already finished
  if (boost::mpi::environment::finalized()) 
    return;

#ifdef DEBUG
  std::cerr << "SYNC: " << process_id(*this) << std::endl;
#endif

  emit_on_synchronize();

  process_id_type id = process_id(*this);     // Our rank
  process_size_type p = num_processes(*this); // The number of processes

  // Pack the remaining incoming messages into the beginning of the
  // buffers, so that we can receive new messages in this
  // synchronization step without losing those messages that have not
  // yet been received.
  pack_headers();

  impl_->synchronizing_stage[id] = -1;
  int stage=-1;
  bool no_new_messages = false;
  while (true) {
      ++stage;
#ifdef DEBUG
      std::cerr << "SYNC: " << id << " starting stage " << (stage+1) << ".\n";
#endif

      // Tell everyone that we are synchronizing. Note: we use MPI_Isend since 
      // we absolutely cannot have any of these operations blocking.
      
      // increment the stage for the source
       ++impl_->synchronizing_stage[id];
       if (impl_->synchronizing_stage[id] != stage)
         std::cerr << "Expected stage " << stage << ", got " << impl_->synchronizing_stage[id] << std::endl;
       BOOST_ASSERT(impl_->synchronizing_stage[id]==stage);
      // record how many still have messages to be sent
      if (static_cast<int>(impl_->synchronizing_unfinished.size())<=stage) {
        BOOST_ASSERT(static_cast<int>(impl_->synchronizing_unfinished.size()) == stage);
        impl_->synchronizing_unfinished.push_back(no_new_messages ? 0 : 1);
      }
      else
        impl_->synchronizing_unfinished[stage]+=(no_new_messages ? 0 : 1);

      // record how many are in that stage
      if (static_cast<int>(impl_->processors_synchronizing_stage.size())<=stage) {
        BOOST_ASSERT(static_cast<int>(impl_->processors_synchronizing_stage.size()) == stage);
        impl_->processors_synchronizing_stage.push_back(1);
      }
      else
        ++impl_->processors_synchronizing_stage[stage];

      impl_->synchronizing = true;

      for (int dest = 0; dest < p; ++dest) {
        int sync_message = no_new_messages ? -1 : impl_->number_sent_batches[dest];
        if (dest != id) {
          impl_->number_sent_batches[dest]=0;       
          MPI_Request request;
          MPI_Isend(&sync_message, 1, MPI_INT, dest, msg_synchronizing, impl_->comm,&request);
          int done=0;
          do {
            poll();
            MPI_Test(&request,&done,MPI_STATUS_IGNORE);
          } while (!done);
        }
        else { // need to subtract how many messages I should have received
          impl_->number_received_batches[id] -=impl_->number_sent_batches[id];
          impl_->number_sent_batches[id]=0;
        }
      }

      // Keep handling out-of-band messages until everyone has gotten
      // to this point.
      while (impl_->processors_synchronizing_stage[stage] <p) {
        // with the trigger based solution we cannot easily pass true here 
        poll(/*wait=*/false, -1, true);

      }

      // check that everyone is at least here
      for (int source=0; source<p ; ++source)
        BOOST_ASSERT(impl_->synchronizing_stage[source] >= stage);

      // receive any batches sent in the meantime
      // all have to be available already
      while (true) {
        bool done=true;
        for (int source=0; source<p ; ++source)
          if(impl_->number_received_batches[source] < 0)
            done = false;
        if (done)
          break;
        poll(false,-1,true);
      }
      
#ifndef NO_IMMEDIATE_PROCESSING
      for (int source=0; source<p ; ++source)
        BOOST_ASSERT(impl_->number_received_batches[source] >= 0);
#endif

      impl_->synchronizing = false;
      
      // Flush out remaining messages
      if (impl_->synchronizing_unfinished[stage]==0)
        break;
#ifdef NO_IMMEDIATE_PROCESSING
      for (process_id_type dest = 0; dest < p; ++dest)
        process_batch(dest);
#endif

      no_new_messages = true;
      for (process_id_type dest = 0; dest < p; ++dest) {
        if (impl_->outgoing[dest].headers.size() || 
            impl_->number_sent_batches[dest]>0)
          no_new_messages = false;
        send_batch(dest);
      }
    }

  impl_->comm.barrier/*nomacro*/();
#if 0
  // set up for next synchronize call
  for (int source=0; source<p; ++source) {
    if (impl_->synchronizing_stage[source] != stage) {
      std::cerr << id << ": expecting stage " << stage << " from source "
                << source << ", got " << impl_->synchronizing_stage[source]
                << std::endl;
    }
    BOOST_ASSERT(impl_->synchronizing_stage[source]==stage);
  }
#endif
  std::fill(impl_->synchronizing_stage.begin(),
            impl_->synchronizing_stage.end(), -1);
            
  // get rid of the information regarding recorded numbers of processors
  // for the stages we just finished
  impl_->processors_synchronizing_stage.clear();
  impl_->synchronizing_unfinished.clear();

  for (process_id_type dest = 0; dest < p; ++dest)
    BOOST_ASSERT (impl_->outgoing[dest].headers.empty());
#ifndef NO_IMMEDIATE_PROCESSING
      for (int source=0; source<p ; ++source)
        BOOST_ASSERT (impl_->number_received_batches[source] == 0);
#endif

  impl_->free_sent_batches();
#ifdef DEBUG
  std::cerr << "SYNC: " << process_id(*this) << " completed." << std::endl;
#endif
}

optional<std::pair<mpi_process_group::process_id_type, int> >
probe(const mpi_process_group& pg)
{ return pg.probe(); }

void mpi_process_group::poll_requests(int block) const
{
  int size = impl_->requests.size();
  if (size==0)
    return;
  std::vector<MPI_Status> statuses(size);
  std::vector<int> indices(size);
  
  while (true) {
    MPI_Testsome(impl_->requests.size(),&impl_->requests[0],
       &size,&indices[0],&statuses[0]);
    if (size==0)
      return; // no message waiting

    // remove handled requests before we get the chance to be recursively called
    if (size) {
      std::vector<MPI_Request> active_requests;
      std::size_t i=0;
      int j=0;
      for (;i< impl_->requests.size() && j< size; ++i) {
        if (int(i)==indices[j])
          // release the dealt-with request 
          ++j;
        else // copy and keep the request
          active_requests.push_back(impl_->requests[i]);
      }    
      while (i < impl_->requests.size())
        active_requests.push_back(impl_->requests[i++]);
      impl_->requests.swap(active_requests);
    }

    optional<std::pair<int, int> > result;
    for (int i=0;i < size; ++i) {
      std::pair<int, int> decoded = decode_tag(statuses[i].MPI_TAG);
      block_type* block = impl_->blocks[decoded.first];
      
      BOOST_ASSERT (decoded.second < static_cast<int>(block->triggers.size()) && block->triggers[decoded.second]);
        // We have a trigger for this message; use it
      trigger_receive_context old_context = impl_->trigger_context;
      impl_->trigger_context = trc_irecv_out_of_band;
      block->triggers[decoded.second]->receive(*this, statuses[i].MPI_SOURCE, 
            decoded.second, impl_->trigger_context, decoded.first);
      impl_->trigger_context = old_context;
    }
  }
}


optional<std::pair<int, int> > 
mpi_process_group::
poll(bool wait, int block, bool synchronizing) const
{
  // Set the new trigger context for these receive operations
  trigger_receive_context old_context = impl_->trigger_context;
  if (synchronizing)
    impl_->trigger_context = trc_in_synchronization;
  else
    impl_->trigger_context = trc_out_of_band;

  //wait = false;
  optional<boost::mpi::status> status;
  bool finished=false;
  optional<std::pair<int, int> > result;
  do {
    poll_requests(block);
    // Check for any messages not yet received.
#ifdef PBGL_PROCESS_GROUP_NO_IRECV
    if (wait)
      status = impl_->comm.probe();
    else 
#endif
       status = impl_->comm.iprobe();

    if (status) { // we have a message
      // Decode the message
      std::pair<int, int> decoded = decode_tag(status.get().tag());
      if (maybe_emit_receive(status.get().source(), status.get().tag()))
        // We received the message out-of-band; poll again
        finished = false;
      else if (decoded.first == (block == -1 ? my_block_number() : block)) {
        // This message is for us, but not through a trigger. Return
        // the decoded message.
        result = std::make_pair(status.get().source(), decoded.second);
      // otherwise we didn't match any message we know how to deal with, so
      // pretend no message exists.
        finished = true;
      }
    }
    else
      // We don't have a message to process; we're done.
      finished=!wait;
  } while (!finished);

  // Restore the prior trigger context
  impl_->trigger_context = old_context;
  poll_requests(block);
  return result;
}

void synchronize(const mpi_process_group& pg) { pg.synchronize(); }

mpi_process_group mpi_process_group::base() const
{
  mpi_process_group copy(*this);
  copy.block_num.reset();
  return copy;
}
  

void mpi_process_group::impl::free_sent_batches()
{
  typedef std::list<batch_request>::iterator iterator;
  iterator it = sent_batches.begin();
  int flag;
  int result;
  while(it != sent_batches.end()) {
    result = MPI_Test(&it->request,&flag,MPI_STATUS_IGNORE);
    BOOST_ASSERT(result == MPI_SUCCESS);
    iterator next=it;
    ++next;
    if (flag)
      sent_batches.erase(it);
    it=next;
  }  
#ifdef PREALLOCATE_BATCHES
  for (std::size_t i=0; i< batch_pool.size();++i) {
    if(batch_pool[i].request != MPI_REQUEST_NULL) {
      result = MPI_Test(&batch_pool[i].request,&flag,MPI_STATUS_IGNORE);
      BOOST_ASSERT(result == MPI_SUCCESS);
      if (flag) {
        free_batches.push(i);
        batch_pool[i].request = MPI_REQUEST_NULL;
        batch_pool[i].buffer.resize(0);
      }
    }
  } 
#endif
}

void 
mpi_process_group::install_trigger(int tag, int block, 
      shared_ptr<trigger_base> const& launcher)
{
  block_type* my_block = impl_->blocks[block];
  BOOST_ASSERT(my_block);

  // Make sure we have enough space in the structure for this trigger.
  if (launcher->tag() >= static_cast<int>(my_block->triggers.size()))
    my_block->triggers.resize(launcher->tag() + 1);

  // If someone already put a trigger in this spot, we have a big
  // problem.
  if (my_block->triggers[launcher->tag()]) {
    std::cerr << "Block " << my_block_number() 
              << " already has a trigger for tag " << launcher->tag()
              << std::endl;
  }
  BOOST_ASSERT(!my_block->triggers[launcher->tag()]);

  // Attach a new trigger launcher
  my_block->triggers[launcher->tag()] = launcher;
}

std::size_t mpi_process_group::message_buffer_size()
{
  return message_buffer.size();
}


void mpi_process_group::set_message_buffer_size(std::size_t s)
{
  int sz;
  void* ptr;
  if (!message_buffer.empty()) {
    MPI_Buffer_detach(&ptr,&sz);
    BOOST_ASSERT(ptr == &message_buffer.front());
    BOOST_ASSERT(static_cast<std::size_t>(sz)  == message_buffer.size());
  }
  else if (old_buffer != 0)
    MPI_Buffer_detach(&old_buffer,&old_buffer_size);
  message_buffer.resize(s);
  if (s)
    MPI_Buffer_attach(&message_buffer.front(), message_buffer.size());
  else if (old_buffer_size)
    MPI_Buffer_attach(old_buffer, old_buffer_size);
}


std::vector<char> mpi_process_group::message_buffer;
int mpi_process_group::old_buffer_size=0;
void* mpi_process_group::old_buffer=0;

} } } // end namespace boost::graph::distributed

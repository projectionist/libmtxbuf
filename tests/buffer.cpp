#include <libmtxbuf/buffer.hpp>

#include <iostream>
#include <sstream>
#include <thread>

#include <unistd.h>
#include "tests.hpp"

using namespace std;

typedef struct work_context {
  mtxbuf::buffer<int> * buffer;
  int n;
  int progress;
} work_context;

void produce_n(work_context *ctx) {
  for(int i = 0; i < ctx->n; i++) {
    ctx->buffer->enqueue(i);
    ctx->progress = i + 1;
  }
}

void yield_and_sleep(int ms=80) {
  this_thread::yield();
  usleep(ms * 1024); // ms millisecond sleep
}

void full_buffer_blocks_producer() {
  size_t sz = 5;

  auto ctx = new work_context{
    .buffer = new mtxbuf::buffer<int>(sz),
    .n = 1,
    .progress = 0
  };

  step("fill the buffer");
  for(size_t i = 0; i < sz; i++) ctx->buffer->enqueue(i);

  step("create a producer thread");
  thread t(produce_n, ctx);
  t.detach();

  step("yield so that the producer thread can run");
  yield_and_sleep();

  step("check that the thread didn't get past queueing");
  if (ctx->progress != 0) {
    throw logic_error("full buffer did not block producer");
  }

  step("unblock the thread");
  for (int i = 0; i < ctx->n; i++) ctx->buffer->dequeue();

  delete ctx;
}

void consume_n(work_context *ctx) {
  for(int i = 0; i < ctx->n; i++) {
    ctx->buffer->dequeue();
    ctx->progress = i + 1;
  }
}

void empty_buffer_blocks_consumer() {
  size_t sz = 1;

  auto ctx = new work_context{
    .buffer = new mtxbuf::buffer<int>(sz),
    .n = 1,
    .progress = 0
  };

  step("create a consumer thread");
  thread t(consume_n, ctx);
  t.detach();

  step("yield so that the producer thread can run");
  yield_and_sleep();

  step("check that the thread didn't get past queueing");
  if (ctx->progress != 0) {
    throw logic_error("empty buffer did not block consumer");
  }

  step("unblock the thread");
  for (int i = 0; i < ctx->n; i++) ctx->buffer->enqueue(i);

  delete ctx;
}

void produce_and_consume() {
  size_t sz = 5;
  mtxbuf::buffer<size_t> buffer(sz);

  step("load data into the buffer");
  for(size_t i = 0; i < sz; i++) {
    buffer.enqueue(i);
  }

  step("check the buffer is fifo");
  for(size_t i = 0; i < sz; i++) {
    size_t e = buffer.dequeue();
    if (e != i) {
      ostringstream msg_builder;
      msg_builder << "got " << e << " from stream, expected " << i;
      throw logic_error(msg_builder.str());
    }
  }
}

void closed_buffer() {
  mtxbuf::buffer<int> buffer;

  step("write some data into the buffer");
  buffer.enqueue(1);
  buffer.enqueue(2);
  buffer.enqueue(3);

  step("close the buffer");
  buffer.close();

  step("write to a closed buffer");
  buffer.enqueue(4); // should be a noop

  step("check we can read data writted before the buffer was closed");
  if (buffer.dequeue() != 1) {
    throw logic_error("should have been able to read '1' from the buffer");
  }
  if (buffer.dequeue() != 2) {
    throw logic_error("should have been able to read '2' from the buffer");
  }
  if (buffer.dequeue() != 3) {
    throw logic_error("should have been able to read '3' from the buffer");
  }

  step("check that further reads result in an exception");
  bool exception_thrown = false;
  try {
    buffer.dequeue();
  } catch (mtxbuf::buffer_closed) {
    exception_thrown = true;
  }
  if (!exception_thrown) {
    throw logic_error("no exception thrown when reading from an empty closed buffer");
  }
}

void buffer_close_releases_blocked_consumer() {
  mtxbuf::buffer<int> buffer;

  step("launch a consumer on an empty buffer");
  bool exception_thrown = false;
  bool unblocked = false;
  std::thread consumer([&]{
    try {
      buffer.dequeue();
    } catch (mtxbuf::buffer_closed) {
      exception_thrown = true;
    }
    unblocked = true;
  });
  consumer.detach();

  step("check that when the buffer is closed, blocked consumers are released");
  buffer.close();
  yield_and_sleep();
  if(!unblocked) {
    throw logic_error("consumer was not unblocked by closing the buffer");
  }
  if(!exception_thrown) {
    throw logic_error("an exception was not thrown by the closed buffer");
  }


}

int main() {

  // check that producers are blocked when the buffer is full
  TEST(full_buffer_blocks_producer);

  // check that consumers are blocked when the buffer is empty
  TEST(empty_buffer_blocks_consumer);

  // check that each object that is produced can be consumed
  // and that non-empty reads and non-full writes do not block
  TEST(produce_and_consume);

  // check that when the buffer is closed remaining items can be read
  // but further reads result in an exception.
  // writing to a closed buffer is a noop. in particular, the items
  // produced are not consumable.
  TEST(closed_buffer);

  // check that if the buffer is closed while a consumer is blocked
  // waiting for data, the consumer becomes unblocked
  TEST(buffer_close_releases_blocked_consumer);

  return 0;
}

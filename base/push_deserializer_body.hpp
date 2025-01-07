#pragma once

#include "base/push_deserializer.hpp"

#include <algorithm>
#include <iomanip>
#include <memory>
#include <utility>

#include "base/sink_source.hpp"
#include "glog/logging.h"
#include "google/protobuf/io/coded_stream_inl.h"

namespace principia {
namespace base {
namespace _push_deserializer {
namespace internal {

using namespace principia::base::_sink_source;

inline DelegatingArrayInputStream::DelegatingArrayInputStream(
    std::function<Array<std::uint8_t>()> on_empty)
    : on_empty_(std::move(on_empty)),
      byte_count_(0),
      position_(0),
      last_returned_size_(0) {}

inline bool DelegatingArrayInputStream::Next(void const** const data,
                                             int* const size) {
  if (position_ == bytes_.size) {
    // We're at the end of the array.  Obtain a new one.
    bytes_ = on_empty_();
    position_ = 0;
    last_returned_size_ = 0;  // Don't let caller back up.
    if (bytes_.size == 0) {
      // At end of input data.
      return false;
    }
  }
  CHECK_LT(position_, bytes_.size);
  last_returned_size_ = bytes_.size - position_;
  *data = &bytes_.data[position_];
  *size = static_cast<int>(last_returned_size_);
  byte_count_ += last_returned_size_;
  position_ += last_returned_size_;
  return true;
}

inline void DelegatingArrayInputStream::BackUp(int const count) {
  CHECK_GT(last_returned_size_, 0)
      << "BackUp() can only be called after a successful Next().";
  CHECK_LE(count, last_returned_size_);
  CHECK_GE(count, 0);
  position_ -= count;
  byte_count_ -= count;
  last_returned_size_ = 0;  // Don't let caller back up.
}

inline bool DelegatingArrayInputStream::Skip(int const count) {
  CHECK_GE(count, 0);
  last_returned_size_ = 0;   // Don't let caller back up.
  std::int64_t remaining = count;
  while (remaining > bytes_.size - position_) {
    byte_count_ += bytes_.size - position_;
    remaining -= bytes_.size - position_;
    // We're at the end of the array.  Obtain a new one.
    bytes_ = on_empty_();
    position_ = 0;
    if (bytes_.size == 0) {
      // At end of input data.
      return false;
    }
  }
  byte_count_ += remaining;
  position_ += remaining;
  return true;
}

inline std::int64_t DelegatingArrayInputStream::ByteCount() const {
  return byte_count_;
}

inline std::ostream& operator<<(std::ostream& out,
                                DelegatingArrayInputStream const& stream) {
  out << "Stream with " << stream.ByteCount() << " total bytes, "
      << "current chunk of size " << stream.bytes_.size
      << " at position " << stream.position_
      << ", last call to Next returned " << stream.last_returned_size_
      << " bytes\n";
  for (std::int64_t index = 0; index < stream.bytes_.size; ++index) {
    out << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<std::int16_t>(stream.bytes_.data[index]);
    if (index == stream.position_) {
      out << "*";
    }
    if ((index + 1) % 32 == 0) {
      out << "  " << std::dec << index - 31 << "\n";
    } else {
      out << " ";
    }
  }
  if (stream.position_ == stream.bytes_.size) {
    out << "*";
  }
  out << "\n";
  return out;
}

inline PushDeserializer::PushDeserializer(
    int const chunk_size,
    int const number_of_chunks,
    std::unique_ptr<Compressor> compressor)
    : compressor_(std::move(compressor)),
      chunk_size_(chunk_size),
      compressed_chunk_size_(
          compressor_ == nullptr
              ? chunk_size_
              : compressor_->MaxCompressedLength(chunk_size_)),
      number_of_chunks_(number_of_chunks),
      uncompressed_data_(chunk_size_),
      stream_(std::bind(&PushDeserializer::Pull, this)) {
  // This sentinel ensures that the two queues are correctly out of step.
  done_.push(nullptr);
}

inline PushDeserializer::~PushDeserializer() {
  if (thread_ != nullptr) {
    thread_->join();
  }
}

inline void PushDeserializer::Start(
    not_null<std::unique_ptr<google::protobuf::Message>> message,
    std::function<void(google::protobuf::Message const&)> done) {
  owned_message_ = std::move(message);
  Start(owned_message_.get(), std::move(done));
}

inline void PushDeserializer::Start(
    not_null<google::protobuf::Message*> const message,
    std::function<void(google::protobuf::Message const&)> done) {
  CHECK(thread_ == nullptr);
  message_ = message;
  thread_ = std::make_unique<std::thread>([this, done = std::move(done)]() {
    // It is a well-known annoyance that, in order to set the total byte limit,
    // we have to copy code from MessageLite::ParseFromZeroCopyStream.  Blame
    // Kenton.
    google::protobuf::io::CodedInputStream decoder(&stream_);
    CHECK(message_->ParseFromCodedStream(&decoder));
    CHECK(decoder.ConsumedEntireMessage());

    // Run any remainining chunk callback.
    absl::MutexLock l(&lock_);
    CHECK_EQ(1, done_.size());
    auto const done_front = done_.front();
    if (done_front != nullptr) {
      done_front();
    }
    done_.pop();

    // Run the final callback.
    if (done != nullptr) {
      done(*message_);
    }
  });
}

inline void PushDeserializer::Push(Array<std::uint8_t> const bytes,
                                   std::function<void()> done) {
  // Slice the incoming data in chunks of size at most `chunk_size`.  Release
  // the lock after each chunk to give the deserializer a chance to run.  This
  // method should be called with `bytes` of size 0 to terminate the
  // deserialization, but it never generates a chunk of size 0 in other
  // circumstances.  The `done` callback is attached to the last chunk.
  Array<std::uint8_t> current = bytes;
  CHECK_LE(0, bytes.size);

  // Decide how much data we are going to push on the queue.  In the presence of
  // compression we have to respect the boundary of the incoming block.  In the
  // absence of compression we have a stream so we can cut into as many chunks
  // as we like.
  int queued_chunk_size;
  if (compressor_ == nullptr) {
    queued_chunk_size = chunk_size_;
  } else {
    CHECK_LE(bytes.size, compressed_chunk_size_);
    queued_chunk_size = compressed_chunk_size_;
  }

  bool is_last;
  do {
    {
      is_last = current.size <= queued_chunk_size;
      absl::MutexLock l(&lock_);

      auto const queue_has_room = [this]() {
        return queue_.size() < static_cast<std::size_t>(number_of_chunks_);
      };
      lock_.Await(absl::Condition(&queue_has_room));

      queue_.emplace(current.data,
                     std::min(current.size,
                              static_cast<std::int64_t>(queued_chunk_size)));
      done_.emplace(is_last ? std::move(done) : nullptr);
    }
    current.data = &current.data[queued_chunk_size];
    current.size -= queued_chunk_size;
  } while (!is_last);
}

inline void PushDeserializer::Push(UniqueArray<std::uint8_t> bytes) {
  Array<std::uint8_t> const unowned_bytes = bytes.get();
  bytes.data.release();
  Push(unowned_bytes,
       /*done=*/[b = unowned_bytes.data]() { delete[] b; });
}

inline Array<std::uint8_t> PushDeserializer::Pull() {
  Array<std::uint8_t> result;
  {
    absl::MutexLock l(&lock_);

    auto const queue_has_elements =  [this]() { return !queue_.empty(); };
    lock_.Await(absl::Condition(&queue_has_elements));

    // The front of `done_` is the callback for the `Array<std::uint8_t>` object
    // that was just processed.  Run it now.
    CHECK(!done_.empty());
    auto const done = done_.front();
    if (done != nullptr) {
      done();
    }
    done_.pop();
    // Get the next `Array<std::uint8_t>` object to process and remove it from
    // `queue_`.  Uncompress it if needed.
    auto const& front = queue_.front();
    if (front.size == 0 || compressor_ == nullptr) {
      result = front;
    } else {
      ArraySource<std::uint8_t> source(front);
      ArraySink<std::uint8_t> sink(uncompressed_data_.get());
      CHECK(compressor_->UncompressStream(&source, &sink));
      result = sink.array();
    }
    queue_.pop();
  }
  return result;
}

}  // namespace internal
}  // namespace _push_deserializer
}  // namespace base
}  // namespace principia

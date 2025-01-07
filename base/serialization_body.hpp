#pragma once

#include "base/serialization.hpp"

namespace principia {
namespace base {
namespace _serialization {
namespace internal {

UniqueArray<std::uint8_t> SerializeAsBytes(
    google::protobuf::MessageLite const& message) {
  UniqueArray<std::uint8_t> bytes(message.ByteSizeLong());
  message.SerializeToArray(bytes.data.get(), bytes.size);
  return bytes;
}

template<typename Message>
Message ParseFromBytes(Array<std::uint8_t const> bytes) {
  Message message;
  CHECK(message.ParseFromArray(bytes.data, bytes.size));
  return message;
}

}  // namespace internal
}  // namespace _serialization
}  // namespace base
}  // namespace principia

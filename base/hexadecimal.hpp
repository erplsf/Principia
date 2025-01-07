#pragma once

#include <cstdint>

#include "base/array.hpp"
#include "base/encoder.hpp"

namespace principia {
namespace base {
namespace _hexadecimal {
namespace internal {

using namespace principia::base::_array;
using namespace principia::base::_encoder;

template<bool null_terminated>
class HexadecimalEncoder : public Encoder<char, null_terminated> {
 public:
  // The result is upper-case.  Either `input.data <= &output.data[1]` or
  // `&output.data[input.size << 1] <= input.data` must hold, in particular,
  // `input.data == output.data` is valid.  `output.size` must be at least twice
  // `input.size`.  The range
  // [&output.data[input.size << 1], &output.data[output.size][ is left
  // unmodified.
  void Encode(Array<std::uint8_t const> input,
              Array<char> output) override;

  UniqueArray<char> Encode(Array<std::uint8_t const> input) override;

  std::int64_t EncodedLength(Array<std::uint8_t const> input) override;

  // Invalid digits are read as 0.  If `input.size` is odd, the last character
  // of the input is ignored.  Ignores case.  Either `output.data <=
  // &input.data[1]` or `&input.data[input.size & ~1] <= output.data` must hold,
  // in particular, `input.data == output.data` is valid.  `output.size` must be
  // at least `input.size / 2`.  The range
  // [&output[input.size / 2], &output[output.size][ is left unmodified.
  void Decode(Array<char const> input,
              Array<std::uint8_t> output) override;

  UniqueArray<std::uint8_t> Decode(Array<char const> input) override;

  std::int64_t DecodedLength(Array<char const> input) override;
};

}  // namespace internal

using internal::HexadecimalEncoder;

}  // namespace _hexadecimal
}  // namespace base
}  // namespace principia

#include "base/hexadecimal_body.hpp"

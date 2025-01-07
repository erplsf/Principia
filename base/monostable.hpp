#pragma once

namespace principia {
namespace base {
namespace _monostable {
namespace internal {

// A boolean which is constructed true and becomes false when `Flop` is called.
class Monostable final {
 public:
  Monostable() = default;

  Monostable(Monostable const&) = delete;
  Monostable(Monostable&&) = delete;
  Monostable& operator=(Monostable const&) = delete;
  Monostable& operator=(Monostable&&) = delete;

  void Flop();
  operator bool() const;

 private:
  bool transient_ = true;
};

}  // namespace internal

using internal::Monostable;

}  // namespace _monostable
}  // namespace base
}  // namespace principia

#include "base/monostable_body.hpp"

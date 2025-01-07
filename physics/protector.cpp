#include "physics/protector.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace principia {
namespace physics {
namespace _protector {
namespace internal {

bool Protector::RunWhenUnprotected(Instant const& t, Callback callback) {
  {
    absl::MutexLock l(&lock_);
    if (!protection_start_times_.empty() &&
        *protection_start_times_.begin() < t) {
      callbacks_.emplace(t, std::move(callback));
      return false;
    }
  }
  callback();
  return true;
}

void Protector::Protect(Instant const& t_min) {
  absl::MutexLock l(&lock_);
  protection_start_times_.insert(t_min);
}

void Protector::Unprotect(Instant const& t_min) {
  std::vector<Callback> callbacks_to_run;
  {
    absl::MutexLock l(&lock_);
    auto const it = protection_start_times_.find(t_min);
    CHECK(it != protection_start_times_.end());
    protection_start_times_.erase(it);

    // Find all the callbacks that are now unprotected and remove them from the
    // multimap.
    std::optional<Instant> const first_guard_start_time =
        protection_start_times_.empty()
            ? std::nullopt
            : std::make_optional(*protection_start_times_.begin());
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
      auto const& t = it->first;
      auto& callback = it->second;
      if (!first_guard_start_time || t <= *first_guard_start_time) {
        callbacks_to_run.emplace_back(std::move(callback));
        it = callbacks_.erase(it);
      } else {
        ++it;
      }
    }
  }

  // Run the callbacks without holding the lock.
  for (auto const& callback : callbacks_to_run) {
    callback();
  }
}

}  // namespace internal
}  // namespace _protector
}  // namespace physics
}  // namespace principia

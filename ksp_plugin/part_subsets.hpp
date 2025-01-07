#pragma once

#include <list>

#include "base/disjoint_sets.hpp"  // 🧙 For _disjoint_sets.
#include "geometry/instant.hpp"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/pile_up.hpp"
#include "physics/ephemeris.hpp"

// This ksp_plugin file is in namespace `base` to specialize templates declared
// therein.

namespace principia {

FORWARD_DECLARE(class, Part, FROM(ksp_plugin, part), INTO(base, disjoint_sets));

namespace base {
namespace _disjoint_sets {
namespace internal {

using namespace principia::geometry::_instant;
using namespace principia::ksp_plugin::_frames;
using namespace principia::ksp_plugin::_pile_up;
using namespace principia::physics::_ephemeris;

// Within an union-find on `Part`s, we maintain lists of the elements in the
// disjoint sets.  Moreover, we keep track of the inclusion relations of those
// sets to the sets of `Part`s in existing `PileUp`s, destroying existing
// `PileUp`s as we learn that they will not appear in the new arrangement.
// The `Collect` operation finalizes this, destroying existing `PileUp` which
// are strict supersets of the new sets, and creating the new `PileUp`s.
template<>
class Subset<Part>::Properties final {
  using PileUps = std::list<PileUp*>;

 public:
  // `*part` must outlive the constructed object.
  explicit Properties(not_null<Part*> part);

  // If `*this` and `other` are subsets of different `PileUp`s, or one is a
  // subset and not the other, the relevant `PileUp`s are erased.
  // Otherwise, `this->subset_of_existing_pile_up_` keeps track of the number of
  // missing parts.
  // Maintains `parts_` by joining the lists.
  void MergeWith(Properties& other);

  // “What’s this thing suddenly coming towards me very fast? Very very fast.
  // So big and flat and round, it needs a big wide sounding name like … ow …
  // ound … round … ground! That’s it! That’s a good name – ground!  I wonder if
  // it will be friends with me?”
  void Ground();
  bool grounded() const;

  // If `collected_`, performs no action.
  // Otherwise, sets `collected_`, and:
  // - if `EqualsExistingPileUp()`, performs no action.
  // - if `StrictSubsetOfExistingPileUp()`, erases the existing `PileUp` inserts
  //   a new `PileUp` into `pile_ups` with the parts in `parts_`.
  // - if `!subset_of_existing_pile_up_`, inserts a new `PileUp` into `pile_ups`
  //   with the parts in `parts_`.  The new `PileUp` is created using the given
  //   parameters.
  void Collect(
      PileUps& pile_ups,
      Instant const& t,
      Ephemeris<Barycentric>::AdaptiveStepParameters const&
          adaptive_step_parameters,
      Ephemeris<Barycentric>::FixedStepParameters const&
          fixed_step_parameters,
      not_null<Ephemeris<Barycentric>*> ephemeris);

 private:
  // Whether `left` and `right` are both subsets of the same existing `PileUp`.
  // Implies `left.SubsetOfExistingPileUp()` and
  // `right.SubsetOfExistingPileUp()`.
  static bool SubsetsOfSamePileUp(Properties const& left,
                                  Properties const& right);
  // Whether the set of `Part`s in `parts_` is equal to the set of `Part`s
  // in an existing `PileUp`.  Implies `SubsetOfExistingPileUp()`.
  bool EqualsExistingPileUp() const;
  // Whether the set of `Part`s in `parts_` is a subset of the set of
  // `Part`s in an existing `PileUp`.  In that case
  // `parts_.front()->containing_pile_up()` is that `PileUp`.
  bool SubsetOfExistingPileUp() const;
  // Whether the set of `Part`s in `parts_` is a strict subset of the set of
  // `Part`s in an existing `PileUp`.  Implies `SubsetOfExistingPileUp()`.
  bool StrictSubsetOfExistingPileUp() const;

  // Whether `Collect` has been called.
  bool collected_ = false;
  // if `SubsetOfExistingPileUp()`, `missing_` is the number of parts in that
  // `PileUp` that are not in this subset.
  int missing_;
  // The list of parts in this subset.
  std::list<not_null<Part*>> parts_;
  // Whether the subset touches the ground.
  bool grounded_ = false;
};

}  // namespace internal
}  // namespace _disjoint_sets
}  // namespace base
}  // namespace principia

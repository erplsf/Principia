#pragma once

#include <set>
#include <string>

#include "absl/container/btree_map.h"
#include "base/macros.hpp"  // 🧙 For forward declarations.
#include "base/not_null.hpp"

namespace principia {
namespace ksp_plugin {

FORWARD_DECLARE(class, Part, FROM(part), INTO(identification));
FORWARD_DECLARE(class, Vessel, FROM(vessel), INTO(identification));

namespace _identification {
namespace internal {

using namespace principia::base::_not_null;

// The GUID of a vessel, obtained by `v.id.ToString()` in C#. We use this as a
// key in a map.
using GUID = std::string;

// Corresponds to KSP's `Part.flightID`, *not* to `Part.uid`.  C#'s `uint`
// corresponds to `uint32_t`.
using PartId = std::uint32_t;

// Comparator by PartId.  Useful for ensuring a consistent ordering in sets of
// pointers to Parts.
struct PartByPartIdComparator {
  bool operator()(not_null<Part*> left, not_null<Part*> right) const;
  bool operator()(not_null<Part const*> left,
                  not_null<Part const*> right) const;
};

// Comparator by GUID.  Useful for ensuring a consistent ordering in sets of
// pointers to Vessels.
struct VesselByGUIDComparator {
  bool operator()(not_null<Vessel*> left, not_null<Vessel*> right) const;
  bool operator()(not_null<Vessel const*> left,
                  not_null<Vessel const*> right) const;
};

template<typename T>
using PartTo = absl::btree_map<not_null<Part*>,
                               T,
                               PartByPartIdComparator>;
using VesselSet = std::set<not_null<Vessel*>,
                           VesselByGUIDComparator>;
using VesselConstSet = std::set<not_null<Vessel const*>,
                                VesselByGUIDComparator>;

}  // namespace internal

using internal::GUID;
using internal::PartByPartIdComparator;
using internal::PartId;
using internal::PartTo;
using internal::VesselByGUIDComparator;
using internal::VesselConstSet;
using internal::VesselSet;

}  // namespace _identification
}  // namespace ksp_plugin
}  // namespace principia

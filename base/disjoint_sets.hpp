#pragma once

#include <optional>
#include <list>

#include "base/not_null.hpp"

namespace principia {
namespace base {
namespace _disjoint_sets {
namespace internal {

using namespace principia::base::_not_null;

// For the purposes of this class, `T` represents the set of its values, and
// a single globally unique partition is being built.  If `MakeSingleton` is
// called on an element `e` of type `T`, all properties of the subset previously
// containing `e` are invalidated.
// To use an union-find algorithm on elements of `T`, specialize
// `Subset<T>::Node::Get`, run `Subset<T>::MakeSingleton` on all elements
// involved, and proceed with calls to `Subset<T>::Unite` and `Subset<T>::Find`.

// A subset of `T`.
template<typename T>
class Subset final {
 public:
  // Any properties about a subset of `T` that can be efficiently maintained
  // when merging (e.g. a list of elements) should be kept in
  // `Subset<T>::Properties`; specialize it as needed.
  class Properties final {
   public:
    void MergeWith(Properties& other) {}
  };

  // The `SubsetPropertiesArgs` are forwarded to the constructor of
  // `Properties`; the constructed `Properties` are owned by
  // `*Node::Get(element)`, and thus by element.
  template<typename... SubsetPropertiesArgs>
  static Subset MakeSingleton(
      T& element,
      SubsetPropertiesArgs... subset_properties_args);

  // The arguments are invalidated; the result may be used to get information
  // about the united subset.
  static Subset Unite(Subset left, Subset right);
  // Returns the subset containing `element`.
  static Subset Find(T& element);

  Properties const& properties() const;
  Properties& mutable_properties();

  class Node final {
   public:
    Node();

   private:
    // Specialize to return a `Node` owned by `element` (in constant time).  The
    // compiler will warn about returning from a non-void function if this is
    // not specialized.
    static not_null<typename Subset::Node*> Get(T& element) {}

    not_null<Node*> Root();

    not_null<Node*> parent_;
    int rank_ = 0;

    // Do not require a default constructor for `Properties`.
    std::optional<Properties> properties_;

    friend class Subset<T>;
  };

  // These operators cannot be defaulted because that would force instantiation
  // of `not_null<Node*>` which itself would force instantiation of
  // `std::optional<Properties>` which itself would force instantiation of
  // `Properties`, thereby preventing further specialization in
  // `part_subsets.hpp`.
  friend bool operator==(Subset left, Subset right) {
    return left.node_ == right.node_;
  }
  friend bool operator!=(Subset left, Subset right) {
    return left.node_ != right.node_;
  }

 private:
  explicit Subset(not_null<Node*> node);

  not_null<Node*> const node_;
};

}  // namespace internal

using internal::Subset;

}  // namespace _disjoint_sets
}  // namespace base
}  // namespace principia

#include "base/disjoint_sets_body.hpp"

#pragma once

#include <algorithm>
#include <memory>
#include <type_traits>
#include <utility>

#include "base/not_constructible.hpp"
#include "base/traits.hpp"
#include "glog/logging.h"

// This file defines a pointer wrapper |not_null| that statically ensures
// non-nullness where possible, and performs runtime checks at the point of
// conversion otherwise.
// The point is to replace cases of undefined behaviour (dereferencing a null
// pointer) by well-defined, localized, failure.
// For instance, when dereferencing a null pointer into a reference, a segfault
// will generally not occur when the pointer is dereferenced, but where the
// reference is used instead, making it hard to track where an invariant was
// violated.
// The static typing of |not_null| also optimizes away some unneeded checks:
// a function taking a |not_null| argument will not need to check its arguments,
// the caller has to provide a |not_null| pointer instead.  If the object passed
// is already a |not_null|, no check needs to be performed.
// The syntax is as follows:
//   not_null<int*> p  // non-null pointer to an |int|.
//   not_null<std::unique_ptr<int>> p // non-null unique pointer to an |int|.
// |not_null| does not have a default constructor, since there is no non-null
// default valid pointer.  The only ways to construct a |not_null| pointer,
// other than from existing instances of |not_null|, are (implicit) conversion
// from a nullable pointer, the factory |check_not_null|, and
// |make_not_null_unique|.
//
// The following example shows uses of |not_null|:
//   void Accumulate(not_null<int*> const accumulator,
//                   not_null<int const*> const term) {
//     *accumulator += *term;  // This will not dereference a null pointer.
//   }
//
//   void InterfaceAccumulator(int* const dubious_accumulator,
//                             int const* const term_of_dubious_c_provenance) {
//     // The call below performs two checks.  If either parameter is null, the
//     // program will fail (through a glog |CHECK|) at the callsite.
//     Accumulate(dubious_accumulator, term_of_dubious_c_provenance);
//   }
//
//   void UseAccumulator() {
//     not_null<std::unique_ptr<int>> accumulator =
//         make_not_null_unique<int>(0);
//     not_null<int> term = // ...
//     // This compiles, no check is performed.
//     Accumulate(accumulator.get(), term);
//     // ...
//   }
//
// If implicit conversion from a nullable pointer to |not_null| does not work,
// e.g. for template deduction, use the factory |check_not_null|.
//   template<typename T>
//   void Foo(not_null<T*> not_null_ptr);
//
//   SomeType* ptr;
//   // The following will not compile:
//   Foo(ptr);
//   // The following works:
//   Foo(check_not_null(ptr));
//   // It is more concise than the alternative:
//   Foo(not_null<SomeType*>(ptr))
//
// The following optimization is made possible by the use of |not_null|:
// |term == nullptr| can be expanded to false through inlining, so the branch
// will likely be optimized away.
//   if (term == nullptr) // ...
//   // Same as above.
//   if (term) // ...

namespace principia {
namespace base {
namespace _not_null {
namespace internal {

using namespace principia::base::_not_constructible;
using namespace principia::base::_traits;

template<typename Pointer>
class not_null;

// Type traits.

// |remove_not_null_t<not_null<T>>| is |remove_not_null_t<T>|.  The recurrence
// ends when |T| is not an instance of |not_null|, in which case
// |remove_not_null_t<T>| is |T|.
template<typename Pointer>
struct remove_not_null : not_constructible {
  using type = Pointer;
};
template<typename Pointer>
struct remove_not_null<not_null<Pointer>> {
  using type = typename remove_not_null<Pointer>::type;
};

template<typename T>
using remove_not_null_t = typename remove_not_null<T>::type;

// Wrapper struct for pointers with move assigment compatible with not_null.
template <typename Pointer, typename = void>
struct NotNullStorage;

// Case: move assignment is equvalent to copy (raw pointer).
template <typename P>
struct NotNullStorage<
    P, std::enable_if_t<std::is_trivially_move_assignable_v<P>>> {
  explicit NotNullStorage(P&& pointer) : pointer(std::move(pointer)) {}
  NotNullStorage(NotNullStorage const&) = default;
  NotNullStorage(NotNullStorage&&) = default;
  NotNullStorage& operator=(NotNullStorage const&) = default;
  NotNullStorage& operator=(NotNullStorage&&) = default;

  P pointer;
};

// Case: move assignent is nontrivial (unique_ptr).
template <typename P>
struct NotNullStorage<
    P, std::enable_if_t<!std::is_trivially_move_assignable_v<P>>> {
  explicit NotNullStorage(P&& pointer) : pointer(std::move(pointer)) {}
  NotNullStorage(NotNullStorage const&) = default;
  NotNullStorage(NotNullStorage&&) = default;
  NotNullStorage& operator=(NotNullStorage const&) = default;
  // Implemented as a swap, so the argument remains valid.
  NotNullStorage& operator=(NotNullStorage&& other) {
    std::swap(pointer, other.pointer);
    return *this;
  }

  P pointer;
};

// When |T| is not a reference, |_checked_not_null<T>| is |not_null<T>| if |T|
// is not already an instance of |not_null|.  It fails otherwise.
// |_checked_not_null| is invariant under application of reference or rvalue
// reference to its template argument.
template<typename Pointer>
using _checked_not_null = typename std::enable_if_t<
    !is_instance_of_v<not_null, typename std::remove_reference_t<Pointer>>,
    not_null<typename std::remove_reference_t<Pointer>>>;

// We cannot refer to the template |not_null| inside of |not_null|.
template<typename Pointer>
inline constexpr bool is_instance_of_not_null_v =
    is_instance_of_v<not_null, Pointer>;

// |not_null<Pointer>| is a wrapper for a non-null object of type |Pointer|.
// |Pointer| should be a C-style pointer or a smart pointer.  |Pointer| must not
// be a const, reference, rvalue reference, or |not_null|.  |not_null<Pointer>|
// is movable and may be left in an invalid state when moved, i.e., its
// |storage_.pointer| may become null.
// |not_null<not_null<Pointer>>| and |not_null<Pointer>| are equivalent.
// This is useful when a |template<typename T>| using a |not_null<T>| is
// instanced with an instance of |not_null|.
template<typename Pointer>
class not_null final {
 public:
  // The type of the pointer being wrapped.
  // This follows the naming convention from |std::unique_ptr|.
  using pointer = typename remove_not_null_t<Pointer>;

  // Smart pointers define this type.
  using element_type = typename std::pointer_traits<pointer>::element_type;

  not_null() = delete;

  // Copy constructor from an other |not_null<Pointer>|.
  constexpr not_null(not_null const&) = default;
  // Copy contructor for implicitly convertible pointers.
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               std::is_convertible_v<OtherPointer, pointer>>>
  constexpr not_null(not_null<OtherPointer> const& other);
  // Constructor from a nullable pointer, performs a null check.
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               std::is_convertible_v<OtherPointer, pointer> &&
               !is_instance_of_not_null_v<pointer>>>
  constexpr not_null(OtherPointer other);  // NOLINT(runtime/explicit)
  // Explicit copy constructor for static_cast'ing.
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               !std::is_convertible_v<OtherPointer, pointer>>,
           typename = decltype(static_cast<pointer>(
                                   std::declval<OtherPointer>()))>
  constexpr explicit not_null(not_null<OtherPointer> const& other);

  // Move constructor from an other |not_null<Pointer>|.  This constructor may
  // invalidate its argument.
  constexpr not_null(not_null&&) = default;
  // Move contructor for implicitly convertible pointers. This constructor may
  // invalidate its argument.
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               std::is_convertible_v<OtherPointer, pointer>>>
  constexpr not_null(not_null<OtherPointer>&& other);
  // Explicit move constructor for static_cast'ing. This constructor may
  // invalidate its argument.
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               !std::is_convertible_v<OtherPointer, pointer>>,
           typename = decltype(static_cast<pointer>(
                                   std::declval<OtherPointer>()))>
  constexpr explicit not_null(not_null<OtherPointer>&& other);

  // Copy assigment operators.
  not_null& operator=(not_null const&) = default;
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               std::is_convertible_v<OtherPointer, pointer>>>
  constexpr not_null& operator=(not_null<OtherPointer> const& other);

  // Move assignment operators.
  not_null& operator=(not_null&& other) = default;
  // This operator may invalidate its argument.
  template<typename OtherPointer,
           typename = typename std::enable_if_t<
               std::is_convertible_v<OtherPointer, pointer>>>
  constexpr not_null& operator=(not_null<OtherPointer>&& other);

  constexpr operator pointer() const;

  template<typename OtherPointer,
           typename = std::enable_if_t<
               std::is_convertible_v<pointer, OtherPointer> &&
               !std::is_same_v<pointer, OtherPointer> &&
               !is_instance_of_not_null_v<OtherPointer>>>
  constexpr operator OtherPointer() const&;

  // Used to convert a |not_null<unique_ptr<>>| to |unique_ptr<>|.
  constexpr operator pointer&&() &&;

  template<typename OtherPointer,
           typename = std::enable_if_t<
               std::is_convertible_v<pointer, OtherPointer> &&
               !std::is_same_v<pointer, OtherPointer> &&
               !is_instance_of_not_null_v<OtherPointer>>>
  constexpr operator OtherPointer() &&;  // NOLINT(whitespace/operators)

  // Returns |*storage_.pointer|.
  constexpr std::add_lvalue_reference_t<element_type> operator*() const;
  constexpr std::add_pointer_t<element_type> operator->() const;

  // When |pointer| has a |get()| member function, this returns
  // |storage_.pointer.get()|.
  template<typename P = pointer, typename = decltype(std::declval<P>().get())>
  constexpr not_null<decltype(std::declval<P>().get())> get() const;

  // When |pointer| has a |release()| member function, this returns
  // |storage_.pointer.release()|.  May invalidate its argument.
  template<typename P = pointer,
           typename = decltype(std::declval<P>().release())>
  constexpr not_null<decltype(std::declval<P>().release())> release();

  // When |pointer| has a |reset()| member function, this calls
  // |storage_.pointer.reset()|.
  template<typename Q,
           typename P = pointer,
           typename = decltype(std::declval<P>().reset())>
  constexpr void reset(not_null<Q> ptr);

  // The following operators are redundant for valid |not_null<Pointer>|s with
  // the implicit conversion to |pointer|, but they should allow some
  // optimization.

  // Returns |false|.
  constexpr bool operator==(std::nullptr_t other) const;
  // Returns |true|.
  constexpr bool operator!=(std::nullptr_t other) const;
  // Returns |true|.
  constexpr operator bool() const;

  // Equality.
  constexpr bool operator==(pointer other) const;
  constexpr bool operator==(not_null other) const;
  constexpr bool operator!=(pointer other) const;
  constexpr bool operator!=(not_null other) const;

  // Ordering.
  constexpr bool operator<(not_null other) const;
  constexpr bool operator<=(not_null other) const;
  constexpr bool operator>=(not_null other) const;
  constexpr bool operator>(not_null other) const;

 private:
  struct unchecked_tag final {};

  // Creates a |not_null<Pointer>| whose |storage_.pointer| equals the given
  // |pointer|, dawg.  The constructor does *not* perform a null check.  Callers
  // must perform one if needed before using it.
  constexpr explicit not_null(pointer other, unchecked_tag tag);

  NotNullStorage<Pointer> storage_;

  static constexpr unchecked_tag unchecked_tag_{};

  template<typename OtherPointer>
  friend class not_null;

  template<typename P>
  friend _checked_not_null<P> check_not_null(P pointer);
  template<typename T, typename... Args>
  friend not_null<std::shared_ptr<T>> make_not_null_shared(Args&&... args);
  template<typename T, typename... Args>
  friend not_null<std::unique_ptr<T>> make_not_null_unique(Args&&... args);
};

// We want only one way of doing things, and we can't make
// |not_null<Pointer> const| and |not_null<Pointer const>| etc. equivalent
// easily.

// Use |not_null<Pointer> const| instead.
template<typename Pointer>
class not_null<Pointer const>;
// Use |not_null<Pointer>&| instead.
template<typename Pointer>
class not_null<Pointer&>;
// Use |not_null<Pointer>&&| instead.
template<typename Pointer>
class not_null<Pointer&&>;

// Factory taking advantage of template argument deduction.  Returns a
// |not_null<Pointer>| to |*pointer|.  |CHECK|s that |pointer| is not null.
template<typename Pointer>
_checked_not_null<Pointer> check_not_null(Pointer pointer);

#if 0
// While the above factory would cover this case using the implicit
// conversion, this results in a redundant |CHECK|.
// This function returns its argument.
template<typename Pointer>
not_null<Pointer> check_not_null(not_null<Pointer> pointer);
#endif

template<typename T, typename... Args>
not_null<std::shared_ptr<T>> make_not_null_shared(Args&&... args);

// Factory for a |not_null<std::unique_ptr<T>>|, forwards the arguments to the
// constructor of T.  |make_not_null_unique<T>(args)| is interchangeable with
// |check_not_null(make_unique<T>(args))|, but does not perform a |CHECK|, since
// the result of |make_unique| is not null.
template<typename T, typename... Args>
not_null<std::unique_ptr<T>> make_not_null_unique(Args&&... args);

// For logging.
template<typename Pointer>
std::ostream& operator<<(std::ostream& stream,
                         not_null<Pointer> const& pointer);

template<typename Result, typename T>
not_null<Result> dynamic_cast_not_null(not_null<T*> const pointer);

template<typename Result, typename T>
not_null<Result> dynamic_cast_not_null(not_null<std::unique_ptr<T>>&& pointer);

}  // namespace internal

using internal::check_not_null;
using internal::dynamic_cast_not_null;
using internal::make_not_null_shared;
using internal::make_not_null_unique;
using internal::not_null;
using internal::remove_not_null;

}  // namespace _not_null
}  // namespace base
}  // namespace principia

#include "base/not_null_body.hpp"

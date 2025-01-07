#pragma once

#include "journal/method.hpp"

#include <list>

#include "journal/recorder.hpp"

namespace principia {
namespace journal {
namespace _method {
namespace internal {

using namespace principia::journal::_recorder;

template<typename Profile>
Method<Profile>::Method() {
  if (Recorder::active_recorder_ != nullptr) {
    serialization::Method method;
    [[maybe_unused]] auto* const message_in =
        method.MutableExtension(Profile::Message::extension);
    Recorder::active_recorder_->WriteAtConstruction(method);
  }
}

template<typename Profile>
template<typename P>
Method<Profile>::Method(typename P::In const& in)
  requires has_in<P> && (!has_out<P>) {
  if (Recorder::active_recorder_ != nullptr) {
    serialization::Method method;
    auto* const message_in =
        method.MutableExtension(Profile::Message::extension);
    Profile::Fill(in, message_in);
    Recorder::active_recorder_->WriteAtConstruction(method);
  }
}

template<typename Profile>
template<typename P>
Method<Profile>::Method(typename P::Out const& out)
  requires has_out<P> && (!has_in<P>) {
  if (Recorder::active_recorder_ != nullptr) {
    serialization::Method method;
    [[maybe_unused]] auto* const message_in =
        method.MutableExtension(Profile::Message::extension);
    Recorder::active_recorder_->WriteAtConstruction(method);
    out_filler_ = [out](
        not_null<typename Profile::Message*> const message) {
      Profile::Fill(out, message);
    };
  }
}

template<typename Profile>
template<typename P>
Method<Profile>::Method(typename P::In const& in,
                        typename P::Out const& out)
  requires has_in<P> && has_out<P> {
  if (Recorder::active_recorder_ != nullptr) {
    serialization::Method method;
    auto* const message_in =
        method.MutableExtension(Profile::Message::extension);
    Profile::Fill(in, message_in);
    Recorder::active_recorder_->WriteAtConstruction(method);
    out_filler_ = [out](
        not_null<typename Profile::Message*> const message) {
      Profile::Fill(out, message);
    };
  }
}

template<typename Profile>
Method<Profile>::~Method() {
  CHECK(returned_);
  if (Recorder::active_recorder_ != nullptr) {
    serialization::Method method;
    auto* const extension =
        method.MutableExtension(Profile::Message::extension);
    if (out_filler_ != nullptr) {
      out_filler_(extension);
    }
    if (return_filler_ != nullptr) {
      return_filler_(extension);
    }
    Recorder::active_recorder_->WriteAtDestruction(method);
  }
}

template<typename Profile>
template<typename P>
void Method<Profile>::Return()
  requires (!has_return<P>) {  // NOLINT
  CHECK(!returned_);
  returned_ = true;
}

template<typename Profile>
template<typename P>
typename P::Return Method<Profile>::Return(
    typename P::Return const& result)
  requires has_return<P> {
  CHECK(!returned_);
  returned_ = true;
  if (Recorder::active_recorder_ != nullptr) {
    return_filler_ =
        [result](not_null<typename Profile::Message*> const message) {
          Profile::Fill(result, message);
        };
  }
  return result;
}

}  // namespace internal
}  // namespace _method
}  // namespace journal
}  // namespace principia

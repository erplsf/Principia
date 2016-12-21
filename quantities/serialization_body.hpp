﻿
#pragma once

#include "quantities/serialization.hpp"

#include "base/not_null.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace quantities {
namespace internal_serialization {

using base::not_null;

template<typename Dimensions, typename Message>
class DoubleOrQuantitySerializer<Quantity<Dimensions>, Message> : type_trait {
 public:
  using T = Quantity<Dimensions>;
  static void WriteToMessage(T const& t, not_null<Message*> const message) {
    t.WriteToMessage(message->mutable_quantity());
  }

  static T ReadFromMessage(Message const& message) {
    CHECK(message.has_quantity());
    return T::ReadFromMessage(message.quantity());
  }
};

template<typename Message>
class DoubleOrQuantitySerializer<double, Message> : type_trait {
 public:
  static void WriteToMessage(double const d,
                             not_null<Message*> const message) {
    message->set_double_(d);
  }

  static double ReadFromMessage(Message const& message) {
    CHECK(message.has_double_());
    return message.double_();
  }
};

}  // namespace internal_serialization
}  // namespace quantities
}  // namespace principia

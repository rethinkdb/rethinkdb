// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_TESTING_GMOCK_SUPPORT_H_
#define V8_TESTING_GMOCK_SUPPORT_H_

#include "testing/gmock/include/gmock/gmock.h"

namespace testing {

template <typename T>
class Capture {
 public:
  Capture() : value_(), has_value_(false) {}

  const T& value() const { return value_; }
  bool has_value() const { return has_value_; }

  void SetValue(const T& value) {
    DCHECK(!has_value());
    value_ = value;
    has_value_ = true;
  }

 private:
  T value_;
  bool has_value_;
};


namespace internal {

template <typename T>
class CaptureEqMatcher : public MatcherInterface<T> {
 public:
  explicit CaptureEqMatcher(Capture<T>* capture) : capture_(capture) {}

  virtual void DescribeTo(std::ostream* os) const {
    *os << "captured by " << static_cast<const void*>(capture_);
    if (capture_->has_value()) *os << " which has value " << capture_->value();
  }

  virtual bool MatchAndExplain(T value, MatchResultListener* listener) const {
    if (!capture_->has_value()) {
      capture_->SetValue(value);
      return true;
    }
    if (value != capture_->value()) {
      *listener << "which is not equal to " << capture_->value();
      return false;
    }
    return true;
  }

 private:
  Capture<T>* capture_;
};

}  // namespace internal


// CaptureEq(capture) captures the value passed in during matching as long as it
// is unset, and once set, compares the value for equality with the argument.
template <typename T>
Matcher<T> CaptureEq(Capture<T>* capture) {
  return MakeMatcher(new internal::CaptureEqMatcher<T>(capture));
}

}  // namespace testing

#endif  // V8_TESTING_GMOCK_SUPPORT_H_

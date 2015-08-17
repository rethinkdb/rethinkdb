// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_STRTOD_H_
#define V8_STRTOD_H_

namespace v8 {
namespace internal {

// The buffer must only contain digits in the range [0-9]. It must not
// contain a dot or a sign. It must not start with '0', and must not be empty.
double Strtod(Vector<const char> buffer, int exponent);

} }  // namespace v8::internal

#endif  // V8_STRTOD_H_

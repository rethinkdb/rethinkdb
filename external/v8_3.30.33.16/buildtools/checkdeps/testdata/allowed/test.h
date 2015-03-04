// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "checkdeps/testdata/allowed/good.h"
#include "checkdeps/testdata/disallowed/bad.h"
#include "checkdeps/testdata/disallowed/allowed/good.h"
#include "checkdeps/testdata/disallowed/temporarily_allowed.h"
#include "third_party/explicitly_disallowed/bad.h"
#include "third_party/allowed_may_use/good.h"
#include "third_party/no_rule/bad.h"

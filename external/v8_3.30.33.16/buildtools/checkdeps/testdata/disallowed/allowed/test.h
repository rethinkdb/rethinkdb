// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "checkdeps/testdata/allowed/good.h"
// Always allowed to include self and parents.
#include "checkdeps/testdata/disallowed/good.h"
#include "checkdeps/testdata/disallowed/allowed/good.h"
#include "third_party/explicitly_disallowed/bad.h"
#include "third_party/allowed_may_use/bad.h"
#include "third_party/no_rule/bad.h"

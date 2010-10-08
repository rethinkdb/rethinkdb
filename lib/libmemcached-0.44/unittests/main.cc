/* -*- mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2010 Monty Taylor
 *
 *  All rights reserved.
 *
 *  Use and distribution licensed under the BSD license.  See
 *  the COPYING file in the parent directory for full text.
 */

#include "config.h"

#include <gtest/gtest.h>


int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

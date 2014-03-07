/*
******************************************************************************
* Copyright (C) 2009-2010, International Business Machines Corporation and
* others. All Rights Reserved.
******************************************************************************
*   Date        Name        Description
*   12/14/09    doug        Creation.
******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/fpositer.h"
#include "cmemory.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_NO_RTTI_IMPLEMENTATION(FieldPositionIterator)

FieldPositionIterator::~FieldPositionIterator() {
  delete data;
  data = NULL;
  pos = -1;
}

FieldPositionIterator::FieldPositionIterator()
    : data(NULL), pos(-1) {
}

FieldPositionIterator::FieldPositionIterator(const FieldPositionIterator &rhs)
  : UObject(rhs), data(NULL), pos(rhs.pos) {

  if (rhs.data) {
    UErrorCode status = U_ZERO_ERROR;
    data = new UVector32(status);
    data->assign(*rhs.data, status);
    if (status != U_ZERO_ERROR) {
      delete data;
      data = NULL;
      pos = -1;
    }
  }
}

UBool FieldPositionIterator::operator==(const FieldPositionIterator &rhs) const {
  if (&rhs == this) {
    return TRUE;
  }
  if (pos != rhs.pos) {
    return FALSE;
  }
  if (!data) {
    return rhs.data == NULL;
  }
  return rhs.data ? data->operator==(*rhs.data) : FALSE;
}

void FieldPositionIterator::setData(UVector32 *adopt, UErrorCode& status) {
  // Verify that adopt has valid data, and update status if it doesn't.
  if (U_SUCCESS(status)) {
    if (adopt) {
      if ((adopt->size() % 3) != 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
      } else {
        for (int i = 1; i < adopt->size(); i += 3) {
          if (adopt->elementAti(i) >= adopt->elementAti(i+1)) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            break;
          }
        }
      }
    }
  }

  // We own the data, even if status is in error, so we need to delete it now
  // if we're not keeping track of it.
  if (!U_SUCCESS(status)) {
    delete adopt;
    return;
  }

  delete data;
  data = adopt;
  pos = adopt == NULL ? -1 : 0;
}

UBool FieldPositionIterator::next(FieldPosition& fp) {
  if (pos == -1) {
    return FALSE;
  }

  fp.setField(data->elementAti(pos++));
  fp.setBeginIndex(data->elementAti(pos++));
  fp.setEndIndex(data->elementAti(pos++));

  if (pos == data->size()) {
    pos = -1;
  }

  return TRUE;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */


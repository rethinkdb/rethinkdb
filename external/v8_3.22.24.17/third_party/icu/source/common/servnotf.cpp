/**
 *******************************************************************************
 * Copyright (C) 2001-2006, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_SERVICE

#include "servnotf.h"
#ifdef NOTIFIER_DEBUG
#include <stdio.h>
#endif

U_NAMESPACE_BEGIN

EventListener::~EventListener() {}
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(EventListener)

ICUNotifier::ICUNotifier(void) 
: notifyLock(0), listeners(NULL) 
{
    umtx_init(&notifyLock);
}

ICUNotifier::~ICUNotifier(void) {
    {
        Mutex lmx(&notifyLock);
        delete listeners;
        listeners = NULL;
    }
    umtx_destroy(&notifyLock);
}


void 
ICUNotifier::addListener(const EventListener* l, UErrorCode& status) 
{
    if (U_SUCCESS(status)) {
        if (l == NULL) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }

        if (acceptsListener(*l)) {
            Mutex lmx(&notifyLock);
            if (listeners == NULL) {
                listeners = new UVector(5, status);
            } else {
                for (int i = 0, e = listeners->size(); i < e; ++i) {
                    const EventListener* el = (const EventListener*)(listeners->elementAt(i));
                    if (l == el) {
                        return;
                    }
                }
            }

            listeners->addElement((void*)l, status); // cast away const
        }
#ifdef NOTIFIER_DEBUG
        else {
            fprintf(stderr, "Listener invalid for this notifier.");
            exit(1);
        }
#endif
    }
}

void 
ICUNotifier::removeListener(const EventListener *l, UErrorCode& status) 
{
    if (U_SUCCESS(status)) {
        if (l == NULL) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }

        {
            Mutex lmx(&notifyLock);
            if (listeners != NULL) {
                // identity equality check
                for (int i = 0, e = listeners->size(); i < e; ++i) {
                    const EventListener* el = (const EventListener*)listeners->elementAt(i);
                    if (l == el) {
                        listeners->removeElementAt(i);
                        if (listeners->size() == 0) {
                            delete listeners;
                            listeners = NULL;
                        }
                        return;
                    }
                }
            }
        }
    }
}

void 
ICUNotifier::notifyChanged(void) 
{
    if (listeners != NULL) {
        Mutex lmx(&notifyLock);
        if (listeners != NULL) {
            for (int i = 0, e = listeners->size(); i < e; ++i) {
                EventListener* el = (EventListener*)listeners->elementAt(i);
                notifyListener(*el);
            }
        }
    }
}

U_NAMESPACE_END

/* UCONFIG_NO_SERVICE */
#endif


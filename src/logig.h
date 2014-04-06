/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the ig-logger project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Original author: Christian Kaiser <info@invest-tools.com>
 */

#ifndef INCLUDED_LOGIG_H
#define INCLUDED_LOGIG_H

const unsigned MAX_MSG_SIZE = 128;

#define GPFPROTECT  1

#include "clsSerialIO.h"

extern bool bUserAbortRequested();
extern void AlertUser(const String& sSubject,
                      const String& sDetails = __sEmptyString);

#endif // !defined INCLUDED_LOGIG_H

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

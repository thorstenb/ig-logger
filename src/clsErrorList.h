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

#ifndef INCLUDED_ERRORLIST_H
#define INCLUDED_ERRORLIST_H

#include "clsSystemtime.h"

typedef WCValSkipListDict<clsSYSTEMTIME,String>     clsErrorList;
typedef WCValSkipListDictIter<clsSYSTEMTIME,String> clsErrorListIter;

#endif // !defined INCLUDED_ERRORLIST_H

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

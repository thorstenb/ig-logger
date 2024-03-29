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

#pragma once

#include "..\logig2\clsSystemtime.h"

typedef WCValSkipListDict<clsSYSTEMTIME,String>		clsErrorList;
typedef WCValSkipListDictIter<clsSYSTEMTIME,String>	clsErrorListIter;


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

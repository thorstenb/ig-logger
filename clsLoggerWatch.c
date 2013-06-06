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

#include "logig.h"

#include "clsExcept.h"
#include "clsLocalNet.h"

clsLoggerWatch::clsLoggerWatch(clsLocalNet* pLocalNet)
	: clsThreadBase("clsLoggerWatch")
	, _pLocalNet(pLocalNet)
{
	ThreadStart();
}

clsLoggerWatch::~clsLoggerWatch()
{
	ThreadJoin();
}

void							clsLoggerWatch::Watch(const clsDeviceID& ID)
{
	_ID = ID;
}


virtual DWORD					clsLoggerWatch::ThreadFunction(void)
{
	UINT	nErrorCount = 0;

	while (ThreadWait(1100))
		{
		if (_ID.bOk())
			{
			try
				{
				clsDeviceID	ID;

				if (_pLocalNet->QueryDataloggerID(ID))
					{
					// printf(" ID: %x-%x\n",nID,_nID);
					if (ID != _ID)
						{
						_pLocalNet->AddException(
							new clsExcept(
								"Warning: datalogger ID did change unexpectedly!\n(old: %s, new: %s)",
								(LPCTSTR)_ID.AsStr(),
								(LPCTSTR)ID.AsStr()
								));
						}
					nErrorCount = 0;
					}
				  else
					{
					if (++nErrorCount >= 3)
						{
						_pLocalNet->AddException(
							new clsConnectionLostExcept(
								"Warning: datalogger %s cannot be reached!",
								(LPCTSTR)_ID.AsStr()
								));
						}
					}
				}
			catch (clsExceptBase& e)
				{
				e.Push("Warning: datalogger %s cannot be reached! %s",
					(LPCTSTR)_ID.AsStr());
				_pLocalNet->AddException(
					new clsConnectionLostExcept(e));
				}
			}
		}
	return(0);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

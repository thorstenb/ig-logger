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

bool							clsLocalNet::QueryLoggerDateTime(clsSYSTEMTIME& ST)
{
	bool			bRes(false);
	clsResult<bool>	APILog("clsLocalNet::QueryLoggerDateTime",bRes);

	for (int nTry = 0; nTry < 3 && !bRes && !bUserAbortRequested(); ++nTry)
		{
		clsIGMessage	MsgQuery(	IGID_PC,
									IGID_DATALOGGER,
									clsIGMessage::IGCMD_DL_GET_DATETIME);
		clsIGMessage	MsgResponse;

		Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

		if (Query(MsgQuery,MsgResponse))
			{
			if (MsgResponse.PayloadSize() == 7)
				{
				const BYTE*	pPayload = MsgResponse.Payload();

				ST.wYear = 2000+pPayload[5];
				ST.wMonth = pPayload[4];
				ST.wDay = pPayload[3];
				ST.wHour = pPayload[2];
				ST.wMinute = pPayload[1];
				ST.wSecond = pPayload[0];
				ST.wDayOfWeek = pPayload[6] % 7;
				bRes = true;
				}
			  else
				{
				throw clsExcept("bad response: bad payload size (%d/%d)",MsgResponse.PayloadSize(),7);
				}
			}
		}
	return(bRes);
}

bool							clsLocalNet::SetLoggerDateTime(const clsSYSTEMTIME& ST)
{
	bool			bRes(false);
	clsResult<bool>	APILog("clsLocalNet::QueryLoggerDateTime",bRes);
	clsIGMessage	MsgQuery(	IGID_PC,
								IGID_DATALOGGER,
								clsIGMessage::IGCMD_DL_SET_DATETIME);
	clsIGMessage	MsgResponse;

	MsgQuery.SetPayload(6,ST.wSecond,ST.wMinute,ST.wHour,ST.wDay,ST.wMonth,ST.wYear-2000);

	Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

	if (Query(MsgQuery,MsgResponse))
		{
		if (MsgResponse.PayloadSize() == 7)
			{
			const BYTE*		pPayload = MsgResponse.Payload();
			clsSYSTEMTIME	STRsp;

			STRsp.wYear = 2000+pPayload[5];
			STRsp.wMonth = pPayload[4];
			STRsp.wDay = pPayload[3];
			STRsp.wHour = pPayload[2];
			STRsp.wMinute = pPayload[1];
			STRsp.wSecond = pPayload[0];
			STRsp.wDayOfWeek = pPayload[6] % 7;

			printf("new time: %s\n",(LPCTSTR)STRsp.sValue());
			bRes = true;
			}
		  else
			{
			throw clsExcept("bad response: bad payload size (%d/%d)",MsgResponse.PayloadSize(),7);
			}
		}
	return(bRes);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

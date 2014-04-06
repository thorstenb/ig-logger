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

bool                            clsLocalNet::QueryLoggerInverterHistoInterval(
    int& nMinutes)
{
    bool            bRes(false);
    clsResult<bool> APILog("clsLocalNet::QueryLoggerInverterHistoInterval()",bRes);

    /*
    only minutes that are divisible by 5 are allowed, otherwise no logging is done.
    */
    for (int nTry = 0; nTry < 3 && !bRes && !bUserAbortRequested(); ++nTry)
    {
        clsIGMessage    MsgQuery(   IGID_PC,
                                    IGID_DATALOGGER,
                                    clsIGMessage::IGCMD_DL_GET_OPTION,
                                    1,
                                    clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL);
        clsIGMessage    MsgResponse;

        Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

        if (Query(MsgQuery,MsgResponse))
        {
            if (MsgResponse.PayloadSize() >= 1)
            {
                const BYTE* pPayload = MsgResponse.Payload();

                nMinutes = pPayload[1] & 0x7f;
                bRes = true;
            }
        }
    }
    return(bRes);
}

bool                            clsLocalNet::SetLoggerInverterHistoInterval(
    const int nMinutes)
{
    bool            bRes(false);
    clsResult<bool> APILog(
        String::Format("clsLocalNet::SetLoggerInverterHistoInterval(%d minutes)",
                       nMinutes),bRes);

    /*
    only minutes that are divisible by 5 are allowed, otherwise no logging is done.
    */
    if (nMinutes > 0)
    {
        clsIGMessage    MsgQuery(   IGID_PC,
                                    IGID_DATALOGGER,
                                    clsIGMessage::IGCMD_DL_SET_OPTION,
                                    3,
                                    clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL,
                                    0xc3,
                                    0x80 + nMinutes);
        clsIGMessage    MsgResponse;

        Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

        if (Query(MsgQuery,MsgResponse))
        {
            if (MsgResponse.PayloadSize() >= 4)
            {
                const BYTE* pPayload = MsgResponse.Payload();

                if (pPayload[3] != 6)
                {
                    throw clsFatalExcept("cannot set historical interval to %d!",nMinutes);
                }

                bRes = true;
            }
        }
        else
        {
            throw clsExcept("bad response: bad payload size (%d/%d)",
                            MsgResponse.PayloadSize(),3);
        }
    }

    return(bRes);
}

bool                            clsLocalNet::QueryLoggerSensorHistoInterval(
    int& nMinutes)
{
    bool            bRes(false);
    clsResult<bool> APILog("clsLocalNet::QueryLoggerSensorHistoInterval()",bRes);

    /*
    only minutes that are divisible by 5 are allowed, otherwise no logging is done.
    */
    for (int nTry = 0; nTry < 3 && !bRes && !bUserAbortRequested(); ++nTry)
    {
        clsIGMessage    MsgQuery(   IGID_PC,
                                    IGID_DATALOGGER,
                                    clsIGMessage::IGCMD_DL_GET_OPTION,
                                    1,
                                    clsIGMessage::IG_DL_OPTION_SENSORHISTOINTERVAL);
        clsIGMessage    MsgResponse;

        Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

        if (Query(MsgQuery,MsgResponse))
        {
            if (MsgResponse.PayloadSize() >= 1)
            {
                const BYTE* pPayload = MsgResponse.Payload();

                nMinutes = pPayload[1] & 0x7f;
                bRes = true;
            }
        }
    }
    return(bRes);
}

bool                            clsLocalNet::SetLoggerSensorHistoInterval(
    const int nMinutes)
{
    bool            bRes(false);
    clsResult<bool> APILog(
        String::Format("clsLocalNet::SetLoggeSensorrHistoInterval(%d minutes)",
                       nMinutes),bRes);

    /*
    only minutes that are divisible by 5 are allowed, otherwise no logging is done.
    */
    if (nMinutes > 0)
    {
        clsIGMessage    MsgQuery(   IGID_PC,
                                    IGID_DATALOGGER,
                                    clsIGMessage::IGCMD_DL_SET_OPTION,
                                    3,
                                    clsIGMessage::IG_DL_OPTION_SENSORHISTOINTERVAL,
                                    0xc3,
                                    0x80 + nMinutes);
        clsIGMessage    MsgResponse;

        Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

        if (Query(MsgQuery,MsgResponse))
        {
            if (MsgResponse.PayloadSize() >= 4)
            {
                const BYTE* pPayload = MsgResponse.Payload();

                if (pPayload[3] != 6)
                {
                    throw clsFatalExcept("cannot set historical interval to %d!",nMinutes);
                }

                bRes = true;
            }
        }
        else
        {
            throw clsExcept("bad response: bad payload size (%d/%d)",
                            MsgResponse.PayloadSize(),3);
        }
    }

    return(bRes);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

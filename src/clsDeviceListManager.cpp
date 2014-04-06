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

#include <math.h>

#include <GPFProt.hpp>

#include "clsExcept.h"
#include "clsLocalNet.h"

virtual DWORD                   clsDeviceListManager::ThreadFunction(void)
{
    INT         nEvent = 0;

    while (ThreadWait(5000,nEvent,1,_evMSGEvent.Handle()))
    {
        if (nEvent < -1)
        {
            _chk_utildebug("clsDeviceListManager::ThreadFunction: threadWait aborted!");
            break;
        }

        switch (nEvent)
        {
            case -1:
            {
                clsCritObj      CO(_CritOfNetDeviceList);

                _NetDeviceList.clear();
            }
            break;
            default:
                _chk_utildebug("unexpected thread event... %x",nEvent);
                break;
            case 0:
                if (!_ToDoList.isEmpty())
                {
                    clsIGMessage    Msg;

                    {
                        clsCritObj      CO(_CritOfToDoList);

                        Msg = _ToDoList.get();
                    }
                    // 01 00 00   00 01 FE
                    const BYTE* pPayload = Msg.Payload();
                    BYTE        nNetwork = *pPayload++; // just an assumption...

                    {
                        clsCritObj  CO(_CritOfNetDeviceList);

                        _NetDeviceList.clear();
                        for (int nDevice = 0; nDevice < (Msg.PayloadSize()-1)/2; ++nDevice)
                        {
                            enLocalNetID    nID = (enLocalNetID)pPayload[2*nDevice];
                            enLocalNetType  nType = (enLocalNetType)pPayload[2*nDevice+1];
                            clsLocalNetID   Device(nID,nType);

                            _NetDeviceList[clsLocalNetID(nID,nType)] = "";
                        }
                        _NetDeviceList[clsLocalNetID(IGID_NODE_0E,IGTYPE_NODE_0E)] = "Net 0E";
                        _NetDeviceList[clsLocalNetID(IGID_PC,IGTYPE_PC)] = "PC";
                    }

                    clsIGMessage    MsgOut(Msg);
                    BYTE            arData[MAX_MSG_SIZE];
                    BYTE*           pCur = arData;

                    *pCur++ = nNetwork;
                    for (clsDeviceListIter Device(_NetDeviceList); ++Device; )
                    {
                        *pCur++ = (BYTE)Device.key().nID();
                        *pCur++ = (BYTE)Device.key().nType();
                    }

                    MsgOut.SetPayload(arData,pCur-arData);
                    _IGInterface.Post(MsgOut);

#if 0
                    {
                        String      s;

                        for (clsDeviceListIter Device(_NetDeviceList); ++Device; )
                        {
                            s.AddStrSeparated(", ",
                                              String::Format("%s='%s'",
                                                             (LPCTSTR)sLocalNetID(Device.key()),
                                                             (LPCTSTR)Device.value()));
                        }
                        printf("Net devices   : %s\n",(LPCTSTR)s);
                    }
#endif

                    clsCritObj  CO1(_CritOfToDoList);

                    if (_ToDoList.isEmpty())
                    {
                        _evMSGEvent.Reset();
                    }
                }
                break;
        }
    }
    return(0);
}

clsDeviceListManager::clsDeviceListManager(clsIGInterface& IGInterface)
    : clsThreadBase(TEXT("clsDeviceListManager"))
    , _IGInterface(IGInterface)
    , _evMSGEvent(TRUE,FALSE)
{
    ThreadStart();
}

clsDeviceListManager::~clsDeviceListManager()
{
    ThreadJoin();
}

void                            clsDeviceListManager::Clear()
{
    {
        clsCritObj  CO1(_CritOfToDoList);

        _ToDoList.clear();
    }

    {
        clsCritObj  CO2(_CritOfNetDeviceList);

        _NetDeviceList.clear();
    }

    _evMSGEvent.Reset();
}

void                            clsDeviceListManager::OnDeviceList(
    const clsIGMessage& Msg)
{
    _chk_debug(+1,">clsLocalNet::OnDeviceList");
    clsCritObj  CO(_CritOfToDoList);

    _ToDoList.append(Msg);
    _evMSGEvent.Set();
    _chk_debug(-1,"<clsLocalNet::OnDeviceList");
}

bool                            clsDeviceListManager::ContainsNode(
    enLocalNetID nNode)
{
    clsCritObj  CO(_CritOfNetDeviceList);

    for (clsDeviceListIter Device(_NetDeviceList); ++Device; )
    {
        if (Device.key().nID() == nNode)
            return(true);
    }
    return(false);
}

bool                            clsDeviceListManager::ContainsDatalogger()
{
    return(ContainsNode(IGID_DATALOGGER));
}

bool                            clsDeviceListManager::ContainsSensorcard()
{
    for (enLocalNetID nDevice = IGID_SENSORCARD_FIRST;
            nDevice <= IGID_SENSORCARD_LAST; nDevice = (enLocalNetID)(nDevice+1))
    {
        if (ContainsNode(nDevice))
            return(true);
    }
    return(false);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

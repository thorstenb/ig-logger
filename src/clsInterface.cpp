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

void                            Log(const char* pszFormat, ...);

//////////////////////////////////////////////////////////////////////////////

clsIGInboundMessageList::clsIGInboundMessageList(unsigned nMessages)
    : _evMsgPresent(TRUE,FALSE)
    , _nMessages(nMessages)
{
}

void                            clsIGInboundMessageList::Append(
    const clsIGMessage& Msg)
{
    _chk_debug(+1,">clsIGInboundMessageList(%#p)::Append (%d, %d, %d)",this,
               _List.contains(Msg),_List.entries(),_nMessages);
    _Crit.Enter();

    if (_nMessages == 0 &&
            !_List.isEmpty())
    {
        _Crit.Leave();

        clsIGMessage    MsgBase;

        try
        {
            bool    bDone(false);

            for (int nMsg = 0; nMsg < _List.entries(); ++nMsg)
            {
                if (Msg == _List.findC(nMsg))
                {
                    MsgBase = _List.get(nMsg);
                    MsgBase += Msg;

                    _chk_utildebug("append -> '%s'",(LPCTSTR)MsgBase.AsString());

                    _Crit.Enter();
                    _List.append(MsgBase);
                    _Crit.Leave();

                    bDone = true;
                    break;
                }
            }
            if (!bDone)
            {
                _Crit.Enter();
                _List.append(Msg);
                _Crit.Leave();
            }

            _evMsgPresent.Set();
            _chk_utildebug("************ ntfy that a message is present");
        }
        catch (clsExceptBase& e)
        {
            // printf("----------------------------------\n%s\n",(LPCTSTR)e.Text());
            if (_List.isEmpty())
            {
                _evMsgPresent.Reset();
            }
        }
    }
    else
    {
        _List.append(Msg);
        _Crit.Leave();
        _evMsgPresent.Set();
    }
    _chk_debug(-1,"<clsIGInboundMessageList::Append");
}

bool                            clsIGInboundMessageList::Get(clsIGMessage& Msg,
        unsigned nTimeout)
{
    _chk_debug(+1,">clsIGInboundMessageList(%#p)::Get()",this);
    bool    bOk = false;

    while (!bOk)
    {
        // _chk_utildebug(">wait...");
        if (!_evMsgPresent.Wait(nTimeout))
        {
            _chk_utildebug("TIMEOUT waiting for a message");
            break;
        }
        // _chk_utildebug("<wait...");
        clsCritObj  CO(_Crit);

        _chk_utildebug("...check empty=%d",_List.isEmpty());
        while (!_List.isEmpty())
        {
            if (_List.find().bComplete())
            {
                _chk_utildebug("COMPLETE!!! ;-)");
                Msg = _List.get();
                bOk = true;
            }
            else
            {
                _chk_utildebug("INCOMPLETE!!!");

                // wait for next part
                _evMsgPresent.Reset();
                break;
            }
        }
        if (_List.isEmpty())
        {
            _evMsgPresent.Reset();
        }
    }
    _chk_debug(-1,"<clsIGInboundMessageList::Get() -> %s",
               bOk ? (LPCTSTR)Msg.AsString() : "NO MESSAGE");
    return(bOk);
}

bool                            clsIGInboundMessageList::Get(
    WCValSList<clsIGMessage>& MsgList, unsigned nTimeout)
{
    _chk_debug(+1,">clsIGInboundMessageList(%#p)::Get()",this);
    bool    bOk = false;

    while (!bOk)
    {
        // _chk_utildebug(">wait...");
        if (!_evMsgPresent.Wait(nTimeout))
        {
            _chk_utildebug("TIMEOUT waiting for a message");
            break;
        }
        // _chk_utildebug("<wait...");
        clsCritObj  CO(_Crit);

        _chk_utildebug("...check empty=%d (%d,%d,%d)",_List.isEmpty(),_List.entries(),
                       MsgList.entries(),_nMessages);
        while (!_List.isEmpty())
        {
            /*
                        if (_List.find().nBlockNumber() != _nMessages - MsgList.entries())
                            {
                            _chk_utildebug("ERROR: blocks are lost!!!");
                            throw(clsExcept("err: blocks are lost: %d %d"),_List.find().nBlockNumber(),_nMessages - MsgList.entries());
                            }
            */
            MsgList.append(_List.get());
            bOk = (MsgList.entries() == _nMessages);

            if (bOk)
            {
                // messages need not be sorted - please sort them by block ID
                WCValSkipListDict<BYTE,clsIGMessage>    SortedList;

                while (!MsgList.isEmpty())
                {
                    clsIGMessage    Msg(MsgList.get());

                    SortedList.insert(Msg.nBlockNumber(),Msg);
                }
                WCValSkipListDictIter<BYTE,clsIGMessage>    SortedMsg(SortedList);

                while (++SortedMsg)
                {
                    MsgList.append(SortedMsg.value());
                }
            }
        }
        if (_List.isEmpty())
        {
            _evMsgPresent.Reset();
        }
    }
    _chk_debug(-1,"<clsIGInboundMessageList::Get() -> %d/%d",MsgList.entries(),
               _nMessages);
    return(bOk);
}

//////////////////////////////////////////////////////////////////////////////

clsIGInterface::clsIGInterface()
    : _DeviceListManager(*this)
    , _nPacketsReceived(0)
    , _nPacketsReceivedWithBadChecksum(0)
    , _pIO(NULL)
{
}

clsIGInterface::~clsIGInterface()
{
    _DeviceListManager.ThreadJoin();

    clsCritObj  CO(_Crit);

    if (_pIO)
    {
        _pIO->IOSetActive(false);
        Close();
    }
}

void                            clsIGInterface::SetActive(bool bActive)
{
    clsCritObj  CO(_Crit);

    if (_pIO)
    {
        _pIO->IOSetActive(bActive);
    }
}

bool                            clsIGInterface::Open(const String& sDevice)
{
    Close();

    try
    {
        clsCritObj  CO(_Crit);

        if (sDevice.Left(4) == "USB:")
        {
            _pIO = new clsUSBIO(this);
        }
        else if (sDevice.Left(4) == "COM:")
        {
            _pIO = new clsRS232IO(this,57600);
        }
        else if (sDevice.Left(5) == "FTDI:")
        {
            _pIO = new clsFTDIIO(this,57600);
        }
        else if (sDevice.Left(8) == "FTDIW32:")
        {
            _pIO = new clsFTDIW32IO(this,57600);
        }
        else
        {
            throw clsExcept("port '%s' has unknown format",(LPCTSTR)sDevice);
        }

        if (_pIO == NULL)
        {
            return(false);
        }
        if (!_pIO->IOOpen(sDevice.GetToken(1,':')))
        {
            Close();
            return(false);
        }
    }
    catch (clsExcept& e)
    {
        Close();
        throw;
    }

    return(true);
}

void                            clsIGInterface::Close()
{
    clsCritObj  CO(_Crit);

    if (_pIO)
    {
        _pIO->IOClose();
        delete _pIO;
        _pIO = NULL;
    }
}

void                            clsIGInterface::NtfyData(const BYTE* pData,
        UINT nSize)
{
    _chk_debug(+1,">clsIGInterface::NtfyData (%d bytes)",nSize);

    for (int n = 0; n < nSize; ++n)
    {
        // _chk_utildebug("%d: %d %d",_MessageStack.entries(),n,pData[n]);
        if (pData[n] == 0x31)
        {
            // a new message starts
            _MessageStack.push(new clsMessageBuffer);
            continue;
        }

        if (!_MessageStack.isEmpty())
        {
            clsMessageBuffer*   pCurrentMessage = _MessageStack.top();

            if (pData[n] == 0x32)
            {
                // message finished
                clsIGMessage    Msg(pCurrentMessage->pData(),pCurrentMessage->nDataSize());

                if (Msg.bChecksumOk())
                {
                    _chk_debug(0,"received message %s",(LPCTSTR)Msg.AsString());

                    ++_nPacketsReceived;

                    switch (Msg.nCmd())
                    {
                        case clsIGMessage::IGCMD_DEVICELIST:
                            if ((Msg.nFrom() == IGID_DATALOGGER || Msg.nFrom() == IGID_INTERFACECARD) &&
                                    Msg.nTo() == IGID_BROADCAST)
                            {
                                _DeviceListManager.OnDeviceList(Msg);
                                break;
                            }
                        // fallthrough
                        default:
                        {
                            clsCritObj  CO(_CritQueryList);

                            if (_QueryList.contains(Msg))
                            {
                                _chk_utildebug("(is in query list)");
                                _QueryList[Msg]->Append(Msg);
                            }
                            else
                            {
                                _chk_utildebug("(is unassigned message)");
                                _UnassignedMessages.Append(Msg);
                            }
                        }
                        break;
                    }
                }
                else
                {
                    _chk_debug(0,"received message %s with BAD CHECKSUM",(LPCTSTR)Msg.AsString());

                    ++_nPacketsReceivedWithBadChecksum;
                    printf("WRN: received a message with BAD CHECKSUM\n");
                }

                delete _MessageStack.pop();
            }
            else
            {
                pCurrentMessage->Append(pData[n]);
            }
        }
        else
        {
            _chk_debug(0,"orphan byte %02x received",pData[n]);
        }
    }
    _chk_debug(-1,"<clsIGInterface::NtfyData");
}

bool                            clsIGInterface::GetMessage(clsIGMessage& Msg,
        unsigned nTimeout)
{
    _chk_debug(+1,">clsIGInterface::GetMessage");
    bool    bRes = false;

    bRes = _UnassignedMessages.Get(Msg,nTimeout);

    _chk_debug(-1,"<clsIGInterface::GetMessage -> %s",
               bRes ? (LPCTSTR)Msg.AsString() : "FAIL");
    return(bRes);
}

bool                            clsIGInterface::QueryMultiple(
    const clsIGMessage& MsgQuery, WCValSList<clsIGMessage>& MsgResponseList,
    int nMessages, unsigned nTimeout)
{
    _chk_debug(+1,">clsIGInterface::Query (%s)",(LPCTSTR)MsgQuery.AsString());
    bool    bRes = false;

    clsIGInboundMessageList IML(nMessages);

    // wait until there is no filter of the same message active!
    for (; ; )
    {
        {
            clsCritObj  CO(_CritQueryList);

            if (!_QueryList.contains(MsgQuery))
            {
                _QueryList.insert(MsgQuery,&IML);
                break;
            }
        }
        Sleep(100);
    }

    if (Post(MsgQuery))
    {
        clsIGInboundMessageList*    pMsgList = NULL;

        {
            clsCritObj  CO(_CritQueryList);

            pMsgList = _QueryList[MsgQuery];
        }
        bRes = pMsgList->Get(MsgResponseList,nTimeout);
    }

    {
        clsCritObj  CO(_CritQueryList);

        _QueryList.remove(MsgQuery);
    }

    _chk_debug(-1,"<clsIGInterface::Query -> %s",bRes ? "OK" : "FAIL");
    return(bRes);
}

bool                            clsIGInterface::Query(const clsIGMessage&
        Query, const clsIGMessage& Filter, clsIGMessage& Msg, unsigned nTimeout)
{
    _chk_debug(+1,">clsIGInterface::Query (%s)",(LPCTSTR)Query.AsString());
    bool    bRes = false;

    clsIGInboundMessageList IML;

    // wait until there is no filter of the same message active!
    for (; ; )
    {
        {
            clsCritObj  CO(_CritQueryList);

            if (!_QueryList.contains(Filter))
            {
                _QueryList.insert(Filter,&IML);
                break;
            }
        }
        Sleep(100);
    }

    if (Post(Query))
    {
        clsIGInboundMessageList*    pMsgList = NULL;

        {
            clsCritObj  CO(_CritQueryList);

            pMsgList = _QueryList[Filter];
        }

        bRes = pMsgList->Get(Msg,nTimeout);
    }

    {
        clsCritObj  CO(_CritQueryList);

        _QueryList.remove(Filter);
    }

    _chk_debug(-1,"<clsIGInterface::Query -> %s",
               bRes ? (LPCTSTR)Msg.AsString() : "FAIL");
    return(bRes);
}

bool                            clsIGInterface::Post(const clsIGMessage& Msg)
{
    //printf("%s\n",(LPCTSTR)Msg.AsString());
    _chk_debug(+1,">clsIGInterface::Post (%s)",(LPCTSTR)Msg.AsString());
    bool                        bOk = false;
    WCValSList<clsDataPacket*>  PacketList;

    if (Msg.GetPacketsForCOMM(PacketList))
    {
        // _chk_debug(0,"packets OK: %d entries",PacketList.entries());
        bOk = true;
        while (!PacketList.isEmpty())
        {
            clsDataPacket*  pPacket = PacketList.get();
            BYTE            arData[MAX_MSG_SIZE];
            UINT            nDataSize = 1 + pPacket->nSize() + 1;
            // _chk_debug(0," ...%d entries, current=%d bytes",PacketList.entries(),nDataSize);

            arData[0] = 0x31;
            memcpy(&arData[1],pPacket->pData(),pPacket->nSize());
            arData[nDataSize-1] = 0x32;

            delete pPacket;

            // _chk_utildebug(">CS begin");
            clsCritObj  CO(_Crit);

            try
            {
                if (_pIO != NULL)
                {
                    // _chk_utildebug(">IOWrite");
                    bOk &= _pIO->IOWrite(arData,nDataSize);
                    // _chk_utildebug("<IOWrite");
                }
            }
            catch(clsNotifyExcept& e)
            {
                e.Push("clsIGInterface::Post(%s)",(LPCTSTR)Msg.AsString());
                _chk_debug(-1,"<clsIGInterface::Post -> NOT OK");
                throw e;
            }
        }
    }
    _chk_debug(-1,"<clsIGInterface::Post -> %d",bOk);
    return(bOk);
}

String                          clsIGInterface::sDevice()
{
    if (_pIO != NULL)
    {
        return(_pIO->sDevice());
    }
    return("NULL");
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

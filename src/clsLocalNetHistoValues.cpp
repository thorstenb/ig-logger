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

#include <gpfprot.hpp>

#include "clsWatchdog.h"

bool                            clsLocalNet::QueryHistoricalData(
    const clsDate& Date, clsDeviceDataList* pDataList)
{
    bool                        bRes(false);
    clsResult<bool>             Result(
        String::Format("clsLocalNet::clsLocalNetQueryHistoricalData",Date.Year(),
                       Date.Month(),Date.Day()),bRes);

    clsIGMessage                MsgQuery(   IGID_PC,
                                            IGID_DATALOGGER,
                                            clsIGMessage::IGCMD_GET_HISTORICALDATA,
                                            3,
                                            Date.Year()-2000,
                                            Date.Month(),
                                            Date.Day());
    clsIGMessage                MsgData(    IGID_DATALOGGER,
                                            IGID_PC,
                                            clsIGMessage::IGCMD_DATA_HISTORICALDATA);
    clsIGMessage                MsgResponse;
    bool                        bFinished(false);
    unsigned                    nSizeToRead(0);
    BYTE*                       pData(NULL);
    unsigned                    nData(0);
    unsigned                    nExpectedBlockID(0);

    Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

    bRes = Query(MsgQuery,MsgData,MsgResponse);

    while (bRes && !bFinished)
    {
        if (bUserAbortRequested())
        {
            delete[] pData;
            return(false);
        }

        // printf("Response %s\n",(LPCTSTR)MsgResponse.AsString());
        const BYTE* pPayload = MsgResponse.Payload();
        unsigned    nID = GetValueFromPayloadBE(1,pPayload);
        unsigned    nUnk16 = 0;
        unsigned    nBlockID = 0;
        unsigned    nBlockCount = 0;

        switch (nID)
        {
            case 0x80: // size follows
                if (pData != NULL)
                {
                    // must be the first block
                    delete[] pData;
                    printf("ERR: multiple 'begin' blocks are unexpected\n");
                    return(false);
                }
                nSizeToRead = GetValueFromPayloadBE(4,pPayload);
                if (nSizeToRead > 0x100000)
                {
                    // I assume no data is that large
                    delete[] pData;
                    printf("ERR: requested buffer size too large (%d bytes)\n",nSizeToRead);
                    return(false);
                }
                pData = new BYTE[nSizeToRead];
                memset(pData,0,nSizeToRead);
                nUnk16 = GetValueFromPayloadBE(2,pPayload);
                // printf("START info: %d bytes to read\n",nSizeToRead);

                MsgData.SetPayload(1,nID);
                bRes = Query(MsgData,MsgResponse);
                break;
            case 0x81: // block follows
                nBlockID = GetValueFromPayloadBE(2,pPayload);
                nBlockCount = GetValueFromPayloadBE(1,pPayload);
                // printf("BLOCK info: %d, consisting of %d blocks\n",nBlockID,nBlockCount);
                if (nBlockID != nExpectedBlockID)
                {
                    delete[] pData;
                    printf("ERR: block numbers not consecutive - try again later\n");
                    return(false);
                }
                if (nBlockCount > 512)
                {
                    delete[] pData;
                    printf("ERR: block count too large (%d bytes)\n",nBlockCount);
                    return(false);
                }
                ++nExpectedBlockID;

                MsgData.SetPayload(3,0x81,LOBYTE(nBlockID),HIBYTE(nBlockID));

                {
                    WCValSList<clsIGMessage>    MsgList;
                    bRes = QueryMultiple(MsgData,MsgList,nBlockCount+1);
                    if (bRes)
                    {
                        MsgResponse = MsgList.get(nBlockCount);
                        // which has an ID of 82, which means 'end of block'
                    }

                    while (!MsgList.isEmpty() &&
                            nData < nSizeToRead)
                    {
                        clsIGMessage    Msg(MsgList.get());
                        INT             nSize = (INT)Msg.PayloadSize() - 1;

                        if (nSize <= 0)
                        {
                            delete[] pData;
                            printf("ERR: data too small (%d) - internal inconsistency",nSize);
                            return(false);
                        }

                        if (nData + nSize > nSizeToRead)
                        {
                            delete[] pData;
                            printf("ERR: data too large - internal inconsistency (%d-%d)",nData + nSize,
                                   nSizeToRead);
                            return(false);
                        }

                        memcpy(pData+nData,Msg.Payload()+1,min(nSize,nSizeToRead-nData));
                        nData += nSize;
                    }
                    printf(" %3d %% complete\n",MulDiv(nData,100,nSizeToRead));
                }
                break;
            case 0x82: // end of block
            {
                // printf("BLOCK END info\n");
                clsIGMessage    MsgResponseOK(MsgResponse);

                MsgResponseOK.AppendPayload(1,0xff); // OK
                bRes = Query(MsgResponseOK,MsgResponse);
            }
            __Watchdog.Reset();
            break;
            case 0x83: // end of transmission
            {
                UINT16  nChecksum = (UINT16)GetValueFromPayloadBE(2,pPayload);

                clsIGMessage    MsgAck( IGID_PC,
                                        IGID_DATALOGGER,
                                        clsIGMessage::IGCMD_ACK_HISTORICALDATA);

                bRes = Query(MsgAck,MsgResponse);
                bFinished = true;
                break;
            }
            default:
                printf("ERR: unknown block type 0x%02x\n",nID);
                delete[] pData;
                bFinished = true;
                return(false);
        }
    }

    //  clsFileRW("histo.dat").Write(pData,nData);

    if (nData != nSizeToRead)
    {
        delete[] pData;
        printf("ERR: read too few bytes\n");
        return(false);
    }

    if (bFinished)
    {
        const BYTE* pCur = pData;
        unsigned    nUnk = GetValueFromPayloadLE(2,pCur); // 0800
        unsigned    nBlocksize = GetValueFromPayloadLE(2,pCur); // 0108
        // printf("nBlocksize=0x%x\n",nBlocksize);
        for (int nBlock = 1; nBlock * nBlocksize < nData; ++nBlock)
        {
            __Watchdog.Reset();
            const BYTE* pDataBase = pData + nBlock * nBlocksize;

            pCur = pDataBase;
            unsigned    nInfo = GetValueFromPayloadLE(2,pCur);
            // printf(" %#p: info %04x\n",pCur - pData,nInfo);

            switch (nInfo)
            {
                // default:
                //  printf("unknown info type %x\n",nInfo);
                case 0x004f:
                    // only 0x0f/0x05
                    continue;
                default:
                case 0x0100:
                case 0x0105:
                case 0x0101:
                case 0x0000:
                {
                    unsigned    nYear = GetValueFromPayloadLE(1,pCur) + 2000;
                    unsigned    nMonth = GetValueFromPayloadLE(1,pCur);
                    unsigned    nDay = GetValueFromPayloadLE(1,pCur);
                    while (true)
                    {
                        const BYTE* pCur2 = pCur;
                        BYTE        nLength = (BYTE)GetValueFromPayloadLE(1,pCur2);
                        if (nLength == 0xff)
                            break;

                        pCur += nLength;

                        enLocalNetID            nSourceID = (enLocalNetID)GetValueFromPayloadLE(1,
                                                            pCur2);
                        enLocalNetType          nSourceType = (enLocalNetType)GetValueFromPayloadLE(1,
                                                              pCur2);
                        clsLocalNetID           Device(nSourceID,nSourceType);
                        unsigned                nHour = GetValueFromPayloadLE(1,pCur2);
                        unsigned                nMinute = GetValueFromPayloadLE(1,pCur2);
                        clsSYSTEMTIME           Time(nYear, nMonth, nDay, nHour, nMinute);

                        if (Time.bValid())
                        {
                            const BYTE*             pData = pCur2;

#if 0
                            String                  sData;

                            while (pCur2 < pCur)
                            {
                                sData.AddStrSeparated(" ",String::Format("%02x",GetValueFromPayloadLE(1,
                                                      pCur2)));
                            }

                            if (IsInverter(Device) ||
                                    IsSensorcard(Device))
                            {
                                printf("   %s %04d-%02d-%02d %02d:%02d: %s\n",
                                       (LPCTSTR)sLocalNetID(Device),
                                       nYear, nMonth, nDay,
                                       nHour, nMinute, (LPCTSTR)sData);
                            }
                            else
                            {
                                printf("   unk(%02x/%02x): %04d-%02d-%02d %02d:%02d: %s\n",
                                       nSourceID, nSourceType,
                                       nYear, nMonth, nDay,
                                       nHour, nMinute, (LPCTSTR)sData);
                            }
#endif

                            pCur2 = pData;

                            unsigned                nType = GetValueFromPayloadLE(1,pCur2);
                            switch (nType)
                            {
                                default:
                                case 0x25:
                                case 0x34:
                                    if (IsInverter(Device))
                                    {
                                        unsigned    nUnk1 = GetValueFromPayloadLE(1,pCur2);
                                        switch (nUnk1)
                                        {
                                            case 0x01: // OK
                                                break;
                                            case 0x0b: // no output
                                                break;
                                        }
                                        unsigned    nDiv = GetValueFromPayloadLE(2,pCur2);
                                        unsigned    nUnk2 = GetValueFromPayloadLE(1,pCur2);
                                        unsigned    nPower = GetValueFromPayloadLE(3,pCur2);
                                        UINT16      nACVoltageTimes10 = (UINT16)GetValueFromPayloadLE(2,pCur2);
                                        UINT16      nDCVoltageTimes10 = (UINT16)GetValueFromPayloadLE(2,pCur2);
                                        UINT16      nDCCurrent = (UINT16)GetValueFromPayloadLE(2,pCur2);
                                        BYTE        nImpedance = (BYTE)GetValueFromPayloadLE(1,pCur2);
                                        BYTE        nUnk3 = (BYTE)GetValueFromPayloadLE(1,pCur2);

                                        double      dPower = (double)nPower / nDiv;
                                        double      dACVoltage = (nACVoltageTimes10 != 0xffff) ? nACVoltageTimes10 /
                                                                 10.0 : 0.0;
                                        double      dACCurrent = (dACVoltage > 0 ? dPower / dACVoltage : 0.0);
                                        double      dDCCurrent = (nDCCurrent != 0xffff) ? nDCCurrent / 100.0 : 0.0;
                                        double      dDCVoltage = (nDCVoltageTimes10 != 0xffff) ? nDCVoltageTimes10 /
                                                                 10.0 : 0.0;
                                        double      dImpedance = (nImpedance != 0xff) ? nImpedance / 100.0 : 0.0;
                                        // printf("     P=%.1fW --- AC=%.1fV, %.2fA --- DC=%.1fV, %.2fA--- R0=%.2f\n",dPower,dACVoltage,dACCurrent,dDCVoltage,dDCCurrent,dImpedance);

                                        if (pDataList)
                                        {
                                            clsDataItemList Data;

                                            Data.insert("State",    clsDataItem(dPower > 0      ? clsVariant(
                                                                                    2)           : clsVariant::vNULL(),""));
                                            Data.insert("Power",    clsDataItem(dPower > 0      ? clsVariant(
                                                                                    dPower)      : clsVariant::vNULL(),"W"));
                                            Data.insert("ACVoltage",
                                                        clsDataItem(dACVoltage > 0  ? clsVariant(dACVoltage)  : clsVariant::vNULL(),
                                                                    "V"));
                                            Data.insert("DCVoltage",
                                                        clsDataItem(dDCVoltage > 0  ? clsVariant(dDCVoltage)  : clsVariant::vNULL(),
                                                                    "V"));
                                            Data.insert("ACCurrent",
                                                        clsDataItem(dACCurrent > 0  ? clsVariant(dACCurrent)  : clsVariant::vNULL(),
                                                                    "A"));
                                            Data.insert("DCCurrent",
                                                        clsDataItem(dDCCurrent > 0  ? clsVariant(dDCCurrent)  : clsVariant::vNULL(),
                                                                    "A"));
                                            Data.insert("Impedance",
                                                        clsDataItem(dImpedance > 0  ? clsVariant(dImpedance)  : clsVariant::vNULL(),
                                                                    "Ohm"));

                                            pDataList->Add(Device,Time,Data);
                                        }
                                    }
                                    if (IsSensorcard(Device))
                                    {
                                        unsigned    nUnk1 = GetValueFromPayloadLE(1,pCur2);
                                        switch (nUnk1)
                                        {
                                            case 0x01: // OK
                                                break;
                                            case 0x0b: // no output
                                                break;
                                        }
                                        unsigned    nUnknown = GetValueFromPayloadLE(2,pCur2);
                                        unsigned    nMask = GetValueFromPayloadLE(1,pCur2);
                                        static      LPCTSTR __pszDescr[] =
                                        {
                                            "Temp1",
                                            "Temp2",
                                            "Irradiation",
                                            "Digi1",
                                            "Digi2",
                                            "Current",
                                        };
                                        static      LPCTSTR __pszUnit[] =
                                        {
                                            "deg",
                                            "deg",
                                            "W/m^2",
                                            "km/h",
                                            "?",
                                            "?",
                                        };

                                        clsDataItemList Data;

                                        for (int nValue = 0; nValue < 6; ++nValue)
                                        {
                                            // printf("MASK=%x\n",nMask);
                                            if (nMask & (1 << nValue))
                                            {
                                                clsIGValue  v(1 << nValue,4,__pszDescr[nValue],pCur2);

                                                // printf(" %s=%s\n",(LPCTSTR)v.pszName(),(LPCTSTR)v.sValueEx());
                                                if (pDataList)
                                                {
                                                    Data.insert(v.pszName(),
                                                                clsDataItem(clsVariant(v.dValueEx()),__pszUnit[nValue]));

                                                }
                                            }
                                        }
                                        if (pDataList)
                                        {
                                            pDataList->Add(Device,Time,Data);
                                        }
                                    }
                                    break;
                                case 0x45: // error or such?
                                {
                                    unsigned    nUnk1 = GetValueFromPayloadLE(1,pCur2);
                                    unsigned    nUnk2 = GetValueFromPayloadLE(1,pCur2);
                                    UINT16      nDCVoltageTimes10 = (UINT16)GetValueFromPayloadLE(2,pCur2);
                                    double      dDCVoltage = (nDCVoltageTimes10 != 0xffff) ? nDCVoltageTimes10 /
                                                             10.0 : 0.0;

                                    switch (nUnk1)
                                    {
                                        case 0x82: // OK
                                            switch (nUnk2)
                                            {
                                                case 0x13: // DCVoltage
                                                case 0x23: // DCVoltage
                                                    if (pDataList)
                                                    {
                                                        clsDataItemList Data;

                                                        Data.insert("State",    clsDataItem(clsVariant::vNULL(),""));
                                                        Data.insert("DCVoltage",
                                                                    clsDataItem(dDCVoltage > 0  ? clsVariant(dDCVoltage)  : clsVariant::vNULL(),
                                                                                "V"));

                                                        pDataList->Add(Device,Time,Data);
                                                    }
                                                    break;
                                            }
                                            break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    delete[] pData;

    pDataList = pDataList;
    return(bRes = true);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

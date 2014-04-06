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

double _round(double d)
{
    return(floor(d + 0.5));
}

String  GetStringFromPayload(const BYTE*& pPayload, bool bAdvancePointer=true)
{
    String  s((const char*)pPayload,1,pPayload[0]);

    if (bAdvancePointer)
    {
        pPayload += 1 + pPayload[0];
    }
    return(s);
}

class clsIGState
    : public clsIGValue
{
private:
    struct scIGState
    {
        /*
        C3 00 13 01 32 03 // Power Low
        C3 00 15 01 32 03 // Power Low 21 s
        43 00 1E 00 00 03 // Test 30 s
        82 00 00 01 32 03 // Sync AC
        44 00 00 00 00 03 // Startup
        45 00 10 00 00 03 // Sync ENS
        06 00 00 00 00 03 // ???
        0e 00 00 00 00 03 // ???
        8B 00 00 01 A9 03 // Wait PS, State 425
        C3 00 1C 01 32 03
        C3 00 14 01 33 03 // ""
        C3 00 14 01 33 03
        C3 00 0F 01 33 03
        */
#pragma pack(push,1)
        scDataBYTE                      _nSubstate;
        scDataUINT                      _nWaitTime;
        scDataUINT                      _nStateCode;
        scDataBYTE                      _nRunState;
#pragma pack(pop)
    };

    String                          sValue(const BYTE*& pPayload,
                                           bool bWithCountdown) const
    {
        String              sRes;
        const scIGState*    pState = reinterpret_cast<const scIGState*>(pPayload);

        switch (pState->_nRunState.nValue())
        {
            case 0x02:
                sRes = "running";
                break;
            case 0x03:
                switch (pState->_nSubstate.nValue() & 0x7f)
                {
                    case 0x02:
                        sRes = "Sync AC";
                        break;
                    case 0x0B:
                        sRes = "Wait PS";
                        break;
                    case 0x06:
                        sRes = String::Format("unknown substate 0x%02x - even IG.Access does not show a text for it ;-)",
                                              pState->_nSubstate.nValue() & 0x7f);
                        break;
                    case 0x43:
                        sRes = "Power Low";
                        break;
                    case 0x44:
                        sRes = "Startup";
                        break;
                    case 0x45:
                        sRes = "Sync ENS";
                        break;
                    case 0x01:
                    default:
                        sRes = String::Format("unknown substate 0x%02x",
                                              pState->_nSubstate.nValue() & 0x7f);
                        break;
                    case 0x00:
                        break;
                }
                if (pState->_nSubstate.nValue() & 0x80)
                {
                    sRes += String::Format(" (%s)",
                                           (LPCTSTR)clsLocalNet::GetStateDescription(pState->_nStateCode.nValue()));
                }
                if (bWithCountdown &&
                        pState->_nWaitTime.nValue() != 0)
                {
                    sRes += String::Format(" (%d sec)",pState->_nWaitTime.nValue());
                }
                break;
            default:
                sRes = String::Format("unknown runstate 0x%x",pState->_nRunState.nValue());
                break;
        }

        return(sRes);
    }
public:
    clsIGState(unsigned nMask, unsigned nBytes, const char* pszName)
        : clsIGValue(nMask, nBytes, pszName)
    {
    }

    unsigned                        nState(const BYTE*& pPayload)
    {
        const scIGState*    pState = reinterpret_cast<const scIGState*>(pPayload);

        return(pState->_nRunState.nValue());
    }
    String                          sState(const BYTE*& pPayload) const
    {
        return(sValue(pPayload,true));
    }
    virtual double                  dValue(const BYTE* pPayload) const
    {
        const scIGState*    pState = reinterpret_cast<const scIGState*>(pPayload);

        return(pState->_nRunState.nValue());
    }
    virtual String                  sValue(const BYTE*& pPayload) const
    {
        return(sValue(pPayload,false));
    }
};

/*
static const char* pszDisplayType(clsLocalNet::enDisplaySelection nDisplaySelection)
{
    switch (nDisplaySelection)
        {
        case clsLocalNet::DATA_TEST:
            return("TEST");
        case clsLocalNet::DATA_EXTENDED:
            return("EXT");
        case clsLocalNet::DATA_STATE:
            return("STATE");
        case clsLocalNet::DATA_CURRENT:
            return("CUR");
        case clsLocalNet::DATA_DAY:
            return("DAY");
        case clsLocalNet::DATA_YEAR:
            return("YEAR");
        case clsLocalNet::DATA_TOTAL:
            return("TOTAL");
        }
    return("?");
}
*/

static  clsIGState  __State             (0x00000001,6,
        "State");                  // 07 00 00 00 00 02
static  clsIGValue  __CurPower          (0x00000002,4,
        "Power");                  // 04 58 0e 84
static  clsIGValue  __CurACCurrent      (0x00000004,4,
        "ACCurrent");              // 01 f7 10 82
static  clsIGValue  __CurACVoltage      (0x00000008,4,
        "ACVoltage");              // 00 dd 11 84
static  clsIGValue  __CurACFrequency    (0x00000010,4,
        "ACFrequency");            // 13 85 13 82
static  clsIGValue  __CurDCCurrent      (0x00000020,4,
        "DCCurrent");              // 01 e8 10 82
static  clsIGValue  __CurDCVoltage      (0x00000040,4,
        "DCVoltage");              // 00 f2 11 84
static  clsIGValue  __CurImpedance      (0x00000080,3,
        "Impedance");              // 31 12 82        .49
static  clsIGValue  __CurIsolation      (0x00000100,3,
        "Isolation");              // 64 12 8f        >10 MOhm
static  clsIGValue  __DayEnergy         (0x00000200,4,
        "Day.Energy");             // 00 02 01 07
static  clsIGValue  __DayUnk            (0x00000400,4,
        "Day.Unknown");            // 00 01 1a 04
static  clsIGValue  __DayACPowerMax     (0x00000800,4,
        "Day.ACPowerMax");         // 04 5e 0e 84
static  clsIGValue  __DayACVoltageMax   (0x00001000,4,
        "Day.ACVoltageMax");       // 00 e3 11 84
static  clsIGValue  __DayACVoltageMin   (0x00002000,4,
        "Day.ACVoltageMin");       // 00 db 11 84
static  clsIGValue  __DayDCVoltage      (0x00004000,4,
        "Day.DCVoltageMax");       // 01 0a 11 84
static  clsIGValue  __DayRuntime        (0x00008000,4,
        "Day.Runtime");            // 00 e7 21 84
static  clsIGValue  __YearEnergy        (0x00010000,6,
        "Year.Energy");            // 00 00 09 ab 01 07
static  clsIGValue  __YearUnk           (0x00020000,6,
        "Year.Unknown");           // 00 00 05 51 1a 04
static  clsIGValue  __YearACPowerMax    (0x00040000,4,
        "Year.ACPowerMax");        // 10 ee 0e 84
static  clsIGValue  __YearACVoltageMax  (0x00080000,4,
        "Year.ACVoltageMax");      // 00 e7 11 84
static  clsIGValue  __YearACVoltageMin  (0x00100000,4,
        "Year.ACVoltageMin");      // 00 10 11 84
static  clsIGValue  __YearDCVoltage     (0x00200000,4,
        "Year.DCVoltageMax");      // 01 30 11 84
static  clsIGValue  __YearRuntime       (0x00400000,6,
        "Year.Runtime");           // 00 02 3a 4d 21 84
static  clsIGValue  __TotalEnergy       (0x00800000,6,
        "Total.Energy");           // 00 00 21 b2 01 07
static  clsIGValue  __TotalUnk          (0x01000000,6,
        "Total.Unknown");          // 00 00 12 88 1a 04
static  clsIGValue  __TotalACPowerMax   (0x02000000,4,
        "Total.ACPowerMax");       // 10 f0 0e 84
static  clsIGValue  __TotalACVoltageMax (0x04000000,4,
        "Total.ACVoltageMax");     // 00 e9 11 84
static  clsIGValue  __TotalACVoltageMin (0x08000000,4,
        "Total.ACVoltageMin");     // 00 10 11 84
static  clsIGValue  __TotalDCVoltage    (0x10000000,4,
        "Total.DCVoltageMax");     // 01 38 11 84
static  clsIGValue  __TotalRuntime      (0x20000000,6,
        "Total.Runtime");          // 00 08 42 43 21 84

static  clsIGValue* __ValueList[] =
{
    &__State,
    &__CurPower,
    &__CurACCurrent,
    &__CurACVoltage,
    &__CurACFrequency,
    &__CurDCCurrent,
    &__CurDCVoltage,
    &__CurImpedance,
    &__CurIsolation,
    &__DayEnergy,
    &__DayUnk,
    &__DayACPowerMax,
    &__DayACVoltageMax,
    &__DayACVoltageMin,
    &__DayDCVoltage,
    &__DayRuntime,
    &__YearEnergy,
    &__YearUnk,
    &__YearACPowerMax,
    &__YearACVoltageMax,
    &__YearACVoltageMin,
    &__YearDCVoltage,
    &__YearRuntime,
    &__TotalEnergy,
    &__TotalUnk,
    &__TotalACPowerMax,
    &__TotalACVoltageMax,
    &__TotalDCVoltage,
    &__TotalRuntime,
};

/*
Current:
> 31 10 34 32 00 F9 32
< 31 10 34 32 00 01 00 15 14 84 A0 32

> 31 10 34 32 01 90 32
< 31 10 34 32 01 01 00 14 14 84 A0 32

> 31 10 34 32 02 2B 32
< 31 10 34 32 02 01 00 00 1B 44 6B 32

> 31 10 34 32 03 42 32
< 31 10 34 32 03 00 42 32

> 31 10 34 32 04 34 32
< 31 10 34 32 04 00 34 32

Day:
> 31 10 34 33 00 90 32
< 31 10 34 33 80 14 00 00 00 15 00 00 00 1B 1B C3 32
< 31 10 34 33 00 22 00 00 00 22 E2 32

> 31 10 34 33 01 F9 32
< 31 10 34 33 81 14 00 00 00 14 00 14 00 1C 9E 32
< 31 10 34 33 91 00 00 00 20 00 00 00 20 8B 32

> 31 10 34 33 02 42 32
< 31 10 34 33 82 00 00 1B 1B 44 00 00 1B 1B 44 04 1B 1B 32
< 31 10 34 33 92 8E 1B 1B 44 04 8E 1B 1B 44 FD 32

> 31 10 34 33 03 2B 32
< 31 10 34 33 03 00 00 01 04 00 00 01 04 00 2B 32
< 31 10 34 33 13 00 01 04 00 00 01 04 CD 32

> 31 10 34 33 04 5D 32
< 31 10 34 33 04 00 00 01 04 00 00 01 04 00 5D 32
< 31 10 34 33 14

> 31 10 34 33 05 34 32
< 31 10 34 33 05 00 00 03 00 00 00 03 00 00 34 32
< 31 10 34 33 15 00 03 00 00 00 03 04 1F 32

*/

bool                            clsLocalNet::QueryData(const clsLocalNetID&
        Device, enDisplaySelection nSelection, clsDataItemList* pData, String* psValue,
        String* psStateErrortext)
{
    bool                    bRes(false);
    clsResult<bool>         Result("clsLocalNet::QueryInverterDisplay",bRes);

    for (int nTry = 0; nTry < 3 && !bRes && !bUserAbortRequested(); ++nTry)
    {
        clsIGMessage    MsgQuery(   IGID_PC,
                                    Device.nID(),
                                    clsIGMessage::IGCMD_INV_QUERYDISPLAY_CURRENT);
        clsIGMessage    MsgResponse;
        unsigned        nMask = 0;

        switch (nSelection)
        {
            case DATA_DISPLAY_VALUES:
                nMask |= __State.nMask();
                nMask |= __CurPower.nMask();
                nMask |= __DayEnergy.nMask();
                nMask |= __CurACVoltage.nMask();
                nMask |= __CurACFrequency.nMask();
                break;
            case DATA_STATE:
                nMask |= __State.nMask();
                break;
            case DATA_CURRENT:
                for (int nValue = 0; nValue < sizeof(__ValueList)/sizeof(__ValueList[0]);
                        ++nValue)
                {
                    nMask |= __ValueList[nValue]->nMask();
                }
                break;
        }
        MsgQuery.SetPayload(5,0x00,(BYTE)(nMask >> 24),(BYTE)(nMask >> 16),
                            (BYTE)(nMask >> 8),(BYTE)(nMask >> 0));

        Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

        if (Query(MsgQuery,MsgResponse))
        {
            if (MsgResponse.PayloadSize() >= MsgQuery.PayloadSize())
            {
                bRes = true;

                String          sRes;
                const BYTE*     pPayload = MsgResponse.Payload();
                unsigned        nFieldMask = (pPayload[0] << 24) + (pPayload[1] << 16) +
                                             (pPayload[2] << 8) + (pPayload[3] << 0);
                clsDataItemList Data;

                pPayload += 4;

                for (int nValue = 0; nValue < sizeof(__ValueList)/sizeof(__ValueList[0]);
                        ++nValue)
                {
                    if (nFieldMask & __ValueList[nValue]->nMask())
                    {
                        // fprintf(stderr,"  %s: %s\n",__ValueList[nValue]->pszName(),(LPCTSTR)__ValueList[nValue]->sValue(pPayload));
                        if (__ValueList[nValue] == &__State)
                        {
                            if (psStateErrortext != NULL)
                            {
                                *psStateErrortext = __State.sState(pPayload);
                            }
                        }

                        _BroadcastServer.Send(String::Format("%s %s:%s=%s",
                                                             (LPCTSTR)clsSYSTEMTIME(true).sTimeHMS(),
                                                             (LPCTSTR)sLocalNetID(Device),
                                                             (LPCTSTR)__ValueList[nValue]->pszName(),
                                                             (LPCTSTR)__ValueList[nValue]->sValue(pPayload).LTrim()));

                        if (nSelection == DATA_DISPLAY_VALUES)
                        {
                            if (psValue)
                            {
                                if (__ValueList[nValue] == &__State)
                                {
                                    if (__State.nState(pPayload) != 2)
                                    {
                                        psValue->AddStrSeparated(", ",__State.sState(pPayload));
                                    }
                                    else
                                    {
                                        psValue->AddStrSeparated(", ","running");
                                    }
                                }
                                else
                                {
                                    psValue->AddStrSeparated(", ",__ValueList[nValue]->sValue(pPayload));
                                }
                            }
                        }
                        else
                        {
                            if (pData != NULL)
                            {
                                if (__ValueList[nValue]->nUnit(pPayload) != UNIT_UNKNOWN)
                                {
                                    clsDataItem Item(__ValueList[nValue]->dValue(pPayload),
                                                     pszUnit(__ValueList[nValue]->nUnit(pPayload)));

                                    pData->insert(__ValueList[nValue]->pszName(),Item);
                                }
                            }
                        }
                        pPayload += __ValueList[nValue]->nBytes();
                    }
                }
            }
        }
    }

    return(bRes); // ignore COM errors
}

static  clsIGValue  __CurTempModules    (0x00000000,4,"Temp1");
static  clsIGValue  __CurTempAmbient    (0x00000001,4,"Temp2");
static  clsIGValue  __CurIrradiation    (0x00000002,4,"Irradiation");
static  clsIGValue  __CurDigi1          (0x00000003,4,"Digi1");
static  clsIGValue  __CurDigi2          (0x00000004,4,"Digi2");
static  clsIGValue  __CurCurrent        (0x00000005,4,"Current");

static  clsIGValue* __SensorList[] =
{
    &__CurTempModules,
    &__CurTempAmbient,
    &__CurIrradiation,
    &__CurDigi1,
    &__CurDigi2,
    &__CurCurrent
};

bool                            clsLocalNet::QuerySensor(
    const clsLocalNetID& Device, clsDataItemList* pData, String* psValue)
{
    bool                    bRes(false);
    clsResult<bool>         Result("clsLocalNet::QuerySensor",bRes);

    for (int nTry = 0; nTry < 3 && !bRes && !bUserAbortRequested(); ++nTry)
    {
        for (int nSensor = 0; nSensor < 5; ++nSensor)
        {
            clsIGMessage    MsgQuery(   IGID_PC,
                                        Device.nID(),
                                        clsIGMessage::IGCMD_SNS_QUERYDISPLAY_CURRENT);
            clsIGMessage    MsgResponse;

            MsgQuery.SetPayload(1,nSensor);

            Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

            if (Query(MsgQuery,MsgResponse))
            {
                Log("<%-8s: %s","query", (LPCTSTR)MsgResponse.AsString());
                if (MsgResponse.PayloadSize() > MsgQuery.PayloadSize())
                {
                    bRes = true;

                    String          sRes;
                    const BYTE*     pPayload = MsgResponse.Payload();
                    unsigned        nSensorFromResponse = GetValueFromPayloadBE(1,pPayload);
                    if (nSensorFromResponse != nSensor)
                    {
                        _chk_utildebug("bad packet: sensor numbers do not match!");
                        continue;
                    }

                    bool            bSensorPresent = (GetValueFromPayloadBE(1,pPayload) != 0);

                    if (bSensorPresent)
                    {
                        for (int nValue = 0; nValue < sizeof(__SensorList)/sizeof(__SensorList[0]);
                                ++nValue)
                        {
                            if (__SensorList[nValue]->nMask() == nSensor)
                            {
                                // printf("%s: %s\n",(LPCTSTR)__SensorList[nValue]->pszName(),(LPCTSTR)__SensorList[nValue]->sValue(pPayload));
                                _BroadcastServer.Send(String::Format("%s %s:%s=%s",
                                                                     (LPCTSTR)clsSYSTEMTIME(true).sTimeHMS(),
                                                                     (LPCTSTR)sLocalNetID(Device),
                                                                     (LPCTSTR)__SensorList[nValue]->pszName(),
                                                                     (LPCTSTR)__SensorList[nValue]->sValue(pPayload).LTrim()));
                                if (psValue)
                                {
                                    psValue->AddStrSeparated(", ",__SensorList[nValue]->sValue(pPayload));
                                }
                                if (pData != NULL)
                                {
                                    if (__SensorList[nValue]->nUnit(pPayload) != UNIT_UNKNOWN)
                                    {
                                        clsDataItem Item(__SensorList[nValue]->dValue(pPayload),
                                                         pszUnit(__SensorList[nValue]->nUnit(pPayload)));

                                        pData->insert(__SensorList[nValue]->pszName(),Item);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return(bRes); // ignore COM errors
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

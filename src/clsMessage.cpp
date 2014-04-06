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

#include "clsExcept.h"
#include "clsLocalNet.h"

#define DETAILED_OUTPUT 0

String                          sString(const BYTE*& pPayload)
{
    String  s;
    char*   pc = s.LockBuffer(*pPayload+1);
    memcpy(pc,pPayload+1,*pPayload);
    pc[*pPayload] = 0;
    s.ReleaseBuffer();
    pPayload += *pPayload + 1;
    return(s);
}

//////////////////////////////////////////////////////////////////////////////

const BYTE __XORTable[256] =
{
    0x00,0x69,0xD2,0xBB,0xCD,0xA4,0x1F,0x76,
    0xF3,0x9A,0x21,0x48,0x3E,0x57,0xEC,0x85,
    0xE6,0x8F,0x34,0x5D,0x2B,0x42,0xF9,0x90,
    0x15,0x7C,0xC7,0xAE,0xD8,0xB1,0x0A,0x63,
    0xA5,0xCC,0x77,0x1E,0x68,0x01,0xBA,0xD3,
    0x56,0x3F,0x84,0xED,0x9B,0xF2,0x49,0x20,
    0x43,0x2A,0x91,0xF8,0x8E,0xE7,0x5C,0x35,
    0xB0,0xD9,0x62,0x0B,0x7D,0x14,0xAF,0xC6,
    0x4A,0x23,0x98,0xF1,0x87,0xEE,0x55,0x3C,
    0xB9,0xD0,0x6B,0x02,0x74,0x1D,0xA6,0xCF,
    0xAC,0xC5,0x7E,0x17,0x61,0x08,0xB3,0xDA,
    0x5F,0x36,0x8D,0xE4,0x92,0xFB,0x40,0x29,
    0xEF,0x86,0x3D,0x54,0x22,0x4B,0xF0,0x99,
    0x1C,0x75,0xCE,0xA7,0xD1,0xB8,0x03,0x6A,
    0x09,0x60,0xDB,0xB2,0xC4,0xAD,0x16,0x7F,
    0xFA,0x93,0x28,0x41,0x37,0x5E,0xE5,0x8C,
    0x94,0xFD,0x46,0x2F,0x59,0x30,0x8B,0xE2,
    0x67,0x0E,0xB5,0xDC,0xAA,0xC3,0x78,0x11,
    0x72,0x1B,0xA0,0xC9,0xBF,0xD6,0x6D,0x04,
    0x81,0xE8,0x53,0x3A,0x4C,0x25,0x9E,0xF7,
    0x31,0x58,0xE3,0x8A,0xFC,0x95,0x2E,0x47,
    0xC2,0xAB,0x10,0x79,0x0F,0x66,0xDD,0xB4,
    0xD7,0xBE,0x05,0x6C,0x1A,0x73,0xC8,0xA1,
    0x24,0x4D,0xF6,0x9F,0xE9,0x80,0x3B,0x52,
    0xDE,0xB7,0x0C,0x65,0x13,0x7A,0xC1,0xA8,
    0x2D,0x44,0xFF,0x96,0xE0,0x89,0x32,0x5B,
    0x38,0x51,0xEA,0x83,0xF5,0x9C,0x27,0x4E,
    0xCB,0xA2,0x19,0x70,0x06,0x6F,0xD4,0xBD,
    0x7B,0x12,0xA9,0xC0,0xB6,0xDF,0x64,0x0D,
    0x88,0xE1,0x5A,0x33,0x45,0x2C,0x97,0xFE,
    0x9D,0xF4,0x4F,0x26,0x50,0x39,0x82,0xEB,
    0x6E,0x07,0xBC,0xD5,0xA3,0xCA,0x71,0x18,
};

BYTE crc(const BYTE* pData, unsigned nBytes)
{
    BYTE    nRes = 0;

    for (int i = 0; i < nBytes; ++i)
    {
        nRes ^= __XORTable[pData[i]];
        // printf("i=%d, data=%02x -> %02x\n",i,pData[i],nRes);
    }
    return(nRes);
}

//////////////////////////////////////////////////////////////////////////////

clsDataPacket::clsDataPacket(BYTE* pHeader, UINT nPayloadSize, BYTE* pPayload)
    : _nSize(0)
    , _pData(new BYTE[2*(3+nPayloadSize+1)])
{
    ASSERT(nPayloadSize <= 10);

    // temp buffer
    BYTE    arData[13];

    memcpy(arData,pHeader,3);
    memcpy(arData+3,pPayload,nPayloadSize);

    // now escape the contents
    BYTE*   pDataCur = _pData;

    for (unsigned n = 0; n <= 3+nPayloadSize; ++n)
    {
        BYTE    c;

        if (n < 3+nPayloadSize)
        {
            c = arData[n];
        }
        else
        {
            c = crc(arData,3+nPayloadSize);
        }

        switch (c)
        {
            case 0x1b:
                *pDataCur++ = 0x1b;
                *pDataCur++ = 0x1b;
                break;
            case 0x31:
                *pDataCur++ = 0x1b;
                *pDataCur++ = 0x1c;
                break;
            case 0x32:
                *pDataCur++ = 0x1b;
                *pDataCur++ = 0x1d;
                break;
            default:
                *pDataCur++ = c;
                break;
        }
    }
    _nSize = pDataCur - _pData;
}

clsDataPacket::~clsDataPacket()
{
    delete[] _pData;
    _pData = NULL;
}

//////////////////////////////////////////////////////////////////////////////

clsIGMessage::clsIGMessage()
    : _pData(NULL)
    , _nDataSize(0)
    , _bChecksumOk(false)
    , _bComplete(true)
    , _nExpectedBlockNumber(0)
{
}

clsIGMessage::clsIGMessage(const clsIGMessage& rhs)
    : _pData(new BYTE[rhs._nDataSize])
    , _nDataSize(rhs._nDataSize)
    , _bChecksumOk(rhs._bChecksumOk)
    , _bComplete(rhs._bComplete)
    , _nExpectedBlockNumber(rhs._nExpectedBlockNumber)
{
    memcpy(_pData,rhs._pData,_nDataSize);
}

clsIGMessage::clsIGMessage(const BYTE* pData, unsigned nDataSize)
    : _pData(new BYTE[nDataSize])
    , _bComplete(true)
    , _nExpectedBlockNumber(0)
{
    BYTE*   pCurData = _pData;

    for (unsigned n = 0; n < nDataSize; ++n)
    {
        if (pData[n] == 0x1b) // ESCAPE
        {
            switch (pData[++n])
            {
                case 0x1b:
                    *pCurData++ = 0x1b;
                    break;
                case 0x1c:
                    *pCurData++ = 0x31;
                    break;
                case 0x1d:
                    *pCurData++ = 0x32;
                    break;
            }
        }
        else
        {
            *pCurData++ = pData[n];
        }
    }
    _nDataSize = (pCurData - _pData) - 1;
    _bChecksumOk = crc(_pData,_nDataSize) == _pData[_nDataSize];

    if (!_bChecksumOk)
    {
        _chk_utildebug("bad checksum. Sizes %d %d, want: %02x",_nDataSize,nDataSize,
                       _pData[_nDataSize]);
        for (int i = 0; i < _nDataSize; ++i)
        {
            _chk_utildebug(" is (%2d=%02x) %02x/%02x",i,_pData[i],crc(_pData,i),
                           _pData[_nDataSize]);
        }
    }

    _chk_utildebug("(size %d)",_nDataSize);
    if (PayloadSize() == 10)
    {
        switch (nCmd())
        {
            case IGCMD_DL_GET_OPTION:
                // sigh - I get a repeated value... 05 05 05 05 05 05...
                break;
            default:
                _chk_utildebug("(message block number 0x%02x)",_pData[3]);
                _bComplete = false;
        }
    }
    _nExpectedBlockNumber = _pData[3] + 1;
}

clsIGMessage::clsIGMessage(enLocalNetID nFrom, enLocalNetID nTo, enCmd nCmd,
                           BYTE* pData, unsigned nDataSize)
    : _pData(new BYTE[MaxBufferSize(nDataSize)])
    , _bChecksumOk(false)
    , _bComplete(true)
    , _nExpectedBlockNumber(0)
{
    _pData[0] = (BYTE)nFrom;
    _pData[1] = (BYTE)nTo;
    _pData[2] = (BYTE)nCmd;
    memcpy(_pData+3,pData,nDataSize);
    _nDataSize = 3+nDataSize;
}

clsIGMessage::clsIGMessage(enLocalNetID nFrom, enLocalNetID nTo, enCmd nCmd,
                           unsigned nDataSize, ...)
    : _pData(new BYTE[MaxBufferSize(nDataSize)])
    , _bChecksumOk(false)
    , _bComplete(true)
    , _nExpectedBlockNumber(0)
{
    _pData[0] = (BYTE)nFrom;
    _pData[1] = (BYTE)nTo;
    _pData[2] = (BYTE)nCmd;

    va_list args;

    va_start(args,nDataSize);
    for (int n = 0; n < nDataSize; ++n)
    {
        _pData[3+n] = va_arg(args,BYTE);
    }
    va_end(args);
    _nDataSize = 3+nDataSize;
}

clsIGMessage::~clsIGMessage()
{
    delete[] _pData;
}

clsIGMessage&                   clsIGMessage::operator=(const clsIGMessage&
        rhs)
{
    if (&rhs != this)
    {
        if (rhs._nDataSize > _nDataSize)
        {
            delete[] _pData;
            _pData = new BYTE[rhs._nDataSize];
        }
        _nDataSize = rhs._nDataSize;
        memcpy(_pData,rhs._pData,_nDataSize);
        _bComplete = rhs._bComplete;
        _nExpectedBlockNumber = rhs._nExpectedBlockNumber;
    }
    return(*this);
}

bool                            clsIGMessage::GetPacketsForCOMM(
    WCValSList<clsDataPacket*>& Packets) const
{
    if (_pData)
    {
        if (_nDataSize > 1000)
        {
            throw clsExcept("WRN: wanted to send message with bad size %d",_nDataSize);
        }

        // printf("SEND %d\n",PayloadSize());
        for (INT nOffset = 0; nOffset <= PayloadSize(); )
        {
            UINT    nSizeThis = min(10,PayloadSize()-nOffset);
            BYTE*   pDataThis = _pData+3+nOffset;

            // printf("  nOffset=%d, %d bytes\n",nOffset,nSizeThis);
            Packets.append(new clsDataPacket(_pData,nSizeThis,pDataThis));

            if (nOffset == 0 &&
                    nSizeThis < 10)
            {
                break;
            }

            if (nSizeThis == 0 ||
                    (nSizeThis % 2) == 1)
            {
                break;
            }
            nOffset += nSizeThis;
        }
    }
    return(!Packets.isEmpty());
}

void                            clsIGMessage::SetPayload(unsigned nDataSize,
        ...)
{
    if (3+nDataSize > _nDataSize)
    {
        BYTE*   pNew = new BYTE[3+nDataSize];
        memcpy(pNew,_pData,3);
        delete[] _pData;
        _pData = pNew;
    }
    va_list args;

    va_start(args,nDataSize);
    for (int n = 0; n < nDataSize; ++n)
    {
        _pData[3+n] = va_arg(args,int);
    }
    va_end(args);
    _nDataSize = 3+nDataSize;
}

clsIGMessage&                   clsIGMessage::operator+=
(const clsIGMessage& rhs)
{
    if (rhs.nBlockNumber() != _nExpectedBlockNumber)
    {
        throw clsExcept("ERROR: bad block number 0x%02x (expected: 0x%02x)",
                        rhs.nBlockNumber(),_nExpectedBlockNumber);
    }

    BYTE*   pNew = new BYTE[_nDataSize + rhs.PayloadSize()
                            -1]; // remove block number...
    memcpy(pNew,_pData,_nDataSize);
    delete[] _pData;
    _pData = pNew;

    memcpy(_pData+_nDataSize,rhs.Payload()+1,
           rhs.PayloadSize()-1); // do not copy block number
    _nDataSize += rhs.PayloadSize()-1;
    _chk_utildebug("new block=%02x own block=%02x payload=%d bytes %x %x\n",
                   rhs.nBlockNumber(),
                   nBlockNumber(),
                   rhs.PayloadSize(),
                   (BYTE)(rhs.nBlockNumber() & 0x0f),
                   (BYTE)((nBlockNumber() >> 4) & 0x0f));

    if ((rhs.PayloadSize() % 2) == 1 ||
            (rhs.nBlockNumber() != 0 &&
             (rhs.nBlockNumber() & 0x0f) == ((rhs.nBlockNumber() >> 4) & 0x0f)))
    {
        _chk_utildebug("(message complete)");
        ++_nExpectedBlockNumber;
        _bComplete = true;
        RemoveBlockNumber();
    }
    else
    {
        _chk_utildebug("(message incomplete)");
        ++_nExpectedBlockNumber;
    }
    return(*this);
}

void                            clsIGMessage::RemoveBlockNumber()
{
    // move own payload
    memcpy(_pData+3,_pData+4,PayloadSize()-1);
    --_nDataSize;
}

void                            clsIGMessage::AppendPayload(const BYTE* pData,
        unsigned nDataSize)
{
    BYTE*   pNew = new BYTE[_nDataSize + nDataSize]; // remove block number...
    memcpy(pNew,_pData,_nDataSize);
    delete[] _pData;
    _pData = pNew;

    memcpy(_pData+_nDataSize,pData,nDataSize); // do not copy block number
    _nDataSize += nDataSize;
}

void                            clsIGMessage::AppendPayload(unsigned nDataSize,
        ...)
{
    BYTE*   pNew = new BYTE[_nDataSize + nDataSize]; // remove block number...
    memcpy(pNew,_pData,_nDataSize);
    delete[] _pData;
    _pData = pNew;

    va_list args;

    va_start(args,nDataSize);
    for (int n = 0; n < nDataSize; ++n)
    {
        _pData[_nDataSize+n] = va_arg(args,BYTE);
    }
    va_end(args);
    _nDataSize += nDataSize;
}

void                            clsIGMessage::SetPayload(const BYTE* pData,
        unsigned nDataSize)
{
    if (3+nDataSize > _nDataSize)
    {
        BYTE*   pNew = new BYTE[3+nDataSize];
        memcpy(pNew,_pData,3);
        delete[] _pData;
        _pData = pNew;
    }
    memcpy(_pData+3,pData,nDataSize);
    _nDataSize = 3+nDataSize;
}

enLocalNetID                clsIGMessage::nFrom() const
{
    return(_pData != NULL ? (enLocalNetID)_pData[0] : IGID_INVALID);
}

enLocalNetID                clsIGMessage::nTo() const
{
    return(_pData != NULL ? (enLocalNetID)_pData[1] : IGID_INVALID);
}

clsIGMessage::enCmd             clsIGMessage::nCmd() const
{
    return(_pData != NULL ? (clsIGMessage::enCmd)_pData[2] : IGCMD_DEVICELIST);
}

String                          clsIGMessage::sCmd() const
{
    switch (nCmd())
    {
        case IGCMD_DEV_GETCONFIG:
            return("QRY_DEV_CONFIG");
    }
    if (IsInverter(nTo()))
    {
        switch (nCmd())
        {
            case IGCMD_INV_QUERYDISPLAY_CURRENT:
                return("QRY_DATA_CUR");
            case IGCMD_SNS_QUERYDISPLAY_CURRENT:
                return("QRY_DATA_CUR");
            default:
                return(String::Format("UNK(0x%02x)",nCmd()));
        }
    }
    switch (nTo())
    {
        case IGID_NODE_0E:
            switch (nCmd())
            {
                default:
                    break;
            }
            break;
        case IGID_PC:
            switch (nCmd())
            {
                case IGCMD_DATA_HISTORICALDATA:
                    return("HIST_DATA");
                default:
                    break;
            }
            break;
        case IGID_DATALOGGER:
            switch (nCmd())
            {
                case IGCMD_DEV_GETCONFIG:
                    return("QRY_DEV_CONFIG");
                case IGCMD_GET_HISTORICALDATA:
                    return("QRY_DL_HIST");
                //              case IGCMD_DATA_HISTORICALDATA: return("HIST_DATA");
                case IGCMD_DL_GET_DATETIME:
                    return("QRY_DL_DTG");
                case IGCMD_DL_SET_DATETIME:
                    return("SET_DL_DTG");
                case IGCMD_DL_GET_OPTION:
                    return("QRY_DL_OPTION");
                case IGCMD_DL_SET_OPTION:
                    return("SET_DL_OPTION");
                case IGCMD_DL_GET_ID:
                    return("GET_DL_ID");
                default:
                    break;
            }
            break;
        case IGID_BROADCAST:
            switch (nCmd())
            {
                case IGCMD_DEVICELIST:
                    return("QRY_DEVLIST");
                default:
                    break;
            }
            break;
    }
    return(String::Format("UNK(0x%02x)",nCmd()));
}

String                          clsIGMessage::AsString() const
{
    String  s;

    // printf(" string size: %d %#p\n",_nDataSize,&s);
    s = String::Format("%-7s -> %-7s: %-8s: ",(LPCTSTR)sFrom(),(LPCTSTR)sTo(),
                       (LPCTSTR)sCmd());

    for (unsigned n = 0; n < _nDataSize && s.Len() <= 80; ++n)
    {
        //      if ((n % 16) == 15)
        //          s += String::Format("\n%44s: ","");
        s.AddStrSeparated(" ",String::Format("%02x",_pData[n]));
    }

    return(s.Len() > 80 ? s.Left(80) + "..." : s);
}

int                             clsIGMessage::operator==
(const clsIGMessage& rhs) const
{
    // _chk_utildebug("op==(%s ---%s)",(LPCTSTR)AsString(),(LPCTSTR)rhs.AsString());
    return(nFrom() == rhs.nFrom() &&
           nTo() == rhs.nTo() &&
           nCmd() == rhs.nCmd());
}

int                             clsIGMessage::operator<(const clsIGMessage&
        rhs) const
{
    // _chk_utildebug("op<(%s ---%s)",(LPCTSTR)AsString(),(LPCTSTR)rhs.AsString());
    if (nFrom() < rhs.nFrom())
        return(1);
    if (nFrom() > rhs.nFrom())
        return(0);
    if (nTo() < rhs.nTo())
        return(1);
    if (nTo() > rhs.nTo())
        return(0);
    if (nCmd() < rhs.nCmd())
        return(1);
    if (nCmd() > rhs.nCmd())
        return(0);
    return(0);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

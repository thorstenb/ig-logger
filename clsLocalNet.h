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

#define DETAILED_OUTPUT	0

#define DEFINE_DATA	1
#include "clsData.h"
#undef DEFINE_DATA

#include <datecls.hpp>
#include "clsSystemTime.h"
#include "clsExcept.h"
#include "clsErrorList.h"
#include "clsBroadcast.h"

#if 1
template <class T>
class clsResult
	{
	private:
		String							_sText;
		T&								_Res;
		String							_sResult;
	public:
										clsResult(const String& sText, T& Res)
											: _Res(Res)
											, _sText(sText)
											{
											_chk_debug(+1,TEXT(">%s"),(LPCTSTR)_sText);
											}
										~clsResult(void)
											{
											if (_sResult.IsEmpty())
												{
												_chk_debug(-1,TEXT("<%s (%d)"),(LPCTSTR)_sText,_Res);
												}
											  else
											  	{
												_chk_debug(-1,TEXT("<%s (%s) -> %d"),(LPCTSTR)_sText,(LPCTSTR)_sResult,_Res);
											  	}
											}

		void							Set(const String& sResult)
											{
											_sResult = sResult;
											}
	};
#else
template <class T>
class clsResult
	{
	public:
										clsResult(const String& /* sText */, T& /* Res */)
											{
											}
										~clsResult(void)
											{
											}

		void							Set(const String&)
											{
											}
	};
#endif

//////////////////////////////////////////////////////////////////////////////

class clsDeviceID
	{
	private:
		UINT							_nHi;
		UINT							_nLo;
	public:
										clsDeviceID()
											: _nHi(0)
											, _nLo(0)
											{
											}
										clsDeviceID(UINT nHi, UINT nLo)
											: _nHi(nHi)
											, _nLo(nLo)
											{
											}
										clsDeviceID(const clsDeviceID& rhs)
											: _nHi(rhs._nHi)
											, _nLo(rhs._nLo)
											{
											}

		clsDeviceID&					operator=(const clsDeviceID& rhs)
											{
											if (&rhs != this)
												{
												_nHi = rhs._nHi;
												_nLo = rhs._nLo;
												}
											return(*this);
											}
		bool							operator==(const clsDeviceID& rhs) const
											{
											return(_nHi == rhs._nHi &&
												   _nLo == rhs._nLo);
											}
		bool							operator!=(const clsDeviceID& rhs) const
											{
											return(!(*this == rhs));
											}
		bool							operator<(const clsDeviceID& rhs) const
											{
											if (_nHi != rhs._nHi)
												return(_nHi < rhs._nHi);
											return(_nLo < rhs._nLo);
											}

		bool							bOk() const
											{
											return(_nHi != 0);
											}
		String							AsStr() const
											{
											return(String::Format(TEXT("%d.%d"),_nHi,_nLo));
											}
	};

enum	enLocalNetID
	{
	IGID_BROADCAST = 0x00,
	IGID_DATALOGGER = 0x01,
	IGID_NODE_0E = 0x0e,
	IGID_PC = 0x10,
	IGID_SENSORCARD_FIRST = 0x34,
	IGID_SENSORCARD_LAST = IGID_SENSORCARD_FIRST+10-1, // 10 sensor cards
	IGID_INTERFACECARD = 0x3d,
	IGID_INVERTER_FIRST = 0x61,
	IGID_INVERTER_LAST = IGID_INVERTER_FIRST+100-1, // 100 inverters
	IGID_INVALID = 0xff,
	};
enum	enLocalNetType
	{
	IGTYPE_DATALOGGER = 0xfe, // also: interface card, sensor card
	IGTYPE_PC = 0xfc,
	IGTYPE_NODE_0E = 0x00,
	IGTYPE_INVERTER = 0xfa,
	IGTYPE_INVALID = 0xff
	};

class clsLocalNetID
	{
	private:
		enLocalNetID					_nID;
		enLocalNetType					_nType;
	public:
										clsLocalNetID()
											: _nID(IGID_INVALID)
											, _nType(IGTYPE_INVALID)
											{
											}
										clsLocalNetID(const clsLocalNetID& rhs)
											: _nID(rhs._nID)
											, _nType(rhs._nType)
											{
											}
										clsLocalNetID(enLocalNetID nID, enLocalNetType nType)
											: _nID(nID)
											, _nType(nType)
											{
											}

		clsLocalNetID&					operator=(const clsLocalNetID& rhs)
											{
											if (&rhs != this)
												{
												_nID = rhs._nID;
												_nType = rhs._nType;
												}
											return(*this);
											}
		int								operator==(const clsLocalNetID& rhs) const
											{
											return(_nID == rhs._nID &&
												_nType == rhs._nType);
											}
		int								operator<(const clsLocalNetID& rhs) const
											{
											if (_nID == rhs._nID)
												return(_nType < rhs._nType);
											return(_nID < rhs._nID);
											}

		enLocalNetID					nID() const
											{
											return(_nID);
											}
		enLocalNetType					nType() const
											{
											return(_nType);
											}
	};

String					sLocalNetID(enLocalNetID nID);
String					sLocalNetID(const clsLocalNetID& Device);
inline bool				IsInverter(enLocalNetID nID)
{
	return(nID >= IGID_INVERTER_FIRST && nID <= IGID_INVERTER_LAST);
}
inline bool				IsInverter(const clsLocalNetID& Device)
{
	return(IsInverter(Device.nID()));
}

inline bool				IsSensorcard(enLocalNetID nID)
{
	return(nID >= IGID_SENSORCARD_FIRST && nID <= IGID_SENSORCARD_LAST);
}
inline bool				IsSensorcard(const clsLocalNetID& Device)
{
	return(IsSensorcard(Device.nID()));
}

class clsDataPacket
	{
	private:
		UINT							_nSize;
		BYTE*							_pData;
	public:
										clsDataPacket(BYTE* pHeader, UINT nPayloadSize, BYTE* pPayload);
										~clsDataPacket();

		UINT							nSize() const
											{
											return(_nSize);
											}
		BYTE*							pData() const
											{
											return(_pData);
											}
	};

class clsIGMessage
	{
	public:
		enum							enCmd
											{
											IGCMD_DEVICELIST = 0x00,
												//< 31 01 00 00 00 01 FE 71 32
												//> 31 01 00 00 00 01 FE 0E 00 10 FC D8 32

												//> 31 0E 00 00 00 0E 00 00 32
												//< 31 0E 00 00 00 0E 00 61 FA 01 FE 22 32
											IGCMD_DEV_GETCONFIG = 0xff,
											IGCMD_DL_SET_DATETIME = 0x32,
												// command: <ss><mm><hh><dd><mm><yy>
												// response <ss><mm><hh><dd><mm><yy><dow>
												//> 31 10 01 32 32 0E 14 08 07 07 BB 32
												//< 31 10 01 32 32 0E 14 08 07 07 07 CD 32
											IGCMD_DL_GET_DATETIME = 0x33,
												// response <ss><mm><hh><dd><mm><yy><dow>
											IGCMD_GET_HISTORICALDATA = 0x2e,
											IGCMD_DL_GET_OPTION = 0x30,
											IGCMD_DL_SET_OPTION = 0x31,
											IGCMD_DL_GET_ID = 0x35,
											IGCMD_INV_QUERYDISPLAY_CURRENT = 0x33,
											IGCMD_SNS_QUERYDISPLAY_CURRENT = 0x32, // + 1 byte "what"
											IGCMD_SNS_QUERYDISPLAY_DAY = 0x33,	   // + 1 byte "what"
											IGCMD_SNS_QUERYDISPLAY_YEAR = 0x35,	   // only one command
											IGCMD_SNS_QUERYDISPLAY_TOTAL = 0x35,
											IGCMD_ACK_HISTORICALDATA = 0x35,

											IGCMD_DATA_HISTORICALDATA = 0x31, // in a reply...

											IGINFO_INVERTER_RESYNC = 0x62,
											};
		enum							enDataloggerOption
											{
											IG_DL_OPTION_INVERTERHISTOINTERVAL = 0x60,
												// get: 10 01 30 60
												// set: 10 01 31 60 c3 <80+seconds>
											IG_DL_OPTION_SENSORHISTOINTERVAL = 0x33,
												// get: 10 01 30 33
												// set: 10 01 31 33 3c <80+seconds>
											IG_DL_OPTION_BROADCASTPORT = 1, // not a "real" variable...
											};
	private:
		BYTE*							_pData;
		unsigned						_nDataSize;
		bool							_bChecksumOk;
		bool							_bComplete;
		BYTE							_nExpectedBlockNumber;

		String							sFrom() const
											{
											return(sLocalNetID(nFrom()));
											}
		String							sTo() const
											{
											return(sLocalNetID(nTo()));
											}
		String							sCmd() const;

		static unsigned					MaxBufferSize(unsigned nDataSize)
											{
											return(3+nDataSize);
											}
		unsigned						MaxBufferSizeForCOMM() const
											{
											return(2*(_nDataSize+1)+1);
											}
	public:
										clsIGMessage();
										clsIGMessage(const clsIGMessage& rhs);
										clsIGMessage(const BYTE* pData, unsigned nDataSize);
										clsIGMessage(enLocalNetID nFrom, enLocalNetID nTo, enCmd nCmd, BYTE* pData = NULL, unsigned nDataSize = 0);
										clsIGMessage(enLocalNetID nFrom, enLocalNetID nTo, enCmd nCmd, unsigned nDataSize, ...);
										~clsIGMessage();

		int								operator==(const clsIGMessage& rhs) const;
		int								operator<(const clsIGMessage& rhs) const;
		clsIGMessage&					operator=(const clsIGMessage& rhs);
		clsIGMessage&					operator+=(const clsIGMessage& rhs);

		bool							GetPacketsForCOMM(WCValSList<clsDataPacket*>& Packets) const;
		enLocalNetID				 	nFrom() const;
		enLocalNetID				 	nTo() const;
		enCmd							nCmd() const;
		BYTE							nBlockNumber() const
											{
											return(Payload()[0]);
											}
		void							RemoveBlockNumber();
		bool							bComplete()	const
											{
											return(_bComplete);
											}
		String							AsString() const;

		BYTE							RemoveBlockCounter();
		void							AppendPayload(const BYTE* pData, unsigned nDataSize);
		void							AppendPayload(unsigned nDataSize, ...);
		void							SetPayload(const BYTE* pData, unsigned nDataSize);
		void							SetPayload(unsigned nDataSize, ...);
		const BYTE*						Payload() const
											{
											return(_pData+3);
											}
		const UINT						PayloadSize() const
											{
											return(_nDataSize-3);
											}
		bool							bChecksumOk() const
											{
											return(_bChecksumOk);
											}
	};

class clsIGInboundMessageList
	{
	private:
		clsCrit							_Crit;
		WCValSList<clsIGMessage> 		_List;
		unsigned						_nMessages;
		clsEvent						_evMsgPresent;
	public:
										clsIGInboundMessageList(unsigned nMessages = 0);

		void							Append(const clsIGMessage& Msg);
		bool							Get(clsIGMessage& Msg, unsigned nTimeout);
		bool							Get(WCValSList<clsIGMessage>& MsgList, unsigned nTimeout);
	};

class clsIGInterface;
class clsLocalNet;

class clsDeviceList
	: public WCValSkipListDict<clsLocalNetID,String>
	{
	public:
		void							Clear()
											{
											clear();
											}
	};
typedef WCValSkipListDictIter<clsLocalNetID,String>	clsDeviceListIter;

class clsDeviceListManager
	: public clsThreadBase
	{
	private:
		clsCrit							_CritOfToDoList;
		clsCrit							_CritOfNetDeviceList;
		clsIGInterface& 				_IGInterface;
		WCValSList<clsIGMessage>		_ToDoList;
		clsDeviceList					_NetDeviceList;
		clsEvent						_evMSGEvent;

		virtual DWORD					ThreadFunction(void);
		virtual DWORD					StackSize(void)
											{
											return(0x10000);
											}
		bool							ContainsNode(enLocalNetID nNode);
	public:
										clsDeviceListManager(clsIGInterface& IGInterface);
										~clsDeviceListManager();

		void							Clear();
		void							OnDeviceList(const clsIGMessage& Msg);
		bool							ContainsDatalogger();
		bool							ContainsSensorcard();
	};

class clsLoggerWatch
	: public clsThreadBase
	{
	private:
		clsLocalNet*	 				_pLocalNet;
		clsDeviceID						_ID;

		virtual DWORD					ThreadFunction(void);
		virtual DWORD					StackSize(void)
											{
											return(0x10000);
											}
	public:
										clsLoggerWatch(clsLocalNet* pLocalNet);
										~clsLoggerWatch();

		void							Watch(const clsDeviceID& ID);
	};

class clsMessageBuffer
	{
	private:
		BYTE							_arData[1024];
		int								_nDataLen;
	public:
										clsMessageBuffer()
											{
											memset(_arData,0,sizeof(_arData));
											_nDataLen = 0;
											}
		void							Append(BYTE b)
											{
											if (_nDataLen < sizeof(_arData))
												{
												_arData[_nDataLen++] = b;
												}
											}
		const BYTE*						pData() const
											{
											return(_arData);
											}
		int								nDataSize() const
											{
											return(_nDataLen);
											}
	};

class clsIGInterface
	: private IDataSink
	{
	private:
		clsIOBase*						_pIO;
		clsCrit							_Crit;
		UINT							_nPacketsReceived;
		UINT							_nPacketsReceivedWithBadChecksum;
		WCStack<clsMessageBuffer*, WCValSList<clsMessageBuffer*> > _MessageStack;

		clsCrit							_CritQueryList;
		WCValSkipListDict<clsIGMessage,clsIGInboundMessageList*>	_QueryList;
		clsIGInboundMessageList 		_UnassignedMessages;

		void							NtfyData(const BYTE* pData, UINT nSize);
	protected:
		clsDeviceListManager			_DeviceListManager;

		void							SetActive(bool bActive);
		bool							GetMessage(clsIGMessage& Msg, unsigned nTimeout = 500);
		bool							Query(const clsIGMessage& MsgQuery, const clsIGMessage& MsgFilter, clsIGMessage& MsgResponse, unsigned nTimeout = 500);
		bool							Query(const clsIGMessage& MsgQuery, clsIGMessage& MsgResponse, unsigned nTimeout = 500)
											{
											return(Query(MsgQuery,MsgQuery,MsgResponse,nTimeout));
											}
		bool							QueryMultiple(const clsIGMessage& MsgQuery, WCValSList<clsIGMessage>& MsgResponseList, int nMessages, unsigned nTimeout = 500);
	public:
										clsIGInterface();
		virtual							~clsIGInterface();

		bool							Open(const String& sDevice);
		void							Close();
		bool							Post(const clsIGMessage& Msg);
		String							sDevice();
	};


#define DEFINE_DATA	2
#include "clsData.h"
#undef DEFINE_DATA

class clsLocalNet
	: public clsIGInterface
	{
	public:
		enum							enDisplaySelection
											{
											DATA_STATE,
											DATA_CURRENT,
											DATA_DISPLAY_VALUES,
											};
		enum							enMode
											{
											MODE_NEUTRAL = 0x00,
											MODE_INSTALL = 0x01,
											MODE_RRDTOOL = 0x02,
											MODE_SQLITE = 0x04,
											};
		enum							enResult
											{
											RES_OK = 0,
											RES_INSTALL_DONE = 1,
											RES_USERABORT = 2,
											ERR_CONNECTIONLOST = -1,
											ERR_FATAL = -2,
											};
	private:
		clsCrit							_Crit;
		clsDeviceList					_ActiveDeviceList;
		WCValSkipListDict<String,clsSYSTEMTIME> _Timestamp;
		clsErrorList					_ErrorStatesOfDay;
		enMode 							_nMode;
		UINT							_nStartDay;
		WCValSList<UINT>				_nHoursOfHistoUpdate;
		WCValSkipListDict<clsIGMessage::enDataloggerOption,clsVariant> _DataloggerOptions;
		clsCrit							_CritForExceptionsFromOtherThreads;
		WCValSList<clsExceptBase*>		_ExceptionsFromOtherThreads;
		clsLoggerWatch					_DataloggerWatch;
		clsBroadcastServerMailslot		_BroadcastServer;

		bool							GetNodeDevices(WCValSList<clsLocalNetID>& MissingDeviceList);
		bool							QueryLoggerDateTime(clsSYSTEMTIME& ST);
		bool							SetLoggerDateTime(const clsSYSTEMTIME& ST);
		bool							QueryLoggerInverterHistoInterval(int& nMinutes);
		bool							SetLoggerInverterHistoInterval(const int nMinutes);
		bool							QueryLoggerSensorHistoInterval(int& nMinutes);
		bool							SetLoggerSensorHistoInterval(const int nMinutes);
		bool							QueryData(const clsLocalNetID& nID, enDisplaySelection nDisplaySelection, clsDataItemList* pDataList = NULL, String* pValue = NULL, String* psStateErrortext = NULL);
		bool							QueryHistoricalData(const clsDate& Date, clsDeviceDataList* pDataList = NULL);
		bool							QuerySensor(const clsLocalNetID& nID, clsDataItemList* pDataList = NULL, String* pValue = NULL);

		unsigned 						GetValueFromPayloadLE(int nBytes, const BYTE*& pPayload);
		unsigned 						GetValueFromPayloadBE(int nBytes, const BYTE*& pPayload);

		void							OnState(BYTE nState, const String& sDescription);
	public:
										clsLocalNet(enMode nMode, const WCValSkipListDict<clsIGMessage::enDataloggerOption,clsVariant>& DataloggerOptions);

		bool							Open(String& sPort);
		enResult						ProcessMessages(bool bActive, bool bFirstProcess);

		static String					GetStateDescription(unsigned nState);
		void							AddException(clsExceptBase* pExcept);
		clsExceptBase*					GetException();

		bool							QueryDataloggerID(clsDeviceID& nID);
	};

inline clsLocalNet::enMode operator|(clsLocalNet::enMode a, clsLocalNet::enMode b)
{
  	return clsLocalNet::enMode((int)a|(int)b);
}

//////////////////////////////////////////////////////////////////////////////

enum	enUnit
	{
	UNIT_ENERGY = 0x01,
	UNIT_LITER = 0x03,
	UNIT_MILLILITER = 0x04,
	UNIT_SPEED_KMH = 0x07,
	UNIT_SPEED_MPH = 0x08,
	UNIT_ENERGY_TOTAL = 0x09,
	UNIT_PERCENT = 0x0a,
	UNIT_MILLIBAR = 0x0c,
	UNIT_HECTOPASCAL = 0x0d,
	UNIT_POWER = 0x0e,
	UNIT_CURRENT = 0x10,
	UNIT_VOLTAGE = 0x11,
	UNIT_RESISTANCE = 0x12,
	UNIT_FREQUENCY = 0x13,
	UNIT_TEMPERATURE_C = 0x14,
	UNIT_TEMPERATURE_F = 0x15,
	UNIT_WEIGHT_GRAMS = 0x18,
	UNIT_WEIGHT_TON = 0x19,
	UNIT_IRRADIATION_SQRM = 0x1b,
	UNIT_ENERGY_SQRM = 0x1c,
	UNIT_RUNTIME_M = 0x21,
	UNIT_HOUR = 0x22,
	UNIT_MINUTE = 0x23,
	UNIT_SECOND = 0x24,
	UNIT_RPM = 0x25,
	UNIT_UNKNOWN = 0xff,
	};

extern	const char* pszUnit(enUnit nUnit);
extern	String		sValue(double dValue, int nDecs, enUnit nUnit);

#pragma pack(push,1)
struct scData4
	{
	BYTE		_nVal[4];
	enUnit		_nUnit : 8;
	BYTE		_nExp;

	double		dValue() const
					{
					// return(((int)_nValHi << 8) + _nValLo);
					unsigned	nValue = 0;
					for (int i = 0; i < 4; ++i)
						{
						nValue = (nValue << 8) + _nVal[i];
						}
					if (_nUnit == UNIT_UNKNOWN)
						return(0);
					if (_nExp < 0x80)
						return(nValue); //  + _nExp/100.0);
					return(nValue * pow(10,_nExp-0x84));
					}
	int			nDecimals() const
					{
					return(min(max(0,0x84-_nExp),10));
					}
	String		sValue() const
					{
					return(::sValue(dValue(),nDecimals(),_nUnit));
					}
	};
struct scData2
	{
	BYTE		_nValHi;
	BYTE		_nValLo;
	enUnit		_nUnit : 8;
	BYTE		_nExp;

	double		dValue() const
					{
					// return(((int)_nValHi << 8) + _nValLo);
					if (_nUnit == UNIT_UNKNOWN)
						return(0);
					if (_nExp < 0x80)
						return(((int)_nValHi << 8) + _nValLo);
					return((((int)_nValHi << 8) + _nValLo) * pow(10,_nExp-0x84));
					}
	int			nDecimals() const
					{
					return(min(max(0,0x84-_nExp),10));
					}
	String		sValue() const
					{
					return(::sValue(dValue(),nDecimals(),_nUnit));
					}
	};
struct scData1
	{
	BYTE		_nVal;
	enUnit		_nUnit : 8;
	BYTE		_nExp;

	double		dValue() const
					{
					if (_nUnit == UNIT_UNKNOWN)
						return(0);
					if (_nExp < 0x80)
						return(_nVal); //  + _nExp / 100.0);
					return(_nVal * pow(10,_nExp-0x84));
					}
	int			nDecimals() const
					{
					return(max(0,0x84-_nExp));
					}
	String		sValue() const
					{
					return(::sValue(dValue(),nDecimals(),_nUnit));
					}
	};
struct scDataBYTE
	{
	BYTE		_nVal;

	UINT		nValue() const
					{
					return(_nVal);
					}
	};
struct scDataUINT
	{
	BYTE		_nValHi;
	BYTE		_nValLo;

	UINT		nValue() const
					{
					return(((UINT)_nValHi << 8) + _nValLo);
					}
	};
#pragma pack(pop)

class clsIGValue
	{
	private:
		unsigned						_nMask;
		unsigned						_nBytes;
		const char*						_pszName;
		const BYTE*						_pData;

		int								nExp(const BYTE* pPayload) const
											{
											return(pPayload[_nBytes-1]);
											}
		int								nDecimals(const BYTE* pPayload) const
											{
											return(min(max(0,nExp(pPayload) >= 100 ? 0x84-nExp(pPayload) : 2),10));
											}
	public:
										clsIGValue(unsigned nMask, unsigned nBytes, const char* pszName)
											: _nMask(nMask)
											, _nBytes(nBytes)
											, _pszName(pszName)
											, _pData(NULL)
											{
											}
										clsIGValue(unsigned nMask, unsigned nBytes, const char* pszName, const BYTE*& pData)
											: _nMask(nMask)
											, _nBytes(nBytes)
											, _pszName(pszName)
											, _pData(pData)
											{
											if (pData)
												pData += nBytes;
											}

		virtual double					dValue(const BYTE* pPayload) const
											{
											// return(((int)_nValHi << 8) + _nValLo);
											unsigned	nValue = 0;
											for (int i = 0; i < _nBytes-2; ++i)
												nValue = (nValue << 8) + pPayload[i];
											if (nUnit(pPayload) == UNIT_UNKNOWN)
												return(0);
											if (nExp(pPayload) < 0x80)
												return(nValue); //  + nExp(pPayload)/100.0);
											double	dValue = nValue * pow(10,nExp(pPayload)-0x84);
											return(dValue);
											}
		virtual String					sValue(const BYTE*& pPayload) const
											{
											String	sRes = ::sValue(dValue(pPayload),nDecimals(pPayload),nUnit(pPayload));
											return(sRes);
											}
		virtual double					dValueEx() const
											{
											unsigned	nValue = 0;
											for (int i = 0; i < _nBytes-2; ++i)
												nValue = (nValue << 8) + _pData[i];
											if (nUnit(_pData) == UNIT_UNKNOWN)
												return(0);
											if (nExp(_pData) < 0x80)
												return(nValue); // + nExp(_pData)/100.0);
											double	dValue = nValue * pow(10,nExp(_pData)-0x84);
											return(dValue);
											}
		virtual String					sValueEx() const
											{
											String	sRes = ::sValue(dValueEx(),nDecimals(_pData),nUnit(_pData));
											return(sRes);
											}
		enUnit							nUnit(const BYTE* pPayload) const
											{
											return((enUnit)pPayload[_nBytes-2]);
											}

		const char*						pszName()
											{
											return(_pszName);
											}
		unsigned						nMask()
											{
											return(_nMask);
											}
		unsigned						nBytes()
											{
											return(_nBytes);
											}
	};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

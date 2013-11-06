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
#include "clsConfig.h"
#include "clsOutputManager.h"

clsConfigOfLocation	__LocationConfig;

void 					Log(const char* pszFormat, ...)
{
	String	s;
  	va_list	args;

  	va_start(args,pszFormat);
	_chk_utildebug("***: %s\n",(LPCTSTR)String::VFormat(pszFormat, args));
	va_end(args);
}

String					sLocalNetID(enLocalNetID nID)
{
	if (IsInverter(nID))
		{
		return("INVERTER");
		}
	if (IsSensorcard(nID))
		{
		return("SENSORCARD");
		}
	switch (nID)
		{
		case IGID_BROADCAST: 					return("ALL");
		case IGID_PC: 							return("PC");
		case IGID_NODE_0E:						return("NODE 0E");
		case IGID_DATALOGGER:					return("LOGGER");
		case IGID_INTERFACECARD:				return("INTERFACECARD");
		case IGID_INVALID:						return(String::Format("INV(%02x)",nID));
		default:								return(String::Format("UNK(%02x)",nID));
		}
}

String					sLocalNetID(const clsLocalNetID& Device)
{
	if (IsInverter(Device.nID()))
		{
		switch (Device.nType())
			{
			case 0xE3:								return(String::Format("ID %02x(IG 4500-LV)",Device.nID()));
			case 0xE5:								return(String::Format("ID %02x(IG 2500-LV)",Device.nID()));
			case 0xEA:								return(String::Format("ID %02x(IG 5100)",Device.nID()));
			case 0xEB:								return(String::Format("ID %02x(IG 4000)",Device.nID()));
			case 0xED:								return(String::Format("ID %02x(IG 3000)",Device.nID()));
			case 0xEE:								return(String::Format("ID %02x(IG 2000)",Device.nID()));
			case 0xF3:								return(String::Format("ID %02x(IG 60/60-HV)",Device.nID()));
			case 0xF4:								return(String::Format("ID %02x(IG 500)",Device.nID()));
			case 0xF5:								return(String::Format("ID %02x(IG 400)",Device.nID()));
			case 0xF6:								return(String::Format("ID %02x(IG 300)",Device.nID()));
			case 0xF9:								return(String::Format("ID %02x(IG 60/60-HV)",Device.nID()));
			case 0xFA:								return(String::Format("ID %02x(IG 40)",Device.nID()));
			case 0xFB:								return(String::Format("ID %02x(IG 30 Dummy)",Device.nID()));
			case 0xFC:								return(String::Format("ID %02x(IG 30)",Device.nID()));
			case 0xFD:								return(String::Format("ID %02x(IG 20)",Device.nID()));
			case 0xFE:								return(String::Format("ID %02x(IG 15)",Device.nID()));
			default:								return(String::Format("UNK(%02x)",Device.nType()));
			}
		}
	  else
	if (IsSensorcard(Device.nID()))
		{
		return(String::Format("ID %02x(SENSORCARD %d)",Device.nID(),Device.nID()-IGID_SENSORCARD_FIRST+1));
		}

	return(::sLocalNetID(Device.nID()));
}

//////////////////////////////////////////////////////////////////////////////

clsLocalNet::clsLocalNet(enMode nMode, const WCValSkipListDict<clsIGMessage::enDataloggerOption,clsVariant>& DataloggerOptions)
	: clsIGInterface()
	, _nMode(nMode)
	, _nStartDay(0)
	, _DataloggerOptions(DataloggerOptions)
	, _DataloggerWatch(this)
{
	if (_DataloggerOptions.contains(clsIGMessage::IG_DL_OPTION_BROADCASTPORT))
		_BroadcastServer.Open("logig");
}

bool							clsLocalNet::Open(String& sPort)
{
#if 0
	clsSolarInfo	SI(__LocationConfig);
	exit(0);
#endif
#if 0
	clsRRDTOOL		RRD(__LocationConfig,"IG 40 ID 1.44226");

	RRD.UploadFile("c:\\test1.htm");
	Sleep(15000);
	exit(1);
#endif

	bool	bLoggerFound(false);

	try
		{
		if (!sPort.IsEmpty())
			{
			bool		bPortFound(false);

			try
				{
				bPortFound = clsIGInterface::Open(sPort);
				}
			catch (clsExcept& e)
				{
				printf(" Open(%s): %s\n",(LPCTSTR)sPort,(LPCTSTR)e.Text());
				}
			if (bPortFound)
				{
				printf("snooping %s for Fronius LocalNet broadcasts\n",(LPCTSTR)clsIGInterface::sDevice());

				SetActive(true);
				clsIGMessage	Msg;

				for (DWORD dwTimeout = GetTickCount() + 2000; GetTickCount() < dwTimeout && !bUserAbortRequested(); )
					{
					GetMessage(Msg,100);

					if (_DeviceListManager.ContainsDatalogger())
						{
						printf("found a datalogger!\n");
						bLoggerFound = true;
						break;
						}
					}
				}
			  else
				{
				printf("no datalogger found at port '%s'!\n",(LPCTSTR)sPort);
				// throw clsExcept("no datalogger found at port '%s'",(LPCTSTR)sPort);
				}
			}
		  else
			{
			for (int nPort = 1; nPort <= 255+3 && !bLoggerFound && !bUserAbortRequested(); ++nPort)
				{
				bool		bPortFound(false);

				switch (nPort)
					{
					default:
						try
							{
							bPortFound = clsIGInterface::Open(sPort = String::Format("COM:COM%d",nPort));
							}
						catch (clsExcept& e)
							{
							printf(" Open(COM%d): %s\n",nPort,(LPCTSTR)e.Text());
							}
						break;
					case 255+1:
						try
							{
							bPortFound = clsIGInterface::Open(sPort = "FTDIW32:Datalogger");
							}
						catch (clsExcept& e)
							{
							printf(" Open(FTDIW32): %s\n",(LPCTSTR)e.Text());
							}
						break;
					case 255+2:
						try
							{
							bPortFound = clsIGInterface::Open(sPort = "FTDI:Datalogger");
							}
						catch (clsExcept& e)
							{
							printf(" Open(USB): %s\n",(LPCTSTR)e.Text());
							}
						break;
					case 255+3:
						try
							{
							bPortFound = clsIGInterface::Open(sPort = "USB:Datalogger");
							}
						catch (clsExcept& e)
							{
							printf(" Open(USB): %s\n",(LPCTSTR)e.Text());
							}
						break;
					}

				if (bPortFound)
					{
					printf("snooping %s for Fronius LocalNet broadcasts\n",(LPCTSTR)clsIGInterface::sDevice());

					SetActive(true);
					clsIGMessage	Msg;

					for (DWORD dwTimeout = GetTickCount() + 2000; GetTickCount() < dwTimeout && !bUserAbortRequested(); )
						{
						GetMessage(Msg,100);

						if (_DeviceListManager.ContainsDatalogger())
							{
							printf("found a datalogger!\n");
							bLoggerFound = true;
							break;
							}
						}

					if (!bLoggerFound)
						{
						clsIGInterface::Close();
						}
					}
				}
			if (!bLoggerFound)
				{
				sPort.Clear();
				}
			}
		}
	catch (clsExcept& e)
		{
		_DeviceListManager.Clear();
		throw;
		}

	return(bLoggerFound);
}

#include "clsLocalNetCurrentValues.c"
#include "clsLocalNetHistoInterval.c"
#include "clsLocalNetHistoValues.c"
#include "clsLocalNetDateTime.c"

unsigned 						clsLocalNet::GetValueFromPayloadLE(int nBytes, const BYTE*& pPayload)
{
	unsigned	nValue = 0;

	for (int i = 0; i < nBytes; ++i)
		{
		nValue = (nValue << 8) + *pPayload++;
		}
	return(nValue);
}

unsigned 						clsLocalNet::GetValueFromPayloadBE(int nBytes, const BYTE*& pPayload)
{
	unsigned	nValue = 0;

	for (int i = 0; i < nBytes; ++i)
		{
		nValue = (nValue << 8) + pPayload[nBytes-1-i];
		}
	pPayload += nBytes;
	return(nValue);
}

String							clsLocalNet::GetStateDescription(unsigned nState)
{
	struct scStateDescr
		{
		unsigned	_nState;
		const char*	_pszDescr;
		};

	static scStateDescr	__StateDescr[] =
		{
			{ 101, "Mains voltage not within admissible range"},
			{ 104, "Mains frequency not within admissible range"},
			{ 107, "Mains network not available"},
			{ 108, "Islanding detected"},
			{ 201, "ENS: AC High"},
			{ 202, "ENS: AC Low"},
			{ 203, "ENS: AC Freq High"},
			{ 204, "ENS: AC Freq Low"},
			{ 205, "ENS: AC Impedance imbalanced"},
			{ 206, "ENS: AC Impedance too high"},
			{ 207, "ENS: AC Relay defective"},
			{ 208, "ENS: AC Relay defective"},
			{ 301, "AC Current too high"},
			{ 302, "DC Current too high"},
			{ 303, "Temperature too high in AC part"},
			{ 304, "Temperature too high in DC part"},
			{ 306, "AC Current too low (?)"},
			{ 307, "DC Current too low"},
			{ 401, "No communication with Power stage set"},
			{ 402, "No communication with EEPROM"},
			{ 403, "EEPROM error"},
			{ 404, "No communication between control and ENS"},
			{ 405, "ENS wrong or defective"},
			{ 406, "Temp sensor defective (AC)"},
			{ 407, "Temp sensor defective (DC)"},
			{ 408, "Direct current input"},
			{ 409, "Missing +15V power supply for control"},
			{ 410, "Service Jumper not in correct position"},
			{ 412, "Voltage set as fix voltage is set too low"},
			{ 413, "Control problems"},
			{ 414, "EEPROM defective (empty)"},
			{ 415, "No OK from ENS"},
			{ 416, "No communication with IG Control"},
			{ 417, "Two Power stage sets have the same Print ID"},
			{ 419, "Two Power stage sets have the same Software ID"},
			{ 421, "Bad Print Number"},
			{ 425, "No communication with Power stage set"},
			{ 434, "Problem with Grounding"},
			{ 501, "Cooler defective"},
			{ 501, "Isolation low"},
			{ 504, "No communication with LocalNet (duplicate ID?)"},
			{ 505, "EEPROM defective"},
			{ 506, "EEPROM defective"},
			{ 507, "EEPROM defective"},
			{ 508, "Bad Fronius IG Address"},
			{ 509, "No Power for more than 24 hours"},
			{ 510, "EEPROM defective"},
			{ 511, "EEPROM defective"},
			{ 512, "Too many Power stage sets"},
			{ 514, "No communication with Power stage set"},
			{ 515, "Bad connections"},
			{ 516, "Error state in one of the Power stage sets"},
			{ 517, "Change of master has taken place"},
		};

	for (int i = 0; i < sizeof(__StateDescr)/sizeof(__StateDescr[0]); ++i)
		{
		if (nState == __StateDescr[i]._nState)
			return(String::Format("state %d: %s",nState,__StateDescr[i]._pszDescr));
		}
	return(String::Format("state %d",nState));
}

bool							clsLocalNet::GetNodeDevices(WCValSList<clsLocalNetID>& MissingDeviceList)
{
	_chk_debug(+1,">clsLocalNet::GetNodeDevices");

	clsIGMessage				MsgQuery(	IGID_NODE_0E,
											IGID_BROADCAST,
											clsIGMessage::IGCMD_DEVICELIST,
											3,
											0x00, // block number
											0x0e,
											0x00);
	clsIGMessage				MsgResponse;

	Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

	bool	bRes = Query(MsgQuery,MsgResponse);

	if (bRes)
		{
		const BYTE*	pPayload = MsgResponse.Payload();

		clsDeviceList 	NewNodeDeviceList;

		for (const BYTE* pData = pPayload; pData < pPayload + MsgResponse.PayloadSize(); )
			{
			if (*pData == 0) // fill byte??? start of new group? Whatever...
				{
				++pData;
				continue;
				}

			enLocalNetID	nID = (enLocalNetID)(*pData++);
			enLocalNetType	nType = (enLocalNetType)(*pData++);
			clsLocalNetID	Device(nID,nType);
			String			sDescription(sLocalNetID(Device));

			if (!_ActiveDeviceList.contains(Device))
				{
				bool	bInsertInList(true);
				printf("detected new device %s\n",(LPCTSTR)sLocalNetID(Device));

				if (Device.nID() != IGID_NODE_0E)
					{
					clsIGMessage				MsgQuery(	IGID_NODE_0E,
															Device.nID(),
															clsIGMessage::IGCMD_DEV_GETCONFIG);
					clsIGMessage				MsgResponse;

					Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

					bool						bRes = Query(MsgQuery,MsgResponse);

					if (bRes)
						{
						// printf("MsgResponse='%s'\n",(LPCTSTR)MsgResponse.AsString());
						String	s(sLocalNetID(Device));

						/*
						case IGTYPE_DATALOGGER:
							05 01 13 00 									   	// Software-Version 5.1.19
							0F 49 47 20 2D 20 44 61 74 61 6C 6F 67 67 65 72    	// "IG - Datalogger"
							03
							03 00 00 FC 00 06 89 								// ID 252.1673
							0A 44 61 74 61 6C 6F 67 67 65 72 					// "Datalogger"
							01 00 00 01 02 00 00 								// Platinenversion 1.2A
							0A 44 61 74 61 6C 6F 67 67 65 72					// "Datalogger"
							02 00 00 05 01 13 00 								// Software 5.1.19
							0A 44 61 74 61 6C 6F 67 67 65 72					// "Datalogger"
						case IGTYPE_INVERTER:
							02 08 01 00 										// Software-Version 2.8.1
							07 49 47 20 34 30 20 20 							// "IG 40  "
							0A
							02 10 01 02 08 01 00								// Software-Version 2.8.1
							07 49 47 2D 43 54 52 4C 							// "IG-CTRL"
							01 00 00 01 05 00 00 								// Platinenversion 1.5A
							07 49 47 2D 43 54 52 4C  							// "IG-CTRL"
							03 00 00 01 00 AC C2 								// ID 1.44226
							07 49 47 2D 43 54 52 4C  							// "IG-CTRL"

							02 00 00 02 03 00 00 								// Software 2.3.0
							05 50 53 20 30 30 									// "PS 00"
							01 00 00 01 03 00 00 								// Platinenversion 1.3A
							05 50 53 20 30 30 									// "PS 00"
							03 00 00 05 00 ED 0A 								// ID 5.60682
							05 50 53 20 30 30 									// "PS 00"

							02 00 00 02 03 00 00 								// Software 2.3.0
							05 50 53 20 30 31 									// "PS 01"
							01 00 00 01 03 00 00 								// Platinenversion 1.3A
							05 50 53 20 30 31 									// "PS 01"
							03 00 00 05 00 ED 0B 								// ID 5.60683
							05 50 53 20 30 31 									// "PS 01"

							02 00 00 26 14 00 00 								// Software V 38.20
							03 45 4E 53											// "ENS"
						case IGTYPE_SENSORCARD:
							01 16 00											// Software-Version 1.22
							0f "IG - Sensorcard"
							05

							02 00 00 01 01 16 00
							0a "Sensorcard"
							01 00 00 01 03 00 41 52 44 "ARD" - looks like a bug to me...!

							03 00 00 fe 00 0c 22
							0a "Sensorcard"

							ff 03 00 00 00 00 00
							0b "SC - Events"

							ff 00 00 00 07 0d 00
							0c "SC - Counter"
						*/
						const BYTE*	pPayload = MsgResponse.Payload();

						const BYTE*	pbRevision = pPayload;
						// possibly a zero-terminated number, or whatever...
						if (IsSensorcard(Device))
							pPayload += 3;
						  else
							pPayload += 4;

						sDescription += String::Format(" '%s'",(LPCTSTR)GetStringFromPayload(pPayload,false).ATrim());
						GetStringFromPayload(pPayload);
						s.AddStrSeparated("\n ",sDescription);
						int		nValues = GetValueFromPayloadLE(1,pPayload);
						for (int nIndex = 0; nIndex < nValues && (pPayload - MsgResponse.Payload()) < MsgResponse.PayloadSize(); ++nIndex)
							{
							unsigned	nType = GetValueFromPayloadLE(1,pPayload);
							unsigned	nUnknown1 = GetValueFromPayloadLE(1,pPayload); // usually 0, except for the first value...
							unsigned	nUnknown2 = GetValueFromPayloadLE(1,pPayload); // usually 0, except for the first value...
							switch (nType)
								{
								case 0x03:
									{
									unsigned	nVerHi = GetValueFromPayloadLE(1,pPayload);
									unsigned	nVerLo = GetValueFromPayloadLE(3,pPayload);
									clsDeviceID	ID(nVerHi,nVerLo);
									String		sKey = GetStringFromPayload(pPayload);

									s.AddStrSeparated("\n  ",String::Format("%s: ID %s",(const char*)sKey,(LPCTSTR)ID.AsStr()));
									if (sKey.Left(3) == "IG-")
										sDescription += ID.AsStr();
									}
									break;
								case 0x01:
									{
									unsigned	nVerHi = GetValueFromPayloadLE(1,pPayload);
									unsigned	nVerLo = GetValueFromPayloadLE(1,pPayload);
									unsigned	nVerRev = GetValueFromPayloadLE(1,pPayload);
									++pPayload;
									String		sKey = GetStringFromPayload(pPayload);

									s.AddStrSeparated("\n  ",String::Format("%s: PCB %d.%d%c",(const char*)sKey,nVerHi,nVerLo,'A'+nVerRev));
									}
									break;
								case 0x02:
									{
									unsigned	nVerHi = GetValueFromPayloadLE(1,pPayload);
									unsigned	nVerLo = GetValueFromPayloadLE(1,pPayload);
									unsigned	nVerRev = GetValueFromPayloadLE(1,pPayload);
									++pPayload;
									String		sKey = GetStringFromPayload(pPayload);

									s.AddStrSeparated("\n  ",String::Format("%s: Software %d.%d",(const char*)sKey,nVerHi,nVerLo));
									}
									break;
								case 0xff:
									{
									unsigned	nValue = GetValueFromPayloadLE(3,pPayload);
									++pPayload;
									String		sKey = GetStringFromPayload(pPayload);

									s.AddStrSeparated("\n  ",String::Format("%s: %d/%d/%d",(const char*)sKey,nUnknown1,nUnknown2,nValue));
									}
									break;
								}
							}
						printf("Config : %s\n",(LPCTSTR)s);
						}
					  else
						{
						if (IsSensorcard(Device))
							{
							}
						  else
							{
							bInsertInList = false; // error: unknown device type...
							}
						}
					}

				if (bInsertInList)
					{
					NewNodeDeviceList.insert(Device,sDescription);

					if (IsInverter(Device) ||
						IsSensorcard(Device))
						{
						__LocationConfig.NotifyNewDevice(Device);
						}
					if (Device.nType() == IGTYPE_DATALOGGER)
						{
						int		nMinutes;

						if (QueryLoggerInverterHistoInterval(nMinutes))
							{
							printf("  interval of inverter historical data: %d minutes\n",nMinutes);

							if (_DataloggerOptions.contains(clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL))
								{
								int		nNewMinutesNew = ((_DataloggerOptions[clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL].AsI4() + 4) / 5) * 5;

								if (nNewMinutesNew != nMinutes)
									{
									if (SetLoggerInverterHistoInterval(nNewMinutesNew))
										{
										printf("   changed to %d minutes\n",nNewMinutesNew);
										}
									}
								}
							}
						if (QueryLoggerSensorHistoInterval(nMinutes))
							{
							printf("  interval of sensor historical data: %d minutes\n",nMinutes);

							if (_DataloggerOptions.contains(clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL))
								{
								int		nNewMinutesNew = ((_DataloggerOptions[clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL].AsI4() + 4) / 5) * 5;

								if (nNewMinutesNew != nMinutes)
									{
									if (SetLoggerSensorHistoInterval(nNewMinutesNew))
										{
										printf("   changed to %d minutes\n",nNewMinutesNew);
										}
									}
								}
							}
						}
					clsDeviceID	ID;

					if (QueryDataloggerID(ID))
						{
						// printf("starting logger watcher for %d.%d\n",HIWORD(nID),LOWORD(nID));
						_DataloggerWatch.Watch(ID);
						}
					}
				}
			  else
				{
				// need to copy the description...
				NewNodeDeviceList.insert(Device,_ActiveDeviceList[Device]);
				}
			}

		for (clsDeviceListIter Device(_ActiveDeviceList); ++Device; )
			{
			if (!NewNodeDeviceList.contains(Device.key()))
				{
				printf("ALERT: device '%s' not present any more\n",(LPCTSTR)Device.value());
				MissingDeviceList.append(Device.key());
				}
			}

		_ActiveDeviceList = NewNodeDeviceList;
		}
	_chk_debug(-1,"<clsLocalNet::GetNodeDevices -> %d",bRes);
	return(bRes);
}

bool							clsLocalNet::QueryDataloggerID(clsDeviceID& ID)
{
	clsIGMessage	MsgQuery(	IGID_PC,
								IGID_DATALOGGER,
								clsIGMessage::IGCMD_DL_GET_ID);
	clsIGMessage	MsgResponse;
	bool			bRes(false);

	Log(">%-8s: %s","query", (LPCTSTR)MsgQuery.AsString());

	if (Query(MsgQuery,MsgResponse))
		{
		if (MsgResponse.PayloadSize() == 4)
			{
			const BYTE*	pPayload = MsgResponse.Payload();
			UINT		nIDHi = GetValueFromPayloadBE(1,pPayload);
			UINT		nIDLo = GetValueFromPayloadBE(3,pPayload);

			// printf("Datalogger ID: %d.%d\n",nIDHi,nIDLo);
			ID = clsDeviceID(nIDHi, nIDLo);
			bRes = true;
			}
		}
	return(bRes);
}

void							clsLocalNet::OnState(BYTE nState, const String& sDescription)
{
	if (nState >= 100)
		{
		printf("WRN: device %s has STATE %d\n",(LPCTSTR)sDescription,nState);
		}
}

class clsDestinationList
	: public WCValSList<clsOutputManager*>
	{
	public:
									~clsDestinationList()
										{
										while (!this->isEmpty())
											{
											delete this->get();
											}
										}
	};

clsLocalNet::enResult				clsLocalNet::ProcessMessages(bool bActive, bool bFirstProcess)
{
	clsIGMessage					Msg;
	clsDestinationList				DestinationList;

	if (_nMode & MODE_SQLITE)
		{
		DestinationList.append(new clsSQLite(__LocationConfig));
		}
	if (_nMode & MODE_RRDTOOL)
		{
		DestinationList.append(new clsRRDTOOL(__LocationConfig));
		}
	if (_DataloggerOptions.contains(clsIGMessage::IG_DL_OPTION_BROADCASTPORT) &&
		!_BroadcastServer.Ok())
		_BroadcastServer.Open("logig");

	clsSYSTEMTIME	ST(true);

	if (GetMessage(Msg,1000))
		{
		bool	bUnusedMessage(true);

		if (_DeviceListManager.ContainsSensorcard())
			{
			if (IsInverter(Msg.nFrom()) &&
				Msg.nCmd() == clsIGMessage::IGINFO_INVERTER_RESYNC)
				{
				// show this - might be interesting...
				//				bUnusedMessage = false;
				}
			if (Msg.nFrom() == IGID_NODE_0E &&
				IsSensorcard(Msg.nTo()) &&
				Msg.nCmd() == clsIGMessage::IGCMD_DEV_GETCONFIG)
				{
				bUnusedMessage = false;
				}
			}

		printf("unused: %02x %02x %02x %02x %02x\n",Msg.nFrom(),Msg.nTo(),Msg.nCmd(),Msg.PayloadSize(),Msg.Payload()[0]);
		if (Msg.nFrom() == IGID_DATALOGGER &&
			Msg.nTo() == IGID_PC &&
			Msg.nCmd() == clsIGMessage::IGCMD_DATA_HISTORICALDATA &&
			Msg.PayloadSize() == 3 &&
			Msg.Payload()[0] == 0x83)
			{
			// leftover from HISTO query..
			bUnusedMessage = false;
			}

		if (bUnusedMessage)
			{
			printf("%02d:%02d: unused message ignored: %s\n",ST.wHour,ST.wMinute,(LPCTSTR)Msg.AsString());
			}
		}

	{
	clsExceptBase* pExcept;

	while ((pExcept = GetException()) != NULL)
		{
		if (!bFirstProcess)
			{
			_ErrorStatesOfDay[ST] = pExcept->Text();
			if (pExcept->ErrorStack().entries() > 1)
				{
				String	sTop(pExcept->ErrorStack().pop());

				AlertUser(
					sTop,
					(LPCTSTR)pExcept->Text());
				}
			  else
			  	{
				AlertUser(
					(LPCTSTR)pExcept->Text(),
					"<no further information available>");
			  	}

			if (reinterpret_cast<clsConnectionLostExcept*>(pExcept) != NULL)
				{
				return(ERR_CONNECTIONLOST);
				}
			}
		delete pExcept;
		}
	}

	if (!_DeviceListManager.ContainsDatalogger())
		{
		_ActiveDeviceList.Clear();
		_Timestamp.clear();
		return(ERR_CONNECTIONLOST);
		}

	if (bActive)
		{
		clsSolarInfo	SI(__LocationConfig);
		bool			bForceQueryHistoricalData(false);

		if (_nStartDay == 0)
			{
			bForceQueryHistoricalData = true;
			}

		if (ST.wDay != _nStartDay)
			{
			_nHoursOfHistoUpdate.clear();
			_nStartDay = ST.wDay;

			// read historical data during the day:
			//  once every hour (from 1 hour before sunrise to 1 hour after sundown)
			for (INT nHour = max(0,(INT)floor(SI.dRise()-1)); nHour <= min(23,ceil(SI.dSet()+1)); ++nHour)
				{
				_nHoursOfHistoUpdate.append((UINT)nHour);
				}
			}

		double			dHourNow(ST.wHour + ST.wMinute / 60.0);

		if ((_nMode & MODE_INSTALL) == 0)
			{
			if (bForceQueryHistoricalData ||
				_nHoursOfHistoUpdate.contains(ST.wHour))
				{
				// remove hour from list
				_nHoursOfHistoUpdate.get(_nHoursOfHistoUpdate.index(ST.wHour));

				bool	bForceGraphics(true);

				clsDeviceDataList	DataList;
				clsDate				LastUpdate(__LocationConfig.GetDateOfLastUpdate());

				printf("query histo data from logger\n");
				QueryHistoricalData(LastUpdate,&DataList);
				// DataList.Dump(_ActiveDeviceList);

				printf("add histo data to database\n");
				for (WCValSListIter<clsOutputManager*> Destination(DestinationList); ++Destination; )
					{
					Destination.current()->Append(DataList,_ErrorStatesOfDay,true,bForceGraphics);
					}

				__LocationConfig.SetDateOfLastUpdate(clsDate((LPCTSTR)NULL));
				}
			}

		if ((ST.wSecond % 5) == 0)
			{
			for (clsDeviceListIter Device(_ActiveDeviceList); ++Device; )
				{
				if (IsInverter(Device.key()) &&
					(ST.wSecond % 10) == 0)
					{
					String		sInstallation(Device.value());

					try
						{
						String	sValue;

						QueryData(Device.key(),DATA_DISPLAY_VALUES,NULL,&sValue);
						printf("%02d:%02d:%02d: %s: %s\n",ST.wHour,ST.wMinute,ST.wSecond,(LPCTSTR)sInstallation,(LPCTSTR)sValue);
						}
					catch(clsStateExcept& e)
						{
						printf("%02d:%02d:%02d: %s: %s\n",ST.wHour,ST.wMinute,ST.wSecond,(LPCTSTR)sInstallation,(LPCTSTR)e.Text());
						}
					}
				if (IsSensorcard(Device.key()) &&
					(ST.wSecond % 10) == 5)
					{
					String		sInstallation(Device.value());

					try
						{
						String	sValue;

						QuerySensor(Device.key(),NULL,&sValue);
						printf("%02d:%02d:%02d: %s: %s\n",ST.wHour,ST.wMinute,ST.wSecond,(LPCTSTR)sInstallation,(LPCTSTR)sValue);
						}
					catch(clsStateExcept& e)
						{
						printf("%02d:%02d:%02d: %s: %s\n",ST.wHour,ST.wMinute,ST.wSecond,(LPCTSTR)sInstallation,(LPCTSTR)e.Text());
						}
					}
				}
			}

		if (!_Timestamp.contains("Action") ||
			((ST.wSecond % 30) == 0 &&
			 ST.wSecond != _Timestamp["Action"].wSecond))
			{
			WCValSList<clsLocalNetID>	MissingDeviceList;

			GetNodeDevices(MissingDeviceList); // also sets '_ActiveDeviceList'

			if (!MissingDeviceList.isEmpty())
				{
				while (!MissingDeviceList.isEmpty())
					{
					clsLocalNetID			nDeviceID(MissingDeviceList.get());
					clsConfigOfInstallation	Installation(__LocationConfig.Installation(nDeviceID));
					double					dToleranceSunrise(Installation._dToleranceSunrise);
					double					dToleranceSunset(Installation._dToleranceSunset);

					if (dHourNow > SI.dRise() + dToleranceSunrise &&
						dHourNow < SI.dSet() - dToleranceSunset)
						{
						AlertUser(
							String::Format("Device '%s' cannot be contacted",(LPCTSTR)Installation._sName),
							String::Format("possibly there's no power generated\n"
								"sunrise: %s\n"
								"sunset:  %s\n"
								"now:     %s",
									(LPCTSTR)clsSYSTEMTIME(SI.dRise(),clsSYSTEMTIME::MODE_HOUR).sTime(),
									(LPCTSTR)clsSYSTEMTIME(SI.dSet(),clsSYSTEMTIME::MODE_HOUR).sTime(),
									(LPCTSTR)ST.sTime()));
						}
					}
				}

			enResult		nRes(RES_OK);

			for (clsDeviceListIter Device(_ActiveDeviceList); ++Device; )
				{
				String	sDeviceID(sLocalNetID(Device.key()));

				if (IsSensorcard(Device.key()))
					{
					try
						{
						String	sStateError;

						if (ST.wMinute != _Timestamp[sDeviceID + ".Data.IntraDaySensors"].wMinute)
							{
							clsDataItemList		Data;
							clsSYSTEMTIME		STLogger;
							clsDeviceDataList	DeviceData;

							if (QueryLoggerDateTime(STLogger) &&
								QuerySensor(Device.key(),&Data))
								{
								DeviceData.Add(Device.key(),STLogger,Data);

								for (WCValSListIter<clsOutputManager*> Destination(DestinationList); ++Destination; )
									{
									Destination.current()->Append(DeviceData,_ErrorStatesOfDay,false,false);
									}

								_Timestamp[sDeviceID + ".Data.IntraDaySensors"] = ST;
								}
							}
						if (!sStateError.IsEmpty())
							{
							throw clsStateExcept(sStateError);
							}
						}
					catch (clsExcept& e)
						{
						printf("%s\n",(LPCTSTR)e.Text());
						}
					}
				if (IsInverter(Device.key()))
					{
					String				sInstallation(Device.value());
					String				sError;

					try
						{
						// once a day...
						if (ST.wHour == 12 &&
							ST.wDay != _Timestamp[sDeviceID + ".Data.Daily"].wDay)
							{
							printf("process daily database maintenance\n");
							for (WCValSListIter<clsOutputManager*> Destination(DestinationList); ++Destination; )
								{
								Destination.current()->DailyMaintenance(Device.key());
								}

							printf("clearing error states\n");
							_ErrorStatesOfDay.clear();

							_Timestamp[sDeviceID + ".Data.Daily"] = ST;
							}
						if (ST.wHour != _Timestamp[sDeviceID + ".QueryDateTime"].wHour) // every hour
							{
							clsConfigOfInstallation	Installation(__LocationConfig.Installation(Device.key()));

							if (Installation._bUpdateInverterClock)
								{
								clsSYSTEMTIME	STLogger;

								if (QueryLoggerDateTime(STLogger))
									{
									double		dTimeLogger = STLogger.dValue();
									double		dTimeNow = ST.dValue();

									printf("Datalogger time : %s\n",(LPCTSTR)STLogger.sValue());
									printf("System time     : %s\n",(LPCTSTR)ST.sValue());

									if (fabs(dTimeNow - dTimeLogger) > 1.0/(24.0*60.0))
										{
										printf("correcting date/time (difference is %.2f minutes [> 1 minutes])\n",fabs(dTimeNow - dTimeLogger)*(24.0*60.0));
										SetLoggerDateTime(ST);
										}
									_Timestamp[sDeviceID + ".QueryDateTime"] = ST;
									}
								}
							}
						}
					catch (clsExcept& e)
						{
						}
					try
						{
						String	sStateError;

						if (ST.wMinute != _Timestamp[sDeviceID + ".Data.IntraDay"].wMinute)
							{
							clsDataItemList		Data;
							clsSYSTEMTIME		STLogger;
							clsDeviceDataList	DeviceData;

							if (QueryLoggerDateTime(STLogger) &&
								QueryData(Device.key(),DATA_CURRENT,&Data,&sStateError))
								{
								DeviceData.Add(Device.key(),STLogger,Data);

								for (WCValSListIter<clsOutputManager*> Destination(DestinationList); ++Destination; )
									{
									Destination.current()->Append(DeviceData,_ErrorStatesOfDay,false,false);
									}

								_Timestamp[sDeviceID + ".Data.IntraDay"] = ST;
								}
							}
						if (!sStateError.IsEmpty())
							{
							throw clsStateExcept(sStateError);
							}

						if (_nMode & MODE_INSTALL)
							{
							nRes = RES_INSTALL_DONE;
							}
						}
					catch (clsStateExcept& e)
						{
						clsConfigOfInstallation	Installation(__LocationConfig.Installation(Device.key()));
						double					dToleranceSunrise(Installation._dToleranceSunrise);
						double					dToleranceSunset(Installation._dToleranceSunset);

						if (dHourNow > SI.dRise() + dToleranceSunrise &&
							dHourNow < SI.dSet() - dToleranceSunset)
							{
							_ErrorStatesOfDay[ST] = e.Text();
							AlertUser(
								String::Format("'%s': %s",(LPCTSTR)sInstallation,(LPCTSTR)e.Text()),
								String::Format("Possibly there's no power generated. This could be normal, though.\n"
									"sunrise: %s\n"
									"sunset:  %s\n"
									"now:     %s",
										(LPCTSTR)clsSYSTEMTIME(SI.dRise(),clsSYSTEMTIME::MODE_HOUR).sTime(),
										(LPCTSTR)clsSYSTEMTIME(SI.dSet(),clsSYSTEMTIME::MODE_HOUR).sTime(),
										(LPCTSTR)ST.sTime()));
							}
						}
					catch (clsExcept& e)
						{
						printf("%s\n",(LPCTSTR)e.Text());
						}
					}
				}
			_Timestamp["Action"] = ST;

			if (nRes)
				return(nRes);
			}
		}

	return(RES_OK);
}

void							clsLocalNet::AddException(clsExceptBase* pExcept)
{
	clsCritObj	CO(_CritForExceptionsFromOtherThreads);

	_ExceptionsFromOtherThreads.append(pExcept);
}

clsExceptBase*					clsLocalNet::GetException()
{
	clsCritObj	CO(_CritForExceptionsFromOtherThreads);

	if (_ExceptionsFromOtherThreads.isEmpty())
		{
		return(NULL);
		}
	return(_ExceptionsFromOtherThreads.get());
}

//////////////////////////////////////////////////////////////////////////////

const char* pszUnit(enUnit nUnit)
{
	switch (nUnit)
		{
		case UNIT_IRRADIATION_SQRM:	return("W/m^2");
		case UNIT_ENERGY_TOTAL:
		case UNIT_ENERGY:			return("kWh");
		case UNIT_POWER:			return("W");
		case UNIT_CURRENT:			return("A");
		case UNIT_VOLTAGE:			return("V");
		case UNIT_FREQUENCY:		return("Hz");
		case UNIT_RESISTANCE:		return("Ohm");
		case UNIT_RUNTIME_M:		return("");
		case UNIT_UNKNOWN:			return("-");
		case UNIT_TEMPERATURE_C:	return("degC");
		case UNIT_TEMPERATURE_F:	return("degF");
		case UNIT_SPEED_KMH:		return("km/h");
		case UNIT_SPEED_MPH:		return("mph");
		case UNIT_LITER:			return("l");
		case UNIT_MILLILITER:		return("ml");
		case UNIT_PERCENT:			return("%");
		case UNIT_MILLIBAR:			return("mBar");
		case UNIT_HECTOPASCAL:		return("hPa");
		case UNIT_WEIGHT_TON:		return("t");
		case UNIT_WEIGHT_GRAMS:		return("g");
		case UNIT_ENERGY_SQRM:		return("Wh/m^2");
		case UNIT_RPM:				return("rpm");
		case UNIT_HOUR:				return("h");
		case UNIT_MINUTE:			return("m");
		case UNIT_SECOND:			return("s");
		default:					return("?");
		}
}

String	sValue(double dValue, int nDecs, enUnit nUnit)
{
	switch (nUnit)
		{
		case UNIT_TEMPERATURE_C:
		case UNIT_TEMPERATURE_F:
		case UNIT_IRRADIATION_SQRM:
		case UNIT_SPEED_KMH:
		case UNIT_SPEED_MPH:
		case UNIT_LITER:
		case UNIT_MILLILITER:
		case UNIT_PERCENT:
		case UNIT_MILLIBAR:
		case UNIT_HECTOPASCAL:
		case UNIT_WEIGHT_TON:
		case UNIT_WEIGHT_GRAMS:
		case UNIT_ENERGY_SQRM:
		case UNIT_RPM:
		case UNIT_HOUR:
		case UNIT_MINUTE:
		case UNIT_SECOND:
		case UNIT_ENERGY_TOTAL:
		case UNIT_ENERGY:
			return(String::Format("%7.0f %s",dValue,pszUnit(nUnit)));
		case UNIT_POWER:
		case UNIT_CURRENT:
		case UNIT_VOLTAGE:
		case UNIT_FREQUENCY:
		case UNIT_RESISTANCE:
			return(String::Format("%7.*f %s",nDecs,dValue,pszUnit(nUnit)));
		case UNIT_UNKNOWN:	return("-");
		default:			return("?");
		case UNIT_RUNTIME_M:
			{
			double	dHours;
			double	dMinutes = modf(dValue/60.0,&dHours) * 60.0;

			return(String::Format("%d:%02d %s",(int)_round(dHours),(int)_round(dMinutes),pszUnit(nUnit)));
			}
		}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

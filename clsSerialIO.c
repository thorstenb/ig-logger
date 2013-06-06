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

#if defined(__IG__)
	#include "logig.h"
#endif

#include <stdarg.h>

#include <clsLateload.hpp>
#include <Buffer.hpp>
#include <GPFProt.hpp>

#include "clsSYSTEMTIME.h"
#include "clsExcept.h"
#include "..\logig\clsSerialIO.h"

#include "..\avr\pc\ftd2xx.h"

#if defined(__TEST__)
	#define FRAME		0x0a
	#define ESCAPE		0x1b
#endif

clsIOBase::clsIOBase(const String& sType, IDataSink* pDataStore)
	: clsThreadBase("clsRS232IO")
	, _sType(sType)
	, _bActive(false)
	, _pDataStore(pDataStore)
{
}

clsIOBase::~clsIOBase()
{
	ThreadJoin();
}

void 							clsIOBase::IOLog(const char* pszFormat, ...)
{
	String	s;
  	va_list	args;

  	va_start(args,pszFormat);
	s.VPrintf(pszFormat,args);
	va_end(args);
	#if defined(__TEST__)
		printf("%s %s: %s\n",(LPCTSTR)clsSYSTEMTIME(true).sTime(),(LPCTSTR)sDevice(),(LPCTSTR)s);
	  #else
		_chk_utildebug("%s %s: %s\n",(LPCTSTR)clsSYSTEMTIME(true).sTime(),(LPCTSTR)sDevice(),(LPCTSTR)s);
	#endif
	// _chk_utildebug("%s %s: %s",(LPCTSTR)clsSYSTEMTIME(true).sTime(),(LPCTSTR)_sType,(LPCTSTR)s);
}

void							clsIOBase::IOSetActive(bool bActive)
{
	_bActive = bActive;
}

void							clsIOBase::IONtfyData(const BYTE* pData, UINT nSize)
{
#if 1
	String	s;

	for (int n = 0; n < nSize; ++n)
		{
		s.AddStrSeparated(" ",String::Format("%02x",pData[n]));
		}
	IOLog("<--(%d) %s",nSize,(LPCTSTR)s);
#endif
	if (_bActive)
		{
		_pDataStore->NtfyData(pData,nSize);
		}
}

void							clsIOBase::IOLogWrite(const BYTE* pData, UINT nSize)
{
#if 1
	String	s;

	for (int n = 0; n < nSize; ++n)
		{
		s.AddStrSeparated(" ",String::Format("%02x",pData[n]));
		}
	IOLog("-->%s",(LPCTSTR)s);
#endif
}

//////////////////////////////////////////////////////////////////////////////

clsRS232IO::clsRS232IO(IDataSink* pDataStore, UINT nBaudrate)
	: clsIOBase("RS232",pDataStore)
	, _hDevice(INVALID_HANDLE_VALUE)
	, _nBaudrate(nBaudrate)
{
}

clsRS232IO::~clsRS232IO()
{
}

String							clsRS232IO::sDevice()
{
	return(_sPort);
}

void							clsRS232IO::IOReset()
{
	DWORD	dwState;
	COMSTAT	State;

	::ClearCommError(_hDevice,&dwState,&State);
	::PurgeComm(_hDevice,PURGE_TXCLEAR|PURGE_RXCLEAR);
	if (!::SetCommMask(_hDevice,EV_RXCHAR|EV_TXEMPTY))
		{
		throw clsExcept("ERROR: Cannot set COM port mask: %s",(LPCTSTR)String::GetLastError());
		}
}

void							clsRS232IO::IOClose()
{
	if (_hDevice != INVALID_HANDLE_VALUE)
		{
		ThreadJoin();
		::CloseHandle(_hDevice);
		_hDevice = INVALID_HANDLE_VALUE;
		}
}

bool							clsRS232IO::IOOpen(const String& sPort)
{
	IOLog("Open %s",(LPCTSTR)sPort);
	_sPort = sPort;
	_hDevice = ::CreateFile(
					String("\\\\.\\") + sPort,
					GENERIC_READ|GENERIC_WRITE,
					0, // no sharing
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,
					NULL);

	if (_hDevice == INVALID_HANDLE_VALUE)
		{
		return(false);
		}

	if (!::PurgeComm(_hDevice,PURGE_TXCLEAR|PURGE_RXCLEAR))
		{
		throw clsExcept("ERROR: Cannot purge COM queue: %s",(LPCTSTR)String::GetLastError());
		}

	if (!::SetCommMask(_hDevice,EV_RXCHAR|EV_TXEMPTY))
		{
		throw clsExcept("ERROR: Cannot set COM port mask: %s",(LPCTSTR)String::GetLastError());
		}

	//now we need to set baud rate etc,
	DCB dcb = {0};

	dcb.DCBlength = sizeof(DCB);

	if (!::GetCommState(_hDevice,&dcb))
		{
		throw clsExcept("ERROR: Cannot get COM state: %s",(LPCTSTR)String::GetLastError());
		}

	dcb.BaudRate	= _nBaudrate;
	dcb.ByteSize	= 8;
	dcb.Parity		= NOPARITY;
	dcb.fParity		= (dcb.fParity != NOPARITY) ? 1 : 0;
	dcb.fBinary		= 1;
	dcb.fNull		= FALSE;
	dcb.fDtrControl	= DTR_CONTROL_ENABLE;
	dcb.fRtsControl	= RTS_CONTROL_ENABLE;
	dcb.StopBits	= ONESTOPBIT;
	dcb.fDsrSensitivity = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.ErrorChar 	= 0;
	dcb.EofChar 	= 0;
	dcb.EvtChar 	= 0;
	dcb.XonChar 	= 0;
	dcb.XoffChar 	= 0;

	if (!::SetCommState(_hDevice,&dcb))
		{
		throw clsExcept("ERROR: Cannot set COM state: %s",(LPCTSTR)String::GetLastError());
		}

	//now set the timeouts ( we control the timeout overselves using WaitForXXX()
	COMMTIMEOUTS timeouts;

	timeouts.ReadIntervalTimeout			= MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier		= 0;
	timeouts.ReadTotalTimeoutConstant		= 0;
	timeouts.WriteTotalTimeoutMultiplier	= 0;
	timeouts.WriteTotalTimeoutConstant		= 0;

	if (!::SetCommTimeouts(_hDevice,&timeouts))
		{
		throw clsExcept("ERROR: Cannot set COM timeouts: %s",(LPCTSTR)String::GetLastError());
		}

	ThreadStart(THREAD_PRIORITY_ABOVE_NORMAL);
	return(true);
}

virtual DWORD					clsRS232IO::ThreadFunction(void)
{
	bool		bAbort(false);
	clsEvent	evCOMSignal(TRUE,FALSE);
	OVERLAPPED	ov = {0};
	bool		bOverlappedIOPending(false);

	ov.hEvent = evCOMSignal.Handle();

	while (!bAbort)
		{
		// IOLog("Wait");
		DWORD	dwEventMask(0);
		bool	bProcess(false);

		if (!bOverlappedIOPending)
			{
			// _chk_utildebug(">WaitCommEvent");
			if (::WaitCommEvent(_hDevice,&dwEventMask,&ov))
				{
				// _chk_utildebug("< -> process immediately");
				bProcess = true;
				}
			  else
				{
				if (GetLastError() != ERROR_IO_PENDING)
					{
					IOLog("ERROR: Cannot get COM event: %s",(LPCTSTR)String::GetLastError());
					continue;
					}
				  else
					{
					// _chk_utildebug("< -> pending");
					bOverlappedIOPending = true;
					}
				}
			}
		if (bOverlappedIOPending)
		  	{
			INT		nEvent;

			// _chk_utildebug(">WaitSignal");
			if (!ThreadWait(5000,nEvent,2,evCOMSignal.Handle(),hAbort()))
				{
				IOLog("End");
				break;
				}
			// _chk_utildebug("< -> %d",nEvent);

			switch (nEvent)
				{
				case -1:
					#if !defined(NO_HEARTBEAT)
					// timeout
					IOLog("FATAL: cannot read from COMM port (heartbeat missing!): %s",(LPCTSTR)String::GetLastError());
					return(1);
					#else
					break;
					#endif
				default:
					IOLog("unexpected thread event... %x",nEvent);
					return(1);
				case 1:
					IOLog("abort");
					bAbort = true;
					break;
				case 0: // comm event
					bOverlappedIOPending = false;
					bProcess = true;
					break;
				}
			}

		if (bProcess)
			{
			if (dwEventMask & EV_TXEMPTY)
				{
				IOLog("(EV_TXEMPTY: data sent)");
				}
			if (dwEventMask & EV_RXCHAR)
				{
				IOLog("(EV_RXCHAR: data received)");
				DWORD 		nBytesRead(0);

				if (::GetOverlappedResult(_hDevice,&ov,&nBytesRead,true))
					{
					// _chk_utildebug("GetOverlappedResult: %d bytes",nBytesRead);
					BYTE	szBuffer[128];

					while (nBytesRead)
						{
						BOOL	abRet = ::ReadFile(_hDevice,szBuffer,sizeof(szBuffer),&nBytesRead,&ov);
						if (!abRet)
							{
							IOLog("ReadFile failed: %s",(LPCTSTR)String::GetLastError());
							break;
							}
						if (nBytesRead > 0)
							{
							IONtfyData(szBuffer,nBytesRead);
							}
						}
					}
				}
			}
		}

	return(0);
}

bool							clsRS232IO::IOWrite(BYTE* pData, int nSize)
{
	if (_hDevice == INVALID_HANDLE_VALUE)
		{
		throw clsExcept("ERROR: Write() called, but device not opened yet");
		}

	IOLogWrite(pData,nSize);

	int 		iRet = 0;
	OVERLAPPED 	ov;

	ZEROINIT(ov);

	DWORD dwBytesWritten = 0;
	// _chk_utildebug(">write %d bytes",nSize);
	iRet = ::WriteFile(_hDevice,pData,nSize,&dwBytesWritten,&ov);
	// _chk_utildebug("<write %d bytes -> %d (written: %d)",nSize,iRet,dwBytesWritten);

	if (iRet == 0)
		{
		if (GetLastError() == ERROR_IO_PENDING)
			{
			// _chk_utildebug(">pending...");
			if (::GetOverlappedResult(_hDevice,&ov,&dwBytesWritten,TRUE))
				{
				// _chk_utildebug("<pending... OK (%d)",dwBytesWritten);
				iRet = 1;
				}
			  else
				{
				throw clsNotifyExcept("cannot write to COMM port: %s",(LPCTSTR)String::GetLastError());
				}
			}
		  else
			{
			throw clsNotifyExcept("cannot write to COMM port: %s",(LPCTSTR)String::GetLastError());
			}
		}
	return(iRet != 0);
}

//////////////////////////////////////////////////////////////////////////////

LATELOAD_BEGIN_CLASS(clsSetupapi,setupapi,FALSE,TRUE)
	LATELOAD_FUNC_4(INVALID_HANDLE_VALUE,HDEVINFO,WINAPI,		SetupDiGetClassDevs, CONST GUID*, LPCTSTR, HWND, DWORD)
	LATELOAD_FUNC_1_VOID(WINAPI,								SetupDiDestroyDeviceInfoList, HDEVINFO)
	LATELOAD_FUNC_5(FALSE,BOOL,WINAPI,							SetupDiEnumDeviceInterfaces, HDEVINFO, SP_DEVINFO_DATA*, CONST GUID*, DWORD, SP_DEVICE_INTERFACE_DATA*)
	LATELOAD_FUNC_6(FALSE,BOOL,WINAPI,							SetupDiGetDeviceInterfaceDetail, HDEVINFO, SP_DEVICE_INTERFACE_DATA*, SP_DEVICE_INTERFACE_DETAIL_DATA*, DWORD, DWORD*, SP_DEVINFO_DATA*)
	LATELOAD_FUNC_7(FALSE,BOOL,WINAPI,							SetupDiGetDeviceRegistryProperty, HDEVINFO, SP_DEVINFO_DATA*, DWORD, DWORD*, PBYTE, DWORD, DWORD*)
	LATELOAD_FUNC_2(FALSE,BOOL,WINAPI,							SetupDiGetDeviceInfoListClass, HDEVINFO, GUID*)
	LATELOAD_FUNC_3(FALSE,BOOL,WINAPI,							SetupDiEnumDeviceInfo, HDEVINFO, DWORD, SP_DEVINFO_DATA*)
	LATELOAD_FUNC_5(FALSE,BOOL,WINAPI,							SetupDiGetDeviceInstanceId, HDEVINFO, SP_DEVINFO_DATA*, LPTSTR, DWORD, DWORD*)
LATELOAD_END_CLASS()

static	clsSetupapi	__SetupAPI;

clsUSBIO::clsUSBIO(IDataSink* pDataStore)
	: clsIOBase("USB",pDataStore)
{
}

clsUSBIO::~clsUSBIO()
{
}

String							clsUSBIO::sDevice()
{
	return(_Device.sDescription());
}

void							clsUSBIO::IOClose()
{
	if (_Device.IsOk())
		{
		IOLog("Close");
		ThreadJoin();
		_Device.Clear();
		}
}

bool							clsUSBIO::IOOpen(const String& sPort)
{
	IOLog("Open %s",(LPCTSTR)sPort);
	if (!FindUSBDeviceOfGUID(sPort,_Device))
		{
		return(false);
		}

	ThreadStart(THREAD_PRIORITY_ABOVE_NORMAL);
	return(true);
}

void							clsUSBIO::NtfyData(const BYTE* pszData, UINT nSize)
{
	if (nSize > 0)
		{
		IONtfyData(pszData,nSize);
		}
}

virtual DWORD					clsUSBIO::ThreadFunction(void)
{
	clsEvent	evDataReady(TRUE,FALSE);
	OVERLAPPED	ovRead = {0};
	bool		bAborted = false;

	ovRead.hEvent = evDataReady.Handle();

	while (!bAborted)
		{
		BYTE 	szTmp[1024];
		DWORD	dwBytesRead(0);

		szTmp[0] = 0;
		if (::ReadFile(_Device.Handle(),szTmp,sizeof(szTmp),&dwBytesRead,&ovRead))
			{
			if (dwBytesRead == 0)
				{
				IOLog("USB port did close unexpectedly (EOF): %s",(LPCTSTR)String::GetLastError());
				return(1);
				}
			IONtfyData(szTmp,dwBytesRead);
			}
		  else
			{
			if (GetLastError() == ERROR_IO_PENDING)
				{
				INT		nEvent;

				if (ThreadWait(2000, nEvent, 2, ovRead.hEvent, hAbort()))
					{
					switch (nEvent)
						{
						case THREADEVENT_TIMEOUT:
							IOLog("USB::Read::Timeout");
							return(1);
						case THREADEVENT_ERROR:
							IOLog("USB::Read::Error");
							return(1);
						case 0:
							if (!GetOverlappedResult(_Device.Handle(),&ovRead,&dwBytesRead,FALSE))
								{
								IOLog("USB port did close unexpectedly: %s",(LPCTSTR)String::GetLastError());
								return(1);
								}
							IONtfyData(szTmp,dwBytesRead);
							break;
						case 1:
							bAborted = true;
							break;
						}
					}
				  else
					{
					IOLog("USB::Read::Error (2)");
					bAborted = true;
					}
				evDataReady.Reset();
				}
			  else
			  	{
				IOLog("USB port issues error: %s",(LPCTSTR)String::GetLastError());
			  	}

			}
		}

	return(0);
}

bool							clsUSBIO::IOWrite(BYTE* pData, int nSize)
{
	if (!_Device.IsOk())
		{
		throw clsExcept("ERROR: Write() called, but device not opened yet");
		}

	IOLogWrite(pData,nSize);

	int 		iRet = 0;
	OVERLAPPED 	ov;

	ZEROINIT(ov);

	DWORD dwBytesWritten = 0;
	iRet = ::WriteFile(_Device.Handle(),pData,nSize,&dwBytesWritten,&ov);

	if (iRet == 0)
		{
		if (GetLastError() == ERROR_IO_PENDING)
			{
			if (GetOverlappedResult(_Device.Handle(),&ov,&dwBytesWritten,TRUE))
				{
				iRet = 1;
				}
			  else
				{
				throw clsNotifyExcept("cannot write to USB port: %s",(LPCTSTR)String::GetLastError());
				}
			}
		  else
			{
			throw clsNotifyExcept("cannot write to USB port: %s",(LPCTSTR)String::GetLastError());
			}
		}
	return(iRet != 0);
}

bool							clsUSBIO::FindUSBDeviceOfGUID(String sDevice, clsIODevice& Device)
{
	GUID						guidInterface;

	CLSIDFromString(L"{a5dcbf10-6530-11d2-901f-00c04fb951ed}",&guidInterface);

	HDEVINFO	hDevInfo = __SetupAPI.SetupDiGetClassDevs(
		&guidInterface,
		NULL,
		0,
		DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

	if (hDevInfo != INVALID_HANDLE_VALUE)
		{
		SP_DEVICE_INTERFACE_DATA	DeviceInterfaceData;

		ZEROINIT_(DeviceInterfaceData,cbSize);

		for (int nInterface = 0;
			__SetupAPI.SetupDiEnumDeviceInterfaces(hDevInfo,
				NULL,
				&guidInterface,
				nInterface,
				&DeviceInterfaceData) &&
			!Device.IsOk(); // get only first device! I assume there's only one.
			++nInterface)
			{
			DWORD	dwSize;
			SP_DEVINFO_DATA DeviceInfoData;

			ZEROINIT_(DeviceInfoData,cbSize);

			__SetupAPI.SetupDiGetDeviceInterfaceDetail(hDevInfo,&DeviceInterfaceData,NULL,0,&dwSize,NULL);

			SP_DEVICE_INTERFACE_DETAIL_DATA*	pDetail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)(new BYTE[dwSize]);
			pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			if (__SetupAPI.SetupDiGetDeviceInterfaceDetail(hDevInfo,&DeviceInterfaceData,pDetail,dwSize,&dwSize,&DeviceInfoData))
				{
				String	sDevicePath(pDetail->DevicePath);

				/*
				printf("------------------------\n");
				printf(" ID='%s'\n",(LPCTSTR)GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_DEVICEDESC));
				printf(" Mfg='%s'\n",(LPCTSTR)GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_MFG));
				printf(" Name='%s'\n",(LPCTSTR)GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_FRIENDLYNAME));
				printf(" PhysName='%s'\n",(LPCTSTR)GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME));
				printf(" DevType='%s'\n",(LPCTSTR)GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_DEVTYPE));
				printf(" Driver='%s'\n",(LPCTSTR)GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_DRIVER));
				printf(" path='%s'\n",(LPCTSTR)sDevicePath);
				*/

				if (GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_DEVICEDESC) == sDevice)
					{
					HANDLE	hFile = ::CreateFile(
						sDevicePath,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						NULL);
					if (hFile != INVALID_HANDLE_VALUE)
						{
						Device.Set(
							hFile,
							GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_DEVICEDESC),
							GetDeviceDescProperty(hDevInfo,DeviceInfoData,SPDRP_MFG));
						}
					  else
						{
						IOLog("open error: %s",(LPCTSTR)String::GetLastError());
						}
					}
				}
			delete[] (BYTE*)pDetail;
			}

		__SetupAPI.SetupDiDestroyDeviceInfoList(hDevInfo);
		}
	return(Device.IsOk());
}

String 							clsUSBIO::GetDeviceDescProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA& DeviceInfoData, DWORD nSPDRP)
{
	String		sResult;
	DWORD 		DataT;
	clsBuffer	buf(100);
	DWORD		dwBufferSize(100);

	//
	// Call function with null to begin with,
	// then use the returned buffer size (doubled)
	// to Alloc the buffer. Keep calling until
	// success or an unknown failure.
	//
	//  Double the returned buffersize to correct
	//  for underlying legacy CM functions that
	//  return an incorrect buffersize value on
	//  DBCS/MBCS systems.
	//
	while (!__SetupAPI.SetupDiGetDeviceRegistryProperty(
		hDevInfo,
		&DeviceInfoData,
		nSPDRP,
		&DataT,
		(PBYTE)buf.Adr(),
		buf.Size(),
		&dwBufferSize))
		{
		if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
			// Change the buffer size.
			// Double the size to avoid problems on
			// W2k MBCS systems per KB 888609.
			buf.Resize(dwBufferSize * 2);
			}
		  else
			{
			// Insert error handling here.
			break;
			}
		}
	switch (DataT)
		{
		case REG_SZ:
			sResult = (LPCSTR)buf;
			break;
		}

	return(sResult);
}

//////////////////////////////////////////////////////////////////////////////

LATELOAD_BEGIN_CLASS(clsFTDI,ftd2xx,FALSE,TRUE)
	LATELOAD_FUNC_3(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_ListDevices, 			void*, void*, DWORD)
	LATELOAD_FUNC_2(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_Open, 					int, FT_HANDLE*)
	LATELOAD_FUNC_1(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_GetLibraryVersion,		DWORD*)
	LATELOAD_FUNC_4(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_Read, 					FT_HANDLE, void*, DWORD, LPDWORD)
	LATELOAD_FUNC_4(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_Write, 					FT_HANDLE, void*, DWORD, LPDWORD)
	LATELOAD_FUNC_2(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_Purge, 					FT_HANDLE, ULONG)
	LATELOAD_FUNC_2(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_SetBaudRate, 			FT_HANDLE, ULONG)
	LATELOAD_FUNC_4(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_SetDataCharacteristics, 	FT_HANDLE, UCHAR, UCHAR, UCHAR)
	LATELOAD_FUNC_4(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_SetFlowControl, 			FT_HANDLE, USHORT, UCHAR, UCHAR)
	LATELOAD_FUNC_1(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_ResetDevice, 			FT_HANDLE)
	LATELOAD_FUNC_1(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_ResetPort, 				FT_HANDLE)
	LATELOAD_FUNC_1(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_CyclePort, 				FT_HANDLE)
	LATELOAD_FUNC_5(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_SetChars, 				FT_HANDLE, UCHAR, UCHAR, UCHAR, UCHAR)
	LATELOAD_FUNC_3(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_SetTimeouts, 			FT_HANDLE, ULONG, ULONG)
	LATELOAD_FUNC_2(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_GetDriverVersion,		FT_HANDLE, DWORD*)
	LATELOAD_FUNC_6(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_GetDeviceInfo,			FT_HANDLE, FT_DEVICE*, DWORD*, CHAR*, CHAR*, void*)
	LATELOAD_FUNC_3(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_SetEventNotification,	FT_HANDLE, DWORD, HANDLE)
	LATELOAD_FUNC_2(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_GetQueueStatus,			FT_HANDLE, DWORD*)
	LATELOAD_FUNC_1(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_StopInTask, 				FT_HANDLE)
	LATELOAD_FUNC_1(FT_OTHER_ERROR,FT_STATUS,FTAPI,				FT_RestartInTask, 	   		FT_HANDLE)


	LATELOAD_FUNC_7(INVALID_HANDLE_VALUE,FT_HANDLE,FTAPI,		FT_W32_CreateFile, LPCSTR, DWORD, DWORD, SECURITY_ATTRIBUTES*, DWORD, DWORD, HANDLE)
	LATELOAD_FUNC_1(FALSE,BOOL,FTAPI,							FT_W32_CloseHandle, 		FT_HANDLE)
	LATELOAD_FUNC_5(FALSE,BOOL,FTAPI,							FT_W32_ReadFile, 			FT_HANDLE, void*, DWORD, DWORD*, OVERLAPPED*)
	LATELOAD_FUNC_5(FALSE,BOOL,FTAPI,							FT_W32_WriteFile, 			FT_HANDLE, void*, DWORD, DWORD*, OVERLAPPED*)
	LATELOAD_FUNC_1(0,DWORD,FTAPI,	 							FT_W32_GetLastError,	  	FT_HANDLE)
	LATELOAD_FUNC_4(FALSE,BOOL,FTAPI,							FT_W32_GetOverlappedResult,FT_HANDLE, OVERLAPPED*, DWORD*, BOOL)
	LATELOAD_FUNC_2(FALSE,BOOL,FTAPI,							FT_W32_GetCommState,		FT_HANDLE, FTDCB*)
	LATELOAD_FUNC_2(FALSE,BOOL,FTAPI,							FT_W32_SetCommState,		FT_HANDLE, FTDCB*)
	LATELOAD_FUNC_2(FALSE,BOOL,FTAPI,							FT_W32_SetCommMask,		FT_HANDLE, DWORD)
	LATELOAD_FUNC_2(FALSE,BOOL,FTAPI,							FT_W32_SetCommTimeouts,	FT_HANDLE, FTTIMEOUTS*)
	LATELOAD_FUNC_3(FALSE,BOOL,FTAPI,							FT_W32_WaitCommEvent,		FT_HANDLE, DWORD*, OVERLAPPED*)
	LATELOAD_FUNC_1(FALSE,BOOL,FTAPI,							FT_W32_CancelIo,			FT_HANDLE)
LATELOAD_END_CLASS()

static	clsFTDI	__FTDI;

clsFTDIIOBase::clsFTDIIOBase(const String& sType, IDataSink* pDataStore, UINT nBaudrate)
	: clsIOBase(sType,pDataStore)
	, _nDevice(-1)
	, _hDevice(NULL)
	, _nBaudrate(nBaudrate)
{
}

clsFTDIIOBase::~clsFTDIIOBase()
{
	IOLog(TEXT("clsFTDIIOBase::~clsFTDIIOBase"));
	IOClose();
}

String							clsFTDIIOBase::sDevice()
{
	return(String::Format("%s:%s",(LPCTSTR)_sType,(LPCTSTR)_sPort));
}

void							clsFTDIIOBase::IOReset()
{
	if (_hDevice)
		{
		__FTDI.FT_Purge(_hDevice, FT_PURGE_TX | FT_PURGE_RX);
		__FTDI.FT_ResetDevice(_hDevice);
		/*
		__FTDI.FT_SetBaudRate(_hDevice, _nBaudrate);
		__FTDI.FT_SetDataCharacteristics(_hDevice, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		__FTDI.FT_SetFlowControl(_hDevice, FT_FLOW_RTS_CTS, 0, 0);
		__FTDI.FT_SetChars(_hDevice, 0, 0, 0, 0);
		__FTDI.FT_SetTimeouts(_hDevice, 100, 100);
		*/
		}
}

void							clsFTDIIOBase::IOClose()
{
	if (_hDevice)
		{
		IOLog(TEXT("clsFTDIIOBase::IOClose()"));
		ThreadJoin();
		__FTDI.FT_Purge(_hDevice, FT_PURGE_TX);
		__FTDI.FT_ResetDevice(_hDevice);
		IOCloseHandle();
		_hDevice = NULL;
		_nDevice = -1;
		}
}

//////////////////////////////////////////////////////////////////////////////

clsFTDIIO::clsFTDIIO(IDataSink* pDataStore, UINT nBaudrate)
	: clsFTDIIOBase("FTDI",pDataStore,nBaudrate)
{
}

bool							clsFTDIIO::IOOpen(const String& sDevice)
{
	DWORD	nVersion;

	if (__FTDI.FT_GetLibraryVersion(&nVersion) == FT_OK)
		{
		IOLog("FTDI library version %d.%02d.%02d",
			(nVersion >> 16) & 0xff,
			(nVersion >> 8) & 0xff,
			nVersion & 0xff);
		}

	IOLog("Open %s",(LPCTSTR)sDevice);
	_sPort = sDevice;
	DWORD	nDevices = 0;

	if (__FTDI.FT_ListDevices(&nDevices,NULL,FT_LIST_NUMBER_ONLY) == FT_OK)
		{
		IOLog("number of FTDI devices: %d",nDevices);

		for (int nDevice = 0; nDevice < nDevices; ++nDevice)
			{
			char	buf[80];

			if (__FTDI.FT_ListDevices((void*)nDevice,buf,FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION) == FT_OK)
				{
				IOLog(" %d: '%s'",nDevice,buf);
				if (sDevice.icmp(buf) == 0)
					{
					_nDevice = nDevice;
					}
				}
			}
		}

	IOLog("device ID=%d",_nDevice);
	if (_nDevice >= 0)
		{
		if (__FTDI.FT_Open(_nDevice, &_hDevice) != FT_OK)
			{
			_nDevice = -1;
			throw clsExcept("cannot open FTDI device %d (%s)",_nDevice,(LPCTSTR)sDevice);
			}

		if (__FTDI.FT_GetDriverVersion(_hDevice,&nVersion) == FT_OK)
			{
			IOLog("FTDI driver  version %d.%02d.%02d",
				(nVersion >> 16) & 0xff,
				(nVersion >> 8) & 0xff,
				nVersion & 0xff);
			}

		DWORD	nType;
		DWORD	nID;
		CHAR	szSerial[16+1];
		CHAR	szDescription[64+1];

		if (__FTDI.FT_GetDeviceInfo(_hDevice,&nType,&nID,szSerial,szDescription,NULL) == FT_OK)
			{
			IOLog("FTDI device  version %d.%02d.%02d ('%s')",
				(nVersion >> 16) & 0xff,
				(nVersion >> 8) & 0xff,
				nVersion & 0xff,
				szDescription);
			}
		__FTDI.FT_Purge(_hDevice, FT_PURGE_TX | FT_PURGE_RX);
		__FTDI.FT_ResetDevice(_hDevice);
		__FTDI.FT_SetBaudRate(_hDevice, _nBaudrate);
		__FTDI.FT_SetDataCharacteristics(_hDevice, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		__FTDI.FT_SetFlowControl(_hDevice, FT_FLOW_RTS_CTS, 0, 0);
		__FTDI.FT_SetChars(_hDevice, 0, 0, 0, 0);
		__FTDI.FT_SetTimeouts(_hDevice, 100, 100);

		ThreadStart(THREAD_PRIORITY_ABOVE_NORMAL);
		}
	return(_nDevice >= 0);
}

void							clsFTDIIO::IOCloseHandle()
{
	::CloseHandle(_hDevice);
}

virtual DWORD					clsFTDIIO::ThreadFunction(void)
{
	clsEvent	evCharReceived(false);
	INT			nEvent(0);
	DWORD		dwTimeout(500);

	if (__FTDI.FT_SetEventNotification(_hDevice,FT_EVENT_RXCHAR,evCharReceived.Handle()) != FT_OK)
		{
		IOLog("FT_SetEventNotification did not work, switch to polling mode");
		dwTimeout = 50; // polling
		}

	while (ThreadWait(dwTimeout,nEvent,1,evCharReceived.Handle()))
		{
		IOLog("Thread: event %d",nEvent);
		BYTE 		szTmp[100];
		DWORD		dwBytesInQueue(0);
		FT_STATUS	nFTDIState;
		clsCritObj	CO(_FTDIAccess);

		if ((nFTDIState = __FTDI.FT_GetQueueStatus(_hDevice, &dwBytesInQueue)) == FT_OK)
			{
			// IOLog("%d bytes waiting",dwBytesInQueue);
			if (dwBytesInQueue > 0)
				{
				DWORD	dwBytesRead(0);

				if (__FTDI.FT_Read(_hDevice, szTmp, min(dwBytesInQueue,sizeof(szTmp)), &dwBytesRead) == FT_OK)
					{
					if (dwBytesRead > 0)
						{
						IONtfyData(szTmp,dwBytesRead);
						}
					}
				}
			}
		  else
			{
			switch (nFTDIState)
				{
				case FT_IO_ERROR:
					IOLog("FT err: FT_IO_ERROR",nFTDIState);
					__FTDI.FT_ResetPort(_hDevice);
					break;
				default:
					IOLog("FT err: %d",nFTDIState);
					break;
				}
			}
		}
	return(0);
}

bool							clsFTDIIO::IOWrite(BYTE* pData, int nSize)
{
	if (_hDevice == NULL)
		{
		throw clsExcept("ERROR: Write() called, but device not opened yet");
		}

	IOLogWrite(pData,nSize);

	clsCritObj	CO(_FTDIAccess);
	ULONG		nDummy;

	return(__FTDI.FT_Write(_hDevice,pData,nSize,&nDummy) && nDummy > 0);
}

//////////////////////////////////////////////////////////////////////////////

clsFTDIW32IO::clsFTDIW32IO(IDataSink* pDataStore, UINT nBaudrate)
	: clsFTDIIOBase("FTDIW32",pDataStore,nBaudrate)
{
}

#define FTDI_Check(cmd,args) \
	if ((ftStatus = cmd args) != FT_OK)	\
		{					\
		_chk_utildebug("%s failed: 0x%x",#cmd,ftStatus);	\
		}


bool							clsFTDIW32IO::IOOpen(const String& sPort)
{
	DWORD	nVersion;

	if (__FTDI.FT_GetLibraryVersion(&nVersion) == FT_OK)
		{
		IOLog("FTDI library version %d.%02d.%02d",
			(nVersion >> 16) & 0xff,
			(nVersion >> 8) & 0xff,
			nVersion & 0xff);
		}

	IOLog("Open '%s'",(LPCTSTR)sPort);
	_sPort = sPort;
	_hDevice = __FTDI.FT_W32_CreateFile(
					sPort,
					GENERIC_READ|GENERIC_WRITE,
					0, // no sharing
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED | FT_OPEN_BY_DESCRIPTION,
					NULL);

	if (_hDevice == INVALID_HANDLE_VALUE)
		{
		IOLog("Open '%s' failed...",(LPCTSTR)sPort);
		return(false);
		}

	if (__FTDI.FT_GetDriverVersion(_hDevice,&nVersion) == FT_OK)
		{
		IOLog("FTDI driver  version %d.%02d.%02d",
			(nVersion >> 16) & 0xff,
			(nVersion >> 8) & 0xff,
			nVersion & 0xff);
		}

	DWORD		nType;
	DWORD		nID;
	CHAR		szSerial[16+1];
	CHAR		szDescription[64+1];

	if (__FTDI.FT_GetDeviceInfo(_hDevice,&nType,&nID,szSerial,szDescription,NULL) == FT_OK)
		{
		IOLog("FTDI device  version %d.%02d.%02d ('%s', serial# '%s')",
			(nVersion >> 16) & 0xff,
			(nVersion >> 8) & 0xff,
			nVersion & 0xff,
			szDescription,
			szSerial);
		}

	FT_STATUS 	ftStatus;

	do
		{
		ftStatus = __FTDI.FT_StopInTask(_hDevice);
		if (ftStatus != FT_OK)
			{
			_chk_utildebug("FT_StopInTask returned %x",ftStatus);
			}
		}
	while (ftStatus != FT_OK);

	FTDI_Check(__FTDI.FT_Purge,(_hDevice, FT_PURGE_TX | FT_PURGE_RX));
	if (!__FTDI.FT_W32_SetCommMask(_hDevice,EV_RXCHAR|EV_TXEMPTY))
		{
		throw clsExcept("ERROR: Cannot set COM port mask: %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
		}

	//now we need to set baud rate etc,
	FTDCB dcb = {0};

	dcb.DCBlength = sizeof(DCB);

	if (!__FTDI.FT_W32_GetCommState(_hDevice,&dcb))
		{
		throw clsExcept("ERROR: Cannot get COM state: %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
		}

	dcb.BaudRate	= _nBaudrate;
	dcb.ByteSize	= 8;
	dcb.Parity		= NOPARITY;
	dcb.fParity		= (dcb.fParity != NOPARITY) ? 1 : 0;
	dcb.fBinary		= 1;
	dcb.fNull		= FALSE;
	#if 0 // NAME==logig
	dcb.fRtsControl	= RTS_CONTROL_HANDSHAKE;
	dcb.fOutxDsrFlow = TRUE;
	#else
	dcb.fDtrControl	= DTR_CONTROL_ENABLE;
	dcb.fRtsControl	= RTS_CONTROL_ENABLE;
	#endif
	dcb.StopBits	= ONESTOPBIT;
	dcb.fDsrSensitivity = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.ErrorChar 	= 0;
	dcb.EofChar 	= 0;
	dcb.EvtChar 	= 0;
	dcb.XonChar 	= 0;
	dcb.XoffChar 	= 0;

	if (!__FTDI.FT_W32_SetCommState(_hDevice,&dcb))
		{
		throw clsExcept("ERROR: Cannot set COM state: %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
		}

	//now set the timeouts ( we control the timeout overselves using WaitForXXX()
	FTTIMEOUTS timeouts;

	timeouts.ReadIntervalTimeout			= MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier		= 1;
	timeouts.ReadTotalTimeoutConstant		= 100;
	timeouts.WriteTotalTimeoutMultiplier	= 1;
	timeouts.WriteTotalTimeoutConstant		= 1000;

	if (!__FTDI.FT_W32_SetCommTimeouts(_hDevice,&timeouts))
		{
		throw clsExcept("ERROR: Cannot set COM timeouts: %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
		}
	do
		{
		ftStatus = __FTDI.FT_RestartInTask(_hDevice);
		if (ftStatus != FT_OK)
			{
			_chk_utildebug("FT_RestartInTask returned %x",ftStatus);
			}
		}
	while (ftStatus != FT_OK);

	ThreadStart(THREAD_PRIORITY_ABOVE_NORMAL);
	return(true);
}

void							clsFTDIW32IO::IOCloseHandle()
{
	__FTDI.FT_W32_CloseHandle(_hDevice);
}

virtual DWORD					clsFTDIW32IO::ThreadFunction(void)
{
	OVERLAPPED	ov = {0};
	bool		bAbort(false);
	clsEvent	evCOMSignal(false);
	bool		bWaitingForData(false);
	BYTE 		szTmp[62];
	DWORD		dwBytesRead(0);

	ov.hEvent = evCOMSignal.Handle();

	while (!bAbort)
		{
		// IOLog("Wait(%d)",bWaitingForData);
		if (!bWaitingForData)
			{
			if (__FTDI.FT_W32_ReadFile(_hDevice,szTmp,sizeof(szTmp),&dwBytesRead,&ov))
				{
				// IOLog("read(1) %d\n",dwBytesRead);
				if (dwBytesRead > 0)
					{
					IONtfyData(szTmp,dwBytesRead);
					}
				}
			  else
				{
				if (__FTDI.FT_W32_GetLastError(_hDevice) != ERROR_IO_PENDING)
					{
					IOLog("read error (1): %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
					__FTDI.FT_ResetPort(_hDevice);
					break;
					}
				bWaitingForData = true;
				}
			}

		if (bWaitingForData)
			{
			INT		nEvent;

			if (!ThreadWait(5000,nEvent,1,evCOMSignal.Handle()))
				{
				IOLog("End");
				bAbort = true;
				break;
				}
			switch (nEvent)
				{
				case -1:
					#if !defined(NO_HEARTBEAT)
					// timeout
					IOLog("FATAL: cannot read from COMM port (heartbeat missing!): %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
					return(1);
					#else
					break;
					#endif
				default:
					IOLog("unexpected thread event... %x",nEvent);
					return(1);
				case 0: // comm event
					if (!__FTDI.FT_W32_GetOverlappedResult(_hDevice, &ov, &dwBytesRead, TRUE))
						{
						__FTDI.FT_ResetPort(_hDevice);
						IOLog("read error (2): %s",(LPCTSTR)String::GetSysErrorString(__FTDI.FT_W32_GetLastError(_hDevice)));
						}
					// IOLog("read(2) %d\n",dwBytesRead,_bActive);
					if (dwBytesRead > 0)
						{
						IONtfyData(szTmp,dwBytesRead);
						}
					bWaitingForData = false;
					break;
				}
			}
		}

	if (bWaitingForData)
		{
		__FTDI.FT_W32_CancelIo(_hDevice);
		}

	return(0);
}

bool							clsFTDIW32IO::IOWrite(BYTE* pData, int nSize)
{
	if (_hDevice == INVALID_HANDLE_VALUE)
		{
		throw clsExcept("ERROR: Write() called, but device not opened yet");
		}

	IOLogWrite(pData,nSize);

	int 		iRet = 0;
	DWORD 		dwBytesWritten = 0;
	OVERLAPPED 	ov;

	ZEROINIT(ov);

	iRet = __FTDI.FT_W32_WriteFile(_hDevice,pData,nSize,&dwBytesWritten,&ov);

	if (iRet == 0)
		{
		if (GetLastError() == ERROR_IO_PENDING)
			{
			if (!__FTDI.FT_W32_GetOverlappedResult(_hDevice, &ov, &dwBytesWritten, TRUE))
				{
				__FTDI.FT_ResetPort(_hDevice);
				throw clsNotifyExcept("cannot write to FTDI port (2): %s",(LPCTSTR)String::GetLastError());
				}
			iRet = 1;
			}
		  else
			{
			__FTDI.FT_ResetPort(_hDevice);
			throw clsNotifyExcept("cannot write to FTDI port: %s",(LPCTSTR)String::GetLastError());
			}
		}

	return(iRet != 0);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

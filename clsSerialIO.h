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

#if !defined(_INC_SERIALIO)
	#define _INC_SERIALIO

#include "\dev\h\ms\setupapi.h"

interface IDataSink
	{
		virtual void					NtfyData(const BYTE* pData, UINT nSize) = 0;
	};

class clsIOBase
	: public clsThreadBase
	{
	protected:
		String							_sType;
		bool							_bActive;
		IDataSink*						_pDataStore;

		void							IOLog(const char* s, ...) ;
		virtual DWORD					StackSize(void)
											{
											return(0x10000);
											}
		void							IONtfyData(const BYTE* pData, UINT nSize);
		void							IOLogWrite(const BYTE* pData, UINT nSize);
	public:
										clsIOBase(const String& sType, IDataSink* pDataStore);
		virtual 						~clsIOBase();

		virtual void					IOSetActive(bool bActive);
		virtual void					IOReset() = 0;
		virtual bool		  			IOOpen(const String& sDevice) = 0;
		virtual void					IOClose() = 0;
		virtual bool					IOWrite(BYTE* pData, int nSize) = 0;
		virtual String					sDevice() = 0;
	};

class clsRS232IO
	: public clsIOBase
	{
	private:
		String							_sPort;
		HANDLE							_hDevice;
		UINT 							_nBaudrate;

		virtual DWORD					ThreadFunction(void);
	public:
										clsRS232IO(IDataSink* pDataStore, UINT nBaudrate);
										~clsRS232IO();

		virtual void					IOReset();
		virtual bool					IOOpen(const String& sDevice);
		virtual void					IOClose();

		virtual bool					IOWrite(BYTE* pData, int nSize);
		virtual String					sDevice();
	};

//////////////////////////////////////////////////////////////////////////////

class clsIODevice
	{
	private:
		HANDLE							_hDevice;
		String							_sDescription;
		String							_sManufacturer;
	public:
										clsIODevice()
											: _hDevice(INVALID_HANDLE_VALUE)
											{
											}
										~clsIODevice()
											{
											Clear();
											}

		void							Set(	HANDLE 	hDevice,
												String	sDescription,
												String	sManufacturer)
											{
											_hDevice = hDevice;
											_sDescription = sDescription;
											_sManufacturer = sManufacturer;
											}
		void							Clear()
											{
											if (_hDevice != INVALID_HANDLE_VALUE)
												{
												::CloseHandle(_hDevice);
												_hDevice = INVALID_HANDLE_VALUE;
												}
											}
		bool							IsOk() const
											{
											return(_hDevice != INVALID_HANDLE_VALUE);
											}
		HANDLE							Handle() const
											{
											return(_hDevice);
											}
		String							sDescription()
											{
											return(_sDescription);
											}
	};

class clsUSBIO
	: public clsIOBase
	{
	private:
		clsIODevice						_Device;
		String							_sPort;

		virtual DWORD					ThreadFunction(void);
		bool							FindUSBDeviceOfGUID(String sGUID, clsIODevice& Device);
		void							NtfyData(const BYTE* pszData, UINT nSize);
		String 							GetDeviceDescProperty(HDEVINFO hDevInfo, SP_DEVINFO_DATA& DeviceInfoData, DWORD nSPDRP);
	public:
										clsUSBIO(IDataSink* pDataStore);
										~clsUSBIO();

		virtual void					IOReset() {	}
		virtual bool					IOOpen(const String& sDevice);
		virtual void					IOClose();

		virtual bool					IOWrite(BYTE* pData, int nSize);
		virtual String					sDevice();
	};

//////////////////////////////////////////////////////////////////////////////

class clsFTDIIOBase
	: public clsIOBase
	{
	protected:
		String							_sPort;
		INT								_nDevice;
		HANDLE							_hDevice;
		UINT 							_nBaudrate;
		clsCrit							_FTDIAccess;

		#if defined(__TEST__)
		void							Send(char cmd, char data);
		#endif
		virtual void					IOCloseHandle() = 0;
	public:
										clsFTDIIOBase(const String& sType, IDataSink* pDataStore, UINT nBaudrate);
										~clsFTDIIOBase();

		virtual void					IOReset();
		virtual void					IOClose();
		virtual String					sDevice();
	};

class clsFTDIIO
	: public clsFTDIIOBase
	{
	private:
		virtual DWORD					ThreadFunction(void);
		virtual void					IOCloseHandle();
	public:
										clsFTDIIO(IDataSink* pDataStore, UINT nBaudrate);

		virtual bool					IOOpen(const String& sDevice);

		virtual bool					IOWrite(BYTE* pData, int nSize);
	};

class clsFTDIW32IO
	: public clsFTDIIOBase
	{
	private:
		virtual DWORD					ThreadFunction(void);
		virtual void					IOCloseHandle();
	public:
										clsFTDIW32IO(IDataSink* pDataStore, UINT nBaudrate);

		virtual bool					IOOpen(const String& sDevice);

		virtual bool					IOWrite(BYTE* pData, int nSize);
	};

//////////////////////////////////////////////////////////////////////////////

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

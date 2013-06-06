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

#include "clsBroadcast.h"

clsBroadcastServer::clsBroadcastServer()
	: _nSocket(INVALID_SOCKET)
{
	ZEROINIT(_SockIn);

	WORD 	wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 2, 0 );

	if (WSAStartup( wVersionRequested, &wsaData ) != 0)
		{
		printf("WinSock: cannot use sockets (WSAStartup failed)\n");
		}
}

clsBroadcastServer::~clsBroadcastServer()
{
	Close();
	WSACleanup();
}

void							clsBroadcastServer::Close()
{
	if (_nSocket != INVALID_SOCKET)
		{
		shutdown(_nSocket,2);
		closesocket(_nSocket);
		_nSocket = INVALID_SOCKET;
		}
}

void							clsBroadcastServer::Open(int nPort)
{
	Close();
	if (nPort > 0)
		{
		int		yes(1);
		int		status;

		_nSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (_nSocket == INVALID_SOCKET)
			{
			printf("WinSock: cannot use sockets (socket() failed)\n");
			return;
			}

		_SockIn.sin_addr.s_addr = htonl(INADDR_ANY);
		_SockIn.sin_port = htons(0);
		_SockIn.sin_family = PF_INET;

		status = bind(_nSocket, (struct sockaddr *)&_SockIn, sizeof(_SockIn));
		// printf("Bind Status = %d\n", status);

		status = setsockopt(_nSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&yes, sizeof(int));
		// printf("Setsockopt Status = %d\n", status);

		/* -1 = 255.255.255.255 this is a BROADCAST address,
		 a local broadcast address could also be used.
		 you can comput the local broadcat using NIC address and its NETMASK
		*/

		_SockIn.sin_addr.s_addr = htonl(-1); /* send message to 255.255.255.255 */
		_SockIn.sin_port = htons(nPort);	 /* port number */
		_SockIn.sin_family = PF_INET;
		}
}

void							clsBroadcastServer::Send(const String& s)
{
	if (_nSocket != INVALID_SOCKET)
		{
		int	status = sendto(
			_nSocket,
			(const char*)s,
			s.Len()+1,
			MSG_DONTROUTE,
			(struct sockaddr *)&_SockIn,
			sizeof(_SockIn));
		printf("sending '%s' -> %d\n",(LPCTSTR)s,status);
		}
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

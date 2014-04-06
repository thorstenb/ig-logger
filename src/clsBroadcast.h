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

//#include <winsock2.h>

class clsBroadcastServer
{
private:
    int                             _nSocket;
    int                             _nStatus;
    struct sockaddr_in              _SockIn;
    void                            Close();
public:
    clsBroadcastServer();
    ~clsBroadcastServer();

    void                            Open(int nPort);
    void                            Send(const String& s);
};

class clsBroadcastServerMailslot
{
private:
    HANDLE                          _hMailslot;

    void                            Close()
    {
        if (_hMailslot != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(_hMailslot);
            _hMailslot = INVALID_HANDLE_VALUE;
        }
    }
public:
    clsBroadcastServerMailslot()
        : _hMailslot(INVALID_HANDLE_VALUE)
    {
    }
    ~clsBroadcastServerMailslot()
    {
        Close();
    }

    bool                            Ok() const
    {
        return(_hMailslot != INVALID_HANDLE_VALUE);
    }
    void                            Open(const String& sMailslotName)
    {
        _hMailslot = ::CreateFile(
                         String::Format("\\\\.\\mailslot\\%s",(LPCTSTR)sMailslotName),
                         GENERIC_WRITE,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
        // printf("MAILSLOT: %d\n",_hMailslot);
    }
    void                            Send(const String& s)
    {
        DWORD   nWritten;

        ::WriteFile(_hMailslot,(LPCTSTR)s,strlen(s)+1,&nWritten,NULL);
        // printf("sending '%s' -> %u\n",(LPCTSTR)s,nWritten);
        if (nWritten == 0)
            Close();
    }
};
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

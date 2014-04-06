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

#include "clsExecProcess.h"

class clsAlertMessage
{
private:
    clsSYSTEMTIME                   _ST;
    String                          _sSubject;
    String                          _sDetails;
    bool                            _bImportant;
public:
    clsAlertMessage()
        : _ST(true)
        , _bImportant(false)
    {
    }
    clsAlertMessage(const String& sSubject, const String& sDetails)
        : _ST(true)
        , _sSubject(sSubject)
        , _sDetails(sDetails)
        , _bImportant(sSubject[0] == '!')
    {
        if (_bImportant)
        {
            _sSubject.Delete(0,1);
        }
    }
    clsAlertMessage(const clsAlertMessage& rhs)
        : _ST(rhs._ST)
        , _sSubject(rhs._sSubject)
        , _sDetails(rhs._sDetails)
        , _bImportant(rhs._bImportant)
    {
    }

    clsAlertMessage&                operator=(const clsAlertMessage& rhs)
    {
        if (&rhs != this)
        {
            _ST = rhs._ST;
            _sSubject = rhs._sSubject;
            _sDetails = rhs._sDetails;
            _bImportant = rhs._bImportant;
        }
        return(*this);
    }
    int                             operator==(const clsAlertMessage& rhs) const
    {
        return(_sSubject == rhs._sSubject);
    }
    int                             operator<(const clsAlertMessage& rhs) const
    {
        return(_sSubject < rhs._sSubject);
    }

    const clsSYSTEMTIME&            DateTime() const
    {
        return(_ST);
    }
    const String&                   sSubject() const
    {
        return(_sSubject);
    }
    const String&                   sDetails() const
    {
        return(_sDetails);
    }
    bool                            bImportant() const
    {
        return(_bImportant);
    }
};

class clsAlertThread
    : private clsThreadBase
    , public clsExecNotifier
{
private:
    clsCrit                         _Crit;
    WCValSkipList<clsAlertMessage>  _ToDoList;
    WCValSkipList<clsAlertMessage>  _SentList;

    virtual DWORD                   ThreadFunction(void);
public:
    clsAlertThread()
        : clsThreadBase("clsAlertThread")
    {
        ThreadStart(THREAD_PRIORITY_ABOVE_NORMAL);
    }
    ~clsAlertThread()
    {
        ThreadJoin();
        Clear();
    }

    void                            Add(const clsAlertMessage& Msg)
    {
        clsCritObj      CO(_Crit);
        clsAlertMessage OldMsg;

        if (Msg.bImportant() ||
                !_SentList.find(Msg,OldMsg))
        {
            _ToDoList.insert(Msg);
        }
        else
        {
            // ignore another message with the same subject for the next hour
            printf("  ignore message '%s' - the previous one is not yet an hour old...\n",
                   (LPCTSTR)OldMsg.sSubject());
        }
    }
    void                            Finalize()
    {
        while (!_ToDoList.isEmpty())
        {
            ::Sleep(1000);
        }
    }
    void                            Clear()
    {
        clsCritObj  CO(_Crit);

        _ToDoList.clear();
    }

    virtual void                    Notify(const String& s)
    {
        printf("NTFY %s\n",(LPCTSTR)s);
    }
};

extern clsAlertThread   __AlertThread;

class clsWatchdog
    : public clsThreadBase
{
private:
    INT                             _nNotActive;
    bool                            _bWithNotify;
    clsEvent                        _evtActive;
    UINT                            _nTimeout;

    virtual DWORD                   ThreadFunction(void)
    {
        INT     nEvent;

        while (ThreadWait(_nTimeout,nEvent,1,
                          _evtActive.Handle())) // allow 60 seconds...
        {
            if (_nNotActive > 0)
                continue;

            // printf("watchdog event %d\n",nEvent);
            switch (nEvent)
            {
                case -1: // timeout
                    if (_bWithNotify)
                    {
                        clsWatchdog WD(false); // watch this thread (without mail!)... ;(

                        AlertUser(
                            "!WATCHDOG: LOGIG seems to hang - force termination!",
                            "Alert: there was no action for the last 60 seconds... the application was terminated!"
                        );
                        __AlertThread.Finalize();
                    }
                    ::ExitProcess(125);
                    break;
                case 0: // _evtActive
                    break;
            }
            _nTimeout = 60000;
        }

        return(0);
    }
public:
    clsWatchdog(bool bWithNotify = true)
        : clsThreadBase("clsWatchdog")
        , _evtActive(FALSE)
        , _bWithNotify(bWithNotify)
        , _nNotActive(0)
        , _nTimeout(60000)
    {
        ThreadStart(THREAD_PRIORITY_ABOVE_NORMAL);
    }
    ~clsWatchdog()
    {
        ThreadJoin();
    }

    void                            Reset()
    {
        _evtActive.Pulse();
    }
    void                            SetActive(bool bActive)
    {
        if (bActive)
            --_nNotActive;
        else
            ++_nNotActive;
        _nTimeout = 10000;
        if (_nNotActive == 0)
            Reset(); // resets the timer
    }
};

extern clsWatchdog      __Watchdog;

class clsWatchdogTemporarilyInactive
{
public:
    clsWatchdogTemporarilyInactive()
    {
        __Watchdog.SetActive(false);
    }
    ~clsWatchdogTemporarilyInactive()
    {
        __Watchdog.SetActive(true);
    }
};
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

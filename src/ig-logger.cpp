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

#include <conio.h>
#include <GPFProt.hpp>

#include "clsExcept.h"
#include "clsLocalNet.h"
#include "clsExecProcess.h"
#include "clsConfig.h"
#include "utils.h"
#include "clsWatchdog.h"
#include "mx.h"

//////////////////////////////////////////////////////////////////////////////

#if 1
#define _IMPLEMENT_LATELOAD_INSTANCES_ 1
#include <clsGDI32.hpp>
#include <clsUser32.hpp>
#include <clsKernel32.hpp>
#include <clsComCtl32.hpp>
#include <clsComDlg32.hpp>
#endif

//////////////////////////////////////////////////////////////////////////////

void usage()
{
    printf("LOGIG (c) 2007-2008 Kaiser Software Engineering\n");
    printf("Usage\n");
    printf("  logig [[-i]|[-r] [-s]] [-t] [-p=<port>]\n");
    printf("    '-i' to set up the configuration (needs running inverter(s)), then quit\n");
    printf("    '-r' to save the data in an RRDTOOL database and upload the files to the internet\n");
    printf("    '-s' to save the data in an SQLite database\n");
    printf("    '-t' to send a test email at startup\n");
    printf("    '-p=<port>' to set a port to use (no auto-detection)\n");
    printf("      <port> is one of the following texts:\n");
    printf("        'COM:COM<n>' (n=1..255)\n");
    // printf("        'USB:Datalogger'\n");
    printf("        'FTDI:Datalogger'\n");
    printf("        'FTDIW32:Datalogger'\n");
    printf("    '-b' to send the current values by mailslot broadcast\n");
    printf("    '-oh=<histo>' to set the interval for historical dates (for inverter(s))\n");
    printf("    '-os=<histo>' to set the interval for historical dates (for sensor card(s))\n");
    printf("    '  <histo>' is the interval in minutes (default: don't change interval)\n");
    printf("    '-?' to print this help information\n");
    printf("    Both '-r' and 's' can be given as output.\n");
    printf("  Example: 'logig -r -s -p=COM:COM1'.\n");
    exit(127);
}

//////////////////////////////////////////////////////////////////////////////

class clsControlHandler
    : public clsThreadBase
{
private:
    volatile bool                   _bUserAbortRequested;
    static BOOL _stdcall            MyControlHandler(DWORD dwControlType);

    virtual DWORD                   ThreadFunction(void)
    {
        // wait for abort
        // (the destruction of the control handler tells that the process terminates normally)
        if (ThreadWait(60000)) // allow 60 seconds...
        {
            printf(TEXT("*** the application seems to hang - force termination!\n"));
            // if the thread lived 30 seconds, it will terminate the process
            ::ExitProcess(126);
        }

        return(0);
    }
public:
    clsControlHandler()
        : clsThreadBase("clsControlHandler")
        , _bUserAbortRequested(false)
    {
        ::SetConsoleCtrlHandler(MyControlHandler,true);
    }
    ~clsControlHandler()
    {
        ::SetConsoleCtrlHandler(MyControlHandler,true);
        ThreadEnd();
    }

    bool                            UserAbortRequested()
    {
        return(_bUserAbortRequested);
    }

    void                            SetUserAbortRequested()
    {
        _bUserAbortRequested = true;
        ThreadStart();
    }
};

clsControlHandler   __ControlHandler;

BOOL clsControlHandler::MyControlHandler(DWORD dwControlType)
{
    switch (dwControlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
            printf("*** user abort requested, please wait - process will shutdown soon\n");
            __ControlHandler.SetUserAbortRequested();
            break;
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            printf("*** console is about to close, please wait.\n");
            __ControlHandler.SetUserAbortRequested();
            break;
    }
    return(TRUE);
}

bool bUserAbortRequested()
{
    return(__ControlHandler.UserAbortRequested());
}

//////////////////////////////////////////////////////////////////////////////

extern  clsConfigOfLocation __LocationConfig;

virtual DWORD                   clsAlertThread::ThreadFunction(void)
{
    DWORD   dwWaitTime = 1000;

    while (ThreadWait(dwWaitTime))
    {
        WCValSkipList<clsAlertMessage>  MessageList;
        clsSYSTEMTIME   ST(true);

        {
            clsCritObj  CO(_Crit);

            MessageList = _ToDoList;

            _ToDoList.clear();
        }

        if (!MessageList.isEmpty())
        {
            WCValSkipListIter<clsAlertMessage>  Message(MessageList);
            clsMailClient                       MailClient(
                __LocationConfig.GetSectionData("Mail"));

            try
            {
                MailClient.Open();

                while (++Message)
                {
                    const clsAlertMessage&  Msg(Message.current());
                    //                  String                  sSubject(String::Format("%s: %s",(LPCTSTR)Msg.DateTime().sValue(),(LPCTSTR)Msg.sSubject()));
                    String                  sSubject(Msg.sSubject());
                    String                  sMessage(Msg.sDetails());

                    try
                    {
                        printf("email: %s\n",(LPCTSTR)sSubject);
                        MailClient.Send(sSubject,sMessage);

                        clsCritObj  CO(_Crit);

                        _SentList.insert(Msg);
                        dwWaitTime = 1000; // if sending was OK, check every second
                    }
                    catch (clsExcept& e)
                    {
                        dwWaitTime = 60000; // once a minute if an error occurred
                        printf("ERR while sending mail: %s\n",(LPCTSTR)e.Text());

                        // and remember for later...
                        clsCritObj  CO(_Crit);

                        _ToDoList.insert(Msg);
                    }
                }
            }
            catch (clsExcept& e)
            {
                dwWaitTime = 60000; // once a minute if an error occurred
                printf("%s\n",(LPCTSTR)e.Text());
            }

            MailClient.Close();
        }

        clsCritObj  CO(_Crit);

        // cleanup of the sent list: delete all sent items old than an hour
        for (WCValSkipListIter<clsAlertMessage> Message(_SentList); ++Message; )
        {
            if ((Message.current().DateTime() - ST) > 1.0/24.0)
            {
                _SentList.remove(Message.current());
                Message.reset();
            }
        }
    }
    return(0);
}


clsAlertThread  __AlertThread;

void AlertUser(const String& sSubject, const String& sDetails)
{
    printf("%s: %s\r\n",(LPCTSTR)clsSYSTEMTIME(true).sValue(),(LPCTSTR)sSubject);

    if (!__LocationConfig.GetSectionData("Mail").isEmpty())
    {
        __AlertThread.Add(clsAlertMessage(sSubject,sDetails));
    }
}

//////////////////////////////////////////////////////////////////////////////

clsWatchdog     __Watchdog;

//////////////////////////////////////////////////////////////////////////////

int run(WCValSList<String>& Args)
{
    printf("LOGIG version %s\n",(LPCTSTR)GetModuleVersion(Args.find()));
    _chk_utildebug("LOGIG version %s\n",(LPCTSTR)GetModuleVersion(Args.find()));

    enum enConnectionState
    {
        CONNECTION_NONE,
        CONNECTION_OK,
        CONNECTION_LOST,
    };

    WCValSListIter<String>  Arg(Args);
    clsLocalNet::enMode     nMode = clsLocalNet::MODE_NEUTRAL;
    String                  sPort;
    clsSYSTEMTIME           STLastTimeoutMessage;
    bool                    bSendTestMail(false);
    WCValSkipListDict<clsIGMessage::enDataloggerOption,clsVariant> Options;

    // default options:
    //  Options[clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL] = 5;

    while (++Arg)
    {
        const char* pcArg = Arg.current();

        if (pcArg[0] == '-' ||
                pcArg[0] == '/')
        {
            ++pcArg;
            switch (*pcArg++)
            {
                case '?':
                    usage();
                    break;
                case 'i':
                    nMode = nMode | clsLocalNet::MODE_INSTALL;
                    break;
                case 's':
                    nMode = nMode | clsLocalNet::MODE_SQLITE;
                    break;
                case 'r':
                    nMode = nMode | clsLocalNet::MODE_RRDTOOL;
                    break;
                case 't':
                    bSendTestMail = true;
                    break;
                case 'p':
                    if (*pcArg == '=')
                        ++pcArg;
                    sPort = pcArg;
                    break;
                case 'b':
                    Options[clsIGMessage::IG_DL_OPTION_BROADCASTPORT] = clsVariant("logig");
                    break;
                case 'o':
                {
                    char    cValue = *pcArg++;

                    if (*pcArg == '=')
                        ++pcArg;
                    switch (cValue)
                    {
                        case 'h':
                            Options[clsIGMessage::IG_DL_OPTION_INVERTERHISTOINTERVAL] = clsVariant(
                                        _chk_atoi(pcArg));
                            break;
                        case 's':
                            Options[clsIGMessage::IG_DL_OPTION_SENSORHISTOINTERVAL] = clsVariant(_chk_atoi(
                                        pcArg));
                            break;
                    }
                }
                break;
            }
        }
    }

    if (bSendTestMail)
    {
        AlertUser(
            String::Format("test email sent by %s",(LPCTSTR)__LibInfo.AppName()),
            String::Format("This mail was created by user '%s' on computer '%s'",
                           (LPCTSTR)UserName(),(LPCTSTR)ComputerName()));
        __AlertThread.Finalize();
        return(0);
    }

    clsLocalNet         LocalNet(nMode,Options);
    enConnectionState   nConnectionState(CONNECTION_NONE);

    while (!bUserAbortRequested())
    {
        try
        {
            while (nConnectionState != CONNECTION_OK)
            {
                __Watchdog.Reset();

                bool    bLoggerFound = LocalNet.Open(sPort);

                if (bLoggerFound)
                {
                    if (nConnectionState == CONNECTION_LOST)
                    {
                        AlertUser("!reconnected to datalogger");
                    }
                    nConnectionState = CONNECTION_OK;
                    STLastTimeoutMessage.Clear();
                }
                else
                {
                    if (sPort.IsEmpty())
                    {
                        printf("FATAL: no port with a datalogger found\n");
                    }
                }
                if (bUserAbortRequested())
                {
                    return(clsLocalNet::RES_USERABORT);
                }
                if (nConnectionState != CONNECTION_OK)
                {
                    Sleep(5000);
                }
            }

            bool    bFirstProcess(true);

            while (nConnectionState == CONNECTION_OK)
            {
                __Watchdog.Reset();

                INT     nRes = LocalNet.ProcessMessages(true,bFirstProcess);

                bFirstProcess = false;

                switch (nRes)
                {
                    case clsLocalNet::RES_OK:
                        STLastTimeoutMessage.Clear();
                        break;
                    case clsLocalNet::RES_INSTALL_DONE:
                        return(0);
                    case clsLocalNet::ERR_CONNECTIONLOST:
                    {
                        clsSYSTEMTIME   ST(true);

                        // alert every hour
                        if (ST.dValue() > STLastTimeoutMessage.dValue()+1.0/24.0)
                        {
                            AlertUser(
                                "!connection to datalogger lost",
                                "LOGIG will permanently retry to re-connect to the datalogger.\r\n"
                                "You will be informed about reconnection if it happens automatically.\r\n"
                            );
                            STLastTimeoutMessage = ST;
                        }
                    }
                    nConnectionState = CONNECTION_LOST;
                    break;
                    default:
                        return(-(INT)nRes);
                }
                if (bUserAbortRequested())
                {
                    return(clsLocalNet::RES_USERABORT);
                }
            }
        }
        catch (clsFatalExcept& e)
        {
            printf("--------------------------------------------------------------\n%s\n",
                   (LPCTSTR)e.Text());
            AlertUser(e.Text());
            __AlertThread.Finalize();
            return(clsLocalNet::ERR_FATAL);
        }
        catch (clsNotifyExcept& e)
        {
            printf("--------------------------------------------------------------\n%s\n",
                   (LPCTSTR)e.Text());
            AlertUser(e.Text());
        }
        catch (clsExceptBase& e)
        {
            printf("--------------------------------------------------------------\n%s\n",
                   (LPCTSTR)e.Text());
        }
    }
    return(clsLocalNet::RES_OK);
}

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    // clsSolarInfo SI; // test object

    __LibInfo.Attach(NULL);

#if GPFPROTECT
    clsGPFProtection    GPF(GPFPROTFLAG_TRACE|GPFPROTFLAG_MSGBOX);

    if (GPF.Error())
    {
        return(255);
    }
#endif

    WCValSList<String>  Args;

    for (int nArg = 0; nArg < argc; ++nArg)
    {
        Args.append(argv[nArg]);
    }

    if (Args.isEmpty())
    {
        usage();
    }

    return(run(Args));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

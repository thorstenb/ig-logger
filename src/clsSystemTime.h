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
#if !defined(_INC_SYSTEMTIME)
#define _INC_SYSTEMTIME

#include <math.h>

class clsSYSTEMTIME
    : public SYSTEMTIME
{
public:
    enum enSetMode
    {
        MODE_OLE,
        MODE_HOUR,
    };
    clsSYSTEMTIME()
    {
        Clear();
    }
    clsSYSTEMTIME(bool bLocalTime)
    {
        ZEROINIT(*this);
        if (bLocalTime)
        {
            ::GetLocalTime(this);
        }
        else
        {
            ::GetSystemTime(this);
        }
    }
    clsSYSTEMTIME(const clsSYSTEMTIME& rhs)
    {
        *this = rhs;
    }
    clsSYSTEMTIME(
        unsigned nYear,
        unsigned nMonth,
        unsigned nDay,
        unsigned nHour = 0,
        unsigned nMinute = 0,
        unsigned nSecond = 0)
    {
        this->wYear = (WORD)nYear;
        this->wMonth = (WORD)nMonth;
        this->wDay = (WORD)nDay;
        this->wHour = (WORD)nHour;
        this->wMinute = (WORD)nMinute;
        this->wSecond = (WORD)nSecond;
        this->wMilliseconds = (WORD)0;
        this->wDayOfWeek = (WORD)-1;
    }
    clsSYSTEMTIME(double dValue, enSetMode nMode)
    {
        Clear();
        switch (nMode)
        {
            case MODE_OLE:
                ::VariantTimeToSystemTime(dValue,this);
                break;
            case MODE_HOUR:
            {
                clsSYSTEMTIME   ST(true);

                this->wHour = floor(dValue);
                this->wMinute = (WORD)floor(60 * dValue) % 60;
                this->wSecond = (WORD)(60 * 60 * dValue) % 60;

                this->wYear = ST.wYear;
                this->wMonth = ST.wMonth;
                this->wDay = ST.wDay;
                this->wDayOfWeek = ST.wDay;
            }
            break;
        }
    }
    clsSYSTEMTIME(const SYSTEMTIME& rhs)
    {
        *(SYSTEMTIME*)this = *(SYSTEMTIME*)&rhs;
    }

    clsSYSTEMTIME&                  operator=(const clsSYSTEMTIME& rhs)
    {
        if (&rhs != this)
        {
            *(SYSTEMTIME*)this = *(SYSTEMTIME*)&rhs;
        }
        return(*this);
    }
    double                          operator-(const clsSYSTEMTIME& rhs) const
    {
        return(dValue() - rhs.dValue());
    }
    const clsSYSTEMTIME&            operator+(const double& dDelta) const
    {
        return(clsSYSTEMTIME(dValue() + dDelta,MODE_OLE));
    }
    const clsSYSTEMTIME&            operator-(const double& dDelta) const
    {
        return(clsSYSTEMTIME(dValue() - dDelta,MODE_OLE));
    }
    void                            operator+=(const double& dDelta)
    {
        ::VariantTimeToSystemTime(dValue() + dDelta,this);
    }
    void                            operator-=(const double& dDelta)
    {
        ::VariantTimeToSystemTime(dValue() - dDelta,this);
    }
    int                             operator==(const clsSYSTEMTIME& rhs) const
    {
        return(dValue() == rhs.dValue());
    }
    int                             operator<(const clsSYSTEMTIME& rhs) const
    {
        return(dValue() < rhs.dValue());
    }

    bool                            bValid() const
    {
        if (this->wYear < 1900 || this->wYear > 2100)
            return(false);
        if (this->wMonth < 1 || this->wMonth > 12)
            return(false);
        if (this->wDay < 1 || this->wDay > 31)
            return(false);
        if (this->wHour > 24)
            return(false);
        if (this->wMinute > 59)
            return(false);
        if (this->wSecond > 59)
            return(false);
        return(true);
    }
    void                            Clear()
    {
        ZEROINIT(*this);
        this->wDayOfWeek = (WORD)-1;
    }
    double                          dValue() const
    {
        double  dThis;
        ::SystemTimeToVariantTime(const_cast<SYSTEMTIME*>
                                  (static_cast<const SYSTEMTIME*>(this)),&dThis);
        return(dThis);
    }
    static int                      __SecondsSince19700101(int nYear, int nMonth,
            int nDay, int nHour, int nMinute, int nSecond)
    {
        TIME_ZONE_INFORMATION   TZ;

        ZEROINIT(TZ);

        struct  tm  tm1 = {0};

        tm1.tm_sec = nSecond;
        tm1.tm_min = nMinute;
        tm1.tm_hour = nHour;
        tm1.tm_mday = nDay;
        tm1.tm_mon = nMonth - 1;
        tm1.tm_year = nYear - 1900;
        tm1.tm_isdst = (::GetTimeZoneInformation(&TZ) == TIME_ZONE_ID_DAYLIGHT);
        return(mktime(&tm1));
    }
    int                             SecondsSince19700101(void) const
    {
        return(__SecondsSince19700101(
                   this->wYear,
                   this->wMonth,
                   this->wDay,
                   this->wHour,
                   this->wMinute,
                   this->wSecond));
    }
    String                          sValue(bool bForDatabase=false) const
    {
        if (bForDatabase)
        {
            return(String::Format("%04d%02d%02d%02d%02d",
                                  this->wYear,
                                  this->wMonth,
                                  this->wDay,
                                  this->wHour,
                                  this->wMinute));
        }
        else
        {
            TCHAR       szDate[30];
            TCHAR       szTime[30];

            szDate[0] = 0;
            szTime[0] = 0;

            ::GetDateFormat(
                LOCALE_USER_DEFAULT,
                DATE_SHORTDATE,
                this,
                NULL,
                szDate,
                sizeofTSTR(szDate));
            if (!::GetTimeFormat(
                        LOCALE_USER_DEFAULT,
                        TIME_NOSECONDS,
                        this,
                        NULL,
                        szTime,
                        sizeofTSTR(szTime)))
            {
                printf("[time %d:%02d:%02d.%03d is invalid: %s]\n",
                       this->wHour,
                       this->wMinute,
                       this->wSecond,
                       this->wMilliseconds,
                       (LPCTSTR)String::GetLastError());
            }
            if (wDayOfWeek == (WORD)-1)
            {
                return(String::Format("%s, %s",
                                      szDate,
                                      szTime));
            }
            else
            {
                return(String::Format("%s, %s (%s)",
                                      szDate,
                                      szTime,
                                      (LPCTSTR)String::Locale(LOCALE_SDAYNAME1+((wDayOfWeek+6)%7))));
            }
        }
    }
    String                          sDate(bool bForDatabase=false) const
    {
        if (bForDatabase)
        {
            return(String::Format("%04d%02d%02d",
                                  this->wYear,
                                  this->wMonth,
                                  this->wDay));
        }
        else
        {
            char        szDate[30];

            ::GetDateFormat(
                LOCALE_USER_DEFAULT,
                DATE_SHORTDATE,
                this,
                NULL,
                szDate,
                sizeof(szDate));
            return(szDate);
        }
    }
    String                          sTime(bool bForDatabase=false) const
    {
        if (bForDatabase)
        {
            return(String::Format("%02d%02d",
                                  this->wHour,
                                  this->wMinute));
        }
        else
        {
            char        szTime[30];

            ::GetTimeFormat(
                LOCALE_USER_DEFAULT,
                TIME_NOSECONDS,
                this,
                NULL,
                szTime,
                sizeof(szTime));
            return(String::Format("%s.%03d",szTime,this->wMilliseconds));
        }
    }
    String                          sTimeHMS() const
    {
        return(String::Format("%02d%02d%02d",
                              this->wHour,
                              this->wMinute,
                              this->wSecond));
    }
    UINT                            nDate() const
    {
        return(this->wYear * 10000 + this->wMonth * 100 + this->wDay);
    }
    UINT                            nTime() const
    {
        return(this->wHour * 100 + this->wMinute);
    }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

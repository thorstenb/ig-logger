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

#ifndef INCLUDED_EXCEPT_H
#define INCLUDED_EXCEPT_H

#include <wcstack.h>
#include <wclist.h>
#include <wclistit.h>

typedef WCStack<String,WCValSList<String> > clsErrorStack;

class clsExceptBase
{
public:
    enum                            enErrorcodes
    {
        RES_OK = 0,
        ERR_FATAL = -1,
    };
private:
    clsErrorStack                   _ErrorStack;
public:
    clsExceptBase() { }
    clsExceptBase(const clsExceptBase& rhs);
    virtual                         ~clsExceptBase() { }


    void                            Push(LPCTSTR pszFormat, ...);
    clsErrorStack&                  ErrorStack();
    String                          Text();
};

class clsExcept
    : public clsExceptBase
{
public:
    clsExcept() { }
    clsExcept(LPCTSTR pszFormat, ...);
};

class clsTemporaryProblem
    : public clsExceptBase
{
public:
    clsTemporaryProblem(LPCTSTR pszFormat, ...);
};

class clsStateExcept
    : public clsExcept
{
public:
    clsStateExcept(LPCTSTR pszFormat, ...);
};

class clsNotifyExcept
    : public clsExceptBase
{
public:
    clsNotifyExcept(LPCTSTR pszFormat, ...);
};

class clsBadTimeExcept
    : public clsExceptBase
{
public:
    clsBadTimeExcept(LPCTSTR pszFormat, ...);
};

class clsFatalExcept
    : public clsExceptBase
{
public:
    clsFatalExcept(LPCTSTR pszFormat, ...);
};

class clsConnectionLostExcept
    : public clsExceptBase
{
public:
    clsConnectionLostExcept(LPCTSTR pszFormat, ...);
    clsConnectionLostExcept(const clsExceptBase& rhs)
        : clsExceptBase(rhs)
    {
    }
};

#endif // !defined( INCLUDED_EXCEPT_H)

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

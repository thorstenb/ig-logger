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

#if DEFINE_DATA==1

#include <clsVariant.hpp>

#include "clsSystemTime.h"

class clsDataItem
{
public:
    clsVariant                      _vData;
    String                          _sUnit;
public:
    clsDataItem()
    {
    }
    clsDataItem(const clsDataItem& rhs)
    {
        *this = rhs;
    }
    clsDataItem(const clsVariant& vData, const String& sUnit)
        : _vData(vData)
        , _sUnit(sUnit)
    {
    }

    clsDataItem&                    operator=(const clsDataItem& rhs)
    {
        if (&rhs != this)
        {
            _vData = rhs._vData;
            _sUnit = rhs._sUnit;
        }
        return(*this);
    }
    String                          AsStr() const
    {
        return(String::Format("%g %s",clsVariant(_vData).AsDouble(),(LPCTSTR)_sUnit));
    }
};

typedef WCValSkipListDictIter<String,clsDataItem>
clsDataItemListIter;
class clsDataItemList
    : public WCValSkipListDict<String,clsDataItem>
{
public:
    clsDataItemList()
    {
    }
    clsDataItemList(const clsDataItemList& rhs)
    {
        *this = rhs;
    }

    clsDataItemList&                operator=(const clsDataItemList& rhs)
    {
        if (&rhs != this)
        {
            WCValSkipListDict<String,clsDataItem>::operator=(rhs);
        }
        return(*this);
    }

    void                            Dump()
    {
        clsDataItemListIter i(*this);

        while (++i)
        {
            printf("  %-20s: %s\n",(LPCTSTR)i.key(),(LPCTSTR)i.value().AsStr());
        }
    }
};

typedef WCValSkipListDictIter<clsSYSTEMTIME,clsDataItemList>
clsDataListIter;
class clsDataList
    : public WCValSkipListDict<clsSYSTEMTIME,clsDataItemList>
{
public:
    clsDataList()
    {
    }
    clsDataList(const clsDataList& rhs)
    {
        *this = rhs;
    }

    clsDataList&                    operator=(const clsDataList& rhs)
    {
        if (&rhs != this)
        {
            WCValSkipListDict<clsSYSTEMTIME,clsDataItemList>::operator=(rhs);
        }
        return(*this);
    }

    void                            Add(const clsSYSTEMTIME& ST,
                                        const clsDataItemList& DL)
    {
        insert(ST,DL);
    }
    void                            Dump()
    {
        clsDataListIter i(*this);

        while (++i)
        {
            const clsSYSTEMTIME&    ST(i.key());

            printf(" %04d-%02d-%02d %02d:%02d:%02d\n",
                   ST.wYear,
                   ST.wMonth,
                   ST.wDay,
                   ST.wHour,
                   ST.wMinute,
                   ST.wSecond);
            i.value().Dump();
        }
    }
};

#endif

#if DEFINE_DATA==2

typedef WCValSkipListDictIter<clsLocalNetID,clsDataList*>
clsDeviceDataListIter;
class clsDeviceDataList
    : public WCValSkipListDict<clsLocalNetID,clsDataList*>
{
public:
    clsDeviceDataList()
    {
    }
    clsDeviceDataList(const clsDeviceDataList& rhs)
    {
        *this = rhs;
    }
    ~clsDeviceDataList()
    {
        Clear();
    }

    void                            Clear()
    {
        clsDeviceDataListIter   i(*this);

        while (++i)
        {
            delete i.value();
        }
        clear();
    }

    clsDeviceDataList&              operator=(const clsDeviceDataList& rhs)
    {
        if (&rhs != this)
        {
            Clear();
            clsDeviceDataListIter   i(rhs);

            while (++i)
            {
                insert(i.key(),new clsDataList(*i.value()));
            }
        }
        return(*this);
    }

    void                            Add(const clsLocalNetID& ID,
                                        const clsSYSTEMTIME& ST, const clsDataItemList& DL)
    {
        clsDataList*    pDL;

        if (!find(ID,pDL))
        {
            insert(ID,pDL = new clsDataList);
        }
        pDL->Add(ST,DL);
    }

    void                            Dump(const clsDeviceList& DeviceList)
    {
        clsDeviceDataListIter   i(*this);

        while (++i)
        {
            printf("DEVICE %s\n",(LPCTSTR)DeviceList[i.key()]);

            i.value()->Dump();
        }
    }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

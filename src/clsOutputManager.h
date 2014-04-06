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

#ifndef INCLUDED_OUTPUTMANAGER_H
#define INCLUDED_OUTPUTMANAGER_H

#include "clsConfig.h"
#include "clsExcept.h"
#include "clsExecProcess.h"
#include "clsDatabase.h"

#include <wcskip.h>

class clsOutputManager
{
private:
    String                          CreateDirectory(const WCValSList<String>&
            DirectoryParts);
protected:
    const clsConfigOfLocation&      _Location;
    String                          _sBaseDirectory;

    virtual LPCTSTR                 pszSectionName() = 0;
    virtual bool                    bHasCommonBaseDirectory() = 0;
    void                            UploadFile(const clsConfigOfInstallation&
            Installation, const String& sFilename);
    virtual void                    Initialize(const clsConfigOfInstallation&
            Installation, const clsLocalNetID& ID);
public:
    clsOutputManager(const clsConfigOfLocation& Location);
    virtual                         ~clsOutputManager()
    {
    }

    virtual bool                    Append(const clsDeviceDataList& List,
                                           const clsErrorList& ErrorList, bool bHistorical, bool bForceGraphic) = 0;
    virtual void                    DailyMaintenance(const clsLocalNetID& /* ID */)
    {
    }
};

class clsRRDTOOL
    : public clsOutputManager
    , private clsExecNotifier
{
private:
    virtual LPCTSTR                 pszSectionName()
    {
        return("RRDTool");
    }
    virtual bool                    bHasCommonBaseDirectory()
    {
        return(false);
    }
    bool                            ExecRRD(const String& sCmdLine);

    String                          sDatabaseName(const clsSYSTEMTIME& ST,
            bool bHistorical);
    virtual void                    Initialize(const clsConfigOfInstallation&
            Installation, const clsLocalNetID& ID);
    void                            CreateDatabase(const clsConfigOfInstallation&
            Installation, const clsSYSTEMTIME& ST, const String& sFilename,
            bool bHistorical);
    void                            CreateGraph(const clsConfigOfInstallation&
            Installation, const clsSYSTEMTIME& ST, const clsDataItemList& List,
            const clsErrorList& ErrorList, bool bHistorical);
    void                            AppendData(const clsConfigOfInstallation&
            Installation, const clsDataList& List, const clsErrorList& ErrorList,
            bool bHistorical, bool bForceGraphic);
    virtual void                    Notify(const String& s);
public:
    clsRRDTOOL(const clsConfigOfLocation& Location);

    virtual bool                    Append(const clsDeviceDataList& List,
                                           const clsErrorList& ErrorList, bool bHistorical, bool bForceGraphic);
};

class clsSQLite
    : public clsOutputManager
{
private:
    clsDatabase*                    _pDatabase;

    virtual LPCTSTR                 pszSectionName()
    {
        return("SQLite");
    }
    virtual bool                    bHasCommonBaseDirectory()
    {
        return(true);
    }
    String                          sTableName(const clsLocalNetID& ID);
    virtual void                    Initialize(const clsConfigOfInstallation&
            Installation, const clsLocalNetID& ID);
    void                            AppendData(const clsConfigOfInstallation&
            Installation, const clsLocalNetID& ID, const clsDataList& List,
            const clsErrorList& ErrorList);
public:
    clsSQLite(const clsConfigOfLocation& Location);
    ~clsSQLite();

    virtual bool                    Append(const clsDeviceDataList& List,
                                           const clsErrorList& ErrorList, bool bHistorical, bool bForceGraphic);
    virtual void                    DailyMaintenance(const clsLocalNetID& ID);
};

#endif // !defined INCLUDED_OUTPUTMANAGER_H

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

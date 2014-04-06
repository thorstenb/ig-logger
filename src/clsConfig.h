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

#ifndef INCLUDED_CONFIG_H
#define INCLUDED_CONFIG_H

#include "clsLocalNet.h"

class clsConfigOfInstallation
{
public:
    String                          _sName;
    String                          _sDirectory;
    double                          _dTilt;          // 35
    double                          _dAspect;        // 260
    double                          _dToleranceSunrise;
    double                          _dToleranceSunset;
    double                          _dPeakPower;
    double                          _dArea;
    bool                            _bUpdateInverterClock;
public:
    clsConfigOfInstallation()
    {
    }
    clsConfigOfInstallation(const clsConfigOfInstallation& rhs)
    {
        *this = rhs;
    }
    clsConfigOfInstallation(const String& sConfigName, const String& sSection);

    clsConfigOfInstallation&        operator=(const clsConfigOfInstallation& rhs)
    {
        if (&rhs != this)
        {
            _sName              = rhs._sName;
            _sDirectory         = rhs._sDirectory;
            _dTilt              = rhs._dTilt;
            _dAspect            = rhs._dAspect;
            _dToleranceSunrise  = rhs._dToleranceSunrise;
            _dToleranceSunset   = rhs._dToleranceSunset;
            _dPeakPower         = rhs._dPeakPower;
            _dArea              = rhs._dArea;
            _bUpdateInverterClock = rhs._bUpdateInverterClock;
        }
        return(*this);
    }
};

class clsConfigOfLocation
{
private:
    String                          _sConfigName; // filename
    WCValSkipListDict<String,clsConfigOfInstallation> _InstallationList;
public:
    String                          _sDirectory;
    String                          _sName;
    double                          _dLongitude;
    double                          _dLatitude;
    double                          _dMaxSolarPowerPerSqrMeter;

    void                            NotifyNewInverter(const clsLocalNetID& Device);
    void                            NotifyNewSensorcard(const clsLocalNetID&
            Device);
public:
    clsConfigOfLocation();

    void                            NotifyNewDevice(const clsLocalNetID& Device);

    WCValSkipListDict<IString,String> GetSectionData(const String& sSection) const;
    const double&                   dMaxSolarPowerPerSqrMeter() const
    {
        return(_dMaxSolarPowerPerSqrMeter);
    }
    clsDate                         GetDateOfLastUpdate() const;
    void                            SetDateOfLastUpdate(const clsDate& date) const;
    const clsConfigOfInstallation   Installation(const clsLocalNetID& Device);
    const String&                   sFilename()
    {
        return(_sConfigName);
    }
};

extern clsConfigOfLocation  __LocationConfig;

#endif // !defined INCLUDED_CONFIG_H

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
 $Id$

 Author(s): Ali Uneri

 (C) Copyright 2007-2009 Johns Hopkins University (JHU), All Rights Reserved.

 --- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#ifndef _trackerSimulator_h
#define _trackerSimulator_h

#include <cisstMultiTask.h>
#include <cisstOSAbstraction.h>
#include <cisstParameterTypes.h>

#include "trackerUI.h"

class trackerSimulator: public mtsTaskPeriodic
{
    CMN_DECLARE_SERVICES(CMN_NO_DYNAMIC_CREATION, CMN_LOG_LOD_RUN_ERROR);

    volatile bool ExitFlag;

protected:
    prmPositionCartesianGet CartesianPosition;

    // user interface generated by FTLK/fluid
    trackerUI UI;

public:
    trackerSimulator(const std::string & taskName, double period);
    ~trackerSimulator() {};

    void Configure(const std::string & CMN_UNUSED(filename) = "") {};
    void Startup();
    void Run();
    void Cleanup() {};

    bool GetExitFlag() const { return ExitFlag; }
};

CMN_DECLARE_SERVICES_INSTANTIATION(trackerSimulator);

#endif  // _trackerSimulator_h

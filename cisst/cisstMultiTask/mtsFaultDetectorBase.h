/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: mtsFaultDetectorBase.h 3034 2011-10-09 01:53:36Z adeguet1 $

  Author(s):  Min Yang Jung
  Created on: 2012-02-07

  (C) Copyright 2012 Johns Hopkins University (JHU), All Rights Reserved.

  --- begin cisst license - do not edit ---

  This software is provided "as is" under an open source license, with
  no warranty.  The complete license can be found in license.txt and
  http://www.cisst.org/cisst/license.txt.

  --- end cisst license ---
*/

/*!
  \file
  \brief Defines the base class for fault detection and diagnosis

  \ingroup cisstMultiTask
*/

#ifndef _mtsFaultDetectorBase_h
#define _mtsFaultDetectorBase_h

#include <cisstMultiTask/mtsMonitorFilterBase.h>

class mtsFaultDetectorBase: public mtsMonitorFilterBase
{
public:
    /*! Constructors and destructor */
    mtsFaultDetectorBase(const std::string & detectorName) 
        : mtsMonitorFilterBase(mtsMonitorFilterBase::FAULT_DETECTOR, detectorName) {}
    virtual ~mtsFaultDetectorBase() {}

    /*! Required by base class */
    void DoFiltering(bool debug = false);
    /*! Check if fault occurs */
    virtual void CheckFault(bool debug = false) = 0;
};

#endif // _mtsFaultDetectorBase_h

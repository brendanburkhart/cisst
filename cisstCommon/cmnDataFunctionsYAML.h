/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Anton Deguet
  Created on: 2011-06-27

  (C) Copyright 2011-2018 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


#pragma once
#ifndef _cmnDataFunctionsYAML_h
#define _cmnDataFunctionsYAML_h

#include <string.h> // for memcpy
#include <iostream>
#include <limits>
#include <vector>
#include <cisstConfig.h> // for CISST_HAS_YAML
#include <cisstCommon/cmnThrow.h>

// always include last
#include <cisstCommon/cmnExport.h>

#if CISST_HAS_YAML

#include <yaml-cpp/yaml.h>

namespace YAML {
    template<class _elementType, size_t _size> class convert<_elementType[_size]> {
    public:
        typedef _elementType arrayType[_size];
        static Node encode(const arrayType & data) {
            Node node;
            // data.SerializeYAML(node); 
            return node;
        }
        static bool decode(const Node & node, arrayType & data) {
            return true; // data.DeSerializeYAML(node);
        }
    };
}

#endif // CISST_HAS_YAML

#endif // _cmnDataFunctionsYAML_h

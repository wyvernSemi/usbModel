// =============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 28th October 2023
//
// This file is part of the usbModel package.
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
// =============================================================

//=============================================================
// VUserMain1.cpp
//=============================================================

#include <stdio.h>
#include <stdlib.h>

#include "usbDevice.h"

static int node = 1;

//-------------------------------------------------------------
// VUserMain1()
//
//-------------------------------------------------------------

extern "C" void VUserMain1()
{
    char sbuf[usbModel::ERRBUFSIZE];

    usbDevice dev(node);
    
    // Delay before connecting
    dev.usbDeviceSleepUs(50);

    // Run the device
    if (dev.usbDeviceRun() != usbModel::USBOK)
    {
        fprintf(stderr, "***ERROR: VUserMain1: runUsbDevice returned bad status\n");
        dev.usbPktGetErrMsg(sbuf);
        fprintf(stderr, "%s\n", sbuf);
        
        // Halt the simulation
        dev.usbDeviceEndExecution();
    }

    dev.usbDeviceSleepUs(usbDevice::SLEEP_FOREVER);

}


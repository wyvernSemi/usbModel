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
// VUserMain0.cpp
//=============================================================

#include <stdio.h>
#include <stdlib.h>

#include "usbHost.h"

static int node = 0;

static uint8_t rxdata     [usbModel::MAXBUFSIZE];
static char    scratchbuf [usbModel::ERRBUFSIZE];

//-------------------------------------------------------------
// VUserMain0()
//
//-------------------------------------------------------------

extern "C" void VUserMain0()
{
    unsigned             linestate;
    uint16_t             dev_status;
    uint8_t              addr         = 0;
    uint8_t              endp         = 0;
    uint16_t             rxlen;

    // Create host interface object to usbModel
    usbHost host(node);

    // Wait for reset to be deasserted
    host.waitOnNotReset();

    host.SendIdle(100);

    // Check that there is a connected device and it's a full speed endpoint
    if ((linestate = host.readLineState()) != usbModel::USB_J)
    {
        fprintf(stderr, "\nVUserMain0: ***ERROR: USB line state (%s) does not indicate a full speed device connected\n",
            usbModel::fmtLineState(linestate));
    }
    else
    {
        host.getDeviceDescriptor  (addr, endp, rxdata, 0x00FF, rxlen, false);
        usbModel::fmtDevDescriptor(scratchbuf, rxdata);
        fprintf(stderr, "\nVUserMain0: received device configuration\n\n%s", scratchbuf);

        /*
        host.getDeviceStatus(addr, endp, dev_status);


        fprintf(stderr, "\nVUserMain0: received device status of 0x%04x\n\n", dev_status);
        */
    }

    host.SendIdle(100);

    // Halt the simulation
    host.haltSimulation();


}


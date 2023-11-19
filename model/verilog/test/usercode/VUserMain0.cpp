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
    int                  linestate;
    uint16_t             dev_status;
    uint8_t              addr;
    uint8_t              endp;
    uint16_t             rxlen;

    // Create host interface object to usbModel
    usbHost host(node);

    // Wait for reset to be deasserted
    host.apiWaitOnNotReset();

    host.apiSendIdle(100);

    // Wait for a connection...
    if ((linestate = host.usbHostWaitForConnection()) != usbModel::USB_J)
    {
        // If an error occurred, then the method timed out waiting
        if (linestate == usbModel::USBERROR)
        {
            host.usbPktGetErrMsg(scratchbuf);
            fprintf(stderr, "\nVUserMain0: ***ERROR: %s\n", scratchbuf);
        }
        // If a valid linestate, but not J then not a full speed connection
        else
        {
            fprintf(stderr, "\nVUserMain0: ***ERROR: USB line state (%s) does not indicate a full speed device connected\n",
                usbModel::fmtLineState(linestate));
        }
    }
    // Successfully connected, so start generating traffic
    else
    {
        // Get the device descriptor
        host.usbHostGetDeviceDescriptor (usbModel::CONTROL_ADDR, usbModel::CONTROL_EP, rxdata, 0x00FF, rxlen, false);
        
        usbModel::fmtDevDescriptor(scratchbuf, rxdata);
        fprintf(stderr, "\nVUserMain0: received device descriptor\n\n%s", scratchbuf);

        // Set the device address to 1
        addr = 1;
        endp = 0;
        host.usbHostSetAddress(usbModel::CONTROL_ADDR, usbModel::CONTROL_EP, addr);

        fprintf(stderr, "\nVUserMain0: sent SET_ADDR (0x%04x)\n\n", addr);

        // Get the configuration descriptor. Device will want to return all descriptors under
        // the config descriptor, so just ask for the number of bytes required for the
        // first descriptor (the config)
        host.usbHostGetConfigDescriptor(addr, endp, rxdata, sizeof(usbModel::configDesc), rxlen, false);

        usbModel::fmtCfgDescriptor(scratchbuf, rxdata);
        fprintf(stderr, "\nVUserMain0: received config descriptor\n\n%s\n", scratchbuf);

        // Extract the total length of the combined descriptors
        usbModel::configDesc *pCfgDesc = (usbModel::configDesc *)rxdata;
        uint16_t wTotalLength = pCfgDesc-> wTotalLength;

        // Now request the lot
        host.usbHostGetConfigDescriptor(addr, endp, rxdata, wTotalLength, rxlen, false);

        usbModel::fmtCfgAllDescriptor(scratchbuf, rxdata);
        fprintf(stderr, "\nVUserMain0: received config descriptor\n\n%s", scratchbuf);

        // Get string descriptor 0
        host.usbHostGetStrDescriptor(addr, endp, 0, rxdata, 0xff, rxlen, false);

        fprintf(stderr, "\nVUserMain0: received string descriptor index 0\n");
        for (int idx = 0; idx < (rxlen-2)/2; idx++)
        {
            fprintf(stderr, "  wLANGID[%d] = 0x%04x\n", idx, ((uint16_t*)&rxdata[2])[idx]);
        }
        fprintf(stderr, "\n");

        // Get string descriptor 1
        host.usbHostGetStrDescriptor(addr, endp, 1, rxdata, 0xff, rxlen, false);

        fprintf(stderr, "\nVUserMain0: received string descriptor index 1\n");
        fprintf(stderr, "  \"%s\"\n\n", &rxdata);

        // Get string descriptor 2
        host.usbHostGetStrDescriptor(addr, endp, 2, rxdata, 0xff, rxlen, false);

        fprintf(stderr, "\nVUserMain0: received string descriptor index 2\n");
        fprintf(stderr, "  \"%s\"\n\n", &rxdata);

        // Get the device status
        host.usbHostGetDeviceStatus(addr, endp, dev_status);

        fprintf(stderr, "\nVUserMain0: received device status of 0x%04x\n\n", dev_status);
    }

    // Wait a bit before halting to let the device receive the ACK
    host.apiSendIdle(100);

    // Halt the simulation
    host.apiHaltSimulation();


}


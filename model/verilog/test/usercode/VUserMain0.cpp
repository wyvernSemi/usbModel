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
    uint16_t             status;
    uint8_t              dev_cfg;
    uint8_t              altif;
    uint8_t              addr;
    uint8_t              endp;
    uint16_t             rxlen;

    // Create host interface object to usbModel
    usbHost host(node);

    // Wait for a bit
    host.usbHostSleepUs(10);

    // Wait for a connection...
    if ((linestate = host.usbHostWaitForConnection()) != usbModel::USB_J)
    {
        // If an error occurred, then the method timed out waiting
        if (linestate == usbModel::USBERROR)
        {
            host.usbPktGetErrMsg(scratchbuf);
            USBDISPPKT ("\nVUserMain0: ***ERROR: %s\n", scratchbuf);
        }
        // If a valid line state, but not J, then not a full speed connection
        else
        {
            USBDISPPKT ("\nVUserMain0: ***ERROR: USB line state (%s) does not indicate a full speed device connected\n",
                usbModel::fmtLineState(linestate));
        }
    }
    // Successfully connected, so start generating traffic
    else
    {
        // Get the device descriptor
        host.usbHostGetDeviceDescriptor (usbModel::CONTROL_ADDR, usbModel::CONTROL_EP, rxdata, 0x00FF, rxlen, false);

        usbModel::fmtDevDescriptor(scratchbuf, rxdata);
        USBDISPPKT ("\nVUserMain0: received device descriptor\n\n%s", scratchbuf);

        // Set the device address to 1
        addr = 1;
        endp = 0;
        host.usbHostSetDeviceAddress(usbModel::CONTROL_ADDR, usbModel::CONTROL_EP, addr);

        USBDISPPKT ("\nVUserMain0: sent SET_ADDR (0x%04x)\n\n", addr);

        // Get the configuration descriptor. Device will want to return all descriptors under
        // the config descriptor, so just ask for the number of bytes required for the
        // first descriptor (the config)
        host.usbHostGetConfigDescriptor(addr, endp, rxdata, sizeof(usbModel::configDesc), rxlen, false);

        usbModel::fmtCfgDescriptor(scratchbuf, rxdata);
        USBDISPPKT ("\nVUserMain0: received config descriptor\n\n%s\n", scratchbuf);

        // Extract the total length of the combined descriptors
        usbModel::configDesc *pCfgDesc = (usbModel::configDesc *)rxdata;
        uint16_t wTotalLength = pCfgDesc-> wTotalLength;

        // Now request the lot
        host.usbHostGetConfigDescriptor(addr, endp, rxdata, wTotalLength, rxlen, false);

        usbModel::fmtCfgAllDescriptor(scratchbuf, rxdata);
        USBDISPPKT ("\nVUserMain0: received config descriptor\n\n%s", scratchbuf);

        // Get string descriptor 0
        host.usbHostGetStrDescriptor(addr, endp, 0, rxdata, 0xff, rxlen, false);

        USBDISPPKT ("\nVUserMain0: received string descriptor index 0\n");
        for (int idx = 0; idx < (rxlen-2)/2; idx++)
        {
            USBDISPPKT ("  wLANGID[%d] = 0x%04x\n", idx, ((uint16_t*)&rxdata[2])[idx]);
        }
        USBDISPPKT ("\n");

        // Get string descriptor 1
        host.usbHostGetStrDescriptor(addr, endp, 1, rxdata, 0xff, rxlen, false);

        USBDISPPKT ("\nVUserMain0: received string descriptor index 1\n");
        USBDISPPKT ("  \"%s\"\n\n", &rxdata);

        // Get string descriptor 2
        host.usbHostGetStrDescriptor(addr, endp, 2, rxdata, 0xff, rxlen, false);

        USBDISPPKT ("\nVUserMain0: received string descriptor index 2\n");
        USBDISPPKT ("  \"%s\"\n\n", &rxdata);

        // Get the device status
        host.usbHostGetDeviceStatus(addr, endp, status);

        USBDISPPKT ("\nVUserMain0: received device status of 0x%04x\n\n", status);

        // Get the device configurations status
        host.usbHostGetDeviceConfig(addr, endp, dev_cfg);

        USBDISPPKT ("\n\nVUserMain0: received device configuration of 0x%02x (%s)\n\n", dev_cfg, dev_cfg ? "enabled" : "disabled");

        // Set the device configuration
        host.usbHostSetDeviceConfig(addr, endp, 1);

        USBDISPPKT ("\nVUserMain0: set the device configuration for index 1\n\n");

        // Get the device configurations status
        host.usbHostGetDeviceConfig(addr, endp, dev_cfg);

        USBDISPPKT ("\n\nVUserMain0: received device configuration of 0x%02x (%s)\n\n", dev_cfg, dev_cfg ? "enabled" : "disabled");

        // Clear a device feature
        host.usbHostClearDeviceFeature(addr, endp, 0);

        // Set a device feature
        host.usbHostSetDeviceFeature(addr, endp, 1);

        // Get interface status
        host.usbHostGetInterfaceStatus(addr, endp, status);

        USBDISPPKT ("\nVUserMain0: received interface status of 0x%04x\n\n", status);

        host.usbHostClearInterfaceFeature(addr, endp, 0);
        host.usbHostSetInterfaceFeature(addr, endp, 0);

        host.usbHostGetInterface(addr, endp, 0, altif);

        USBDISPPKT ("\nVUserMain0: received get interface value of 0x%02x\n\n", altif);

        host.usbHostSetInterface(addr, endp, 0, 1);
        
        host.usbHostGetEndpointStatus(addr, endp, status);
        
        USBDISPPKT ("\nVUserMain0: received endpoint status of 0x%02x (%s)\n\n", status, status ? "halted" : "not halted");
        
        host.usbHostSetEndpointFeature(addr, endp, usbModel::EP_HALT_FEATURE);
        
        host.usbHostGetEndpointStatus(addr, endp, status);
        
        USBDISPPKT ("\nVUserMain0: received endpoint status of 0x%02x (%s)\n\n", status, status ? "halted" : "not halted");
        
        host.usbHostClearEndpointFeature(addr, endp, usbModel::EP_HALT_FEATURE);
        
        host.usbHostGetEndpointStatus(addr, endp, status);
        
        USBDISPPKT ("\nVUserMain0: received endpoint status of 0x%02x (%s)\n\n", status, status ? "halted" : "not halted");
        
        host.usbHostGetEndpointSynchFrame(addr, endp, status);
        
        USBDISPPKT ("\nVUserMain0: received endpoint synch frame of 0x%04x\n\n", status);

    }

    // Wait a bit before halting to let the device receive the ACK
    host.usbHostSleepUs(10);

    // Halt the simulation
    host.usbHostEndExecution();


}


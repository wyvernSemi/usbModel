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

static uint8_t databuf    [usbModel::MAXBUFSIZE];
static uint8_t cfgdescbuf [usbModel::MAXBUFSIZE];
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
    uint8_t              addr = 0;
    uint8_t              endp = 0;
    uint16_t             rxlen;

    usbModel::deviceDesc devdesc;

    // Create host interface object to usbModel
    usbHost host(node);

    // Wait for a bit
    host.usbHostSleepUs(10);

    //-------------------------------------------------------------
    // Wait for a connection...
    //-------------------------------------------------------------

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
        //-------------------------------------------------------------
        // Get the connected device's descriptor information
        //-------------------------------------------------------------

        // Get the device descriptor
        host.usbHostGetDeviceDescriptor (usbModel::CONTROL_ADDR, usbModel::CONTROL_EP, databuf, 0x00FF, rxlen, false);

        usbModel::fmtDevDescriptor(scratchbuf, databuf);
        USBDISPPKT ("\nVUserMain0: received device descriptor\n\n%s", scratchbuf);

        // Save off the descriptor for use later
        devdesc = *((usbModel::deviceDesc*)databuf);

        //-------------------------------------------------------------
        // Reset the device
        //-------------------------------------------------------------

        host.usbHostResetDevice();

        //-------------------------------------------------------------
        // Set the device's address to 1
        //-------------------------------------------------------------
        addr = 1;
        host.usbHostSetDeviceAddress(usbModel::CONTROL_ADDR, usbModel::CONTROL_EP, addr);

        USBDISPPKT ("\nVUserMain0: sent SET_ADDR (0x%04x)\n\n", addr);

        //-------------------------------------------------------------
        // Get the configuration descriptor.
        //-------------------------------------------------------------

        // Device will want to return all descriptors under the config
        // descriptor, so just ask for the number of bytes required for
        // the first descriptor (the configuration descriptor)
        host.usbHostGetConfigDescriptor(addr, endp, databuf, sizeof(usbModel::configDesc), rxlen, false);

        usbModel::fmtCfgDescriptor(scratchbuf, databuf);
        USBDISPPKT ("\nVUserMain0: received config descriptor\n\n%s\n", scratchbuf);

        // Extract the total length of the combined descriptors
        usbModel::configDesc *pCfgDesc = (usbModel::configDesc *)databuf;
        uint16_t wTotalLength = pCfgDesc-> wTotalLength;

        // Now request the lot
        host.usbHostGetConfigDescriptor(addr, endp, databuf, wTotalLength, rxlen, false);

        // Save off the descriptor data
        memcpy(cfgdescbuf, databuf, wTotalLength);

        usbModel::fmtCfgAllDescriptor(scratchbuf, databuf);
        USBDISPPKT ("\nVUserMain0: received config descriptor\n\n%s", scratchbuf);

        //-------------------------------------------------------------
        // Get the string descriptors
        //-------------------------------------------------------------

        // Get string descriptor 0
        host.usbHostGetStrDescriptor(addr, endp, 0, databuf, 0xff, rxlen, false);

        USBDISPPKT ("\nVUserMain0: received string descriptor index 0\n");
        for (int idx = 0; idx < (databuf[0]-2)/2; idx++)
        {
            USBDISPPKT ("  wLANGID[%d] = 0x%04x\n", idx, ((uint16_t*)&databuf[2])[idx]);
        }
        USBDISPPKT ("\n");

        // Get string descriptor 1
        host.usbHostGetStrDescriptor(addr, endp, 1, databuf, 0xff, rxlen, false);

        USBDISPPKT ("\nVUserMain0: received string descriptor index 1\n");
        USBDISPPKT ("  \"%s\"\n\n", (char*)&databuf);

        // Get string descriptor 2
        host.usbHostGetStrDescriptor(addr, endp, 2, databuf, 0xff, rxlen, false);

        USBDISPPKT ("\nVUserMain0: received string descriptor index 2\n");
        USBDISPPKT ("  \"%s\"\n\n", (char*)&databuf);

        //-------------------------------------------------------------
        // Get the device's status.
        // Indicates self-powered(bit 0) and remote wakeup (bit 1)
        // statuses
        //-------------------------------------------------------------

        host.usbHostGetDeviceStatus(addr, endp, status);

        USBDISPPKT ("\nVUserMain0: received device status of 0x%04x\n\n", status);

        //-------------------------------------------------------------
        // Get/set the device's configuration status.
        // Indicates/controls whether enabled or disabled.
        //-------------------------------------------------------------

        // Get the current device status
        host.usbHostGetDeviceConfig(addr, endp, dev_cfg);

        USBDISPPKT ("\n\nVUserMain0: received device configuration of 0x%02x (%s)\n\n", dev_cfg, dev_cfg ? "enabled" : "disabled");

        // Set the device configuration
        host.usbHostSetDeviceConfig(addr, endp, 1);

        USBDISPPKT ("\nVUserMain0: set the device configuration for index 1\n\n");

        // Get the device configurations status
        host.usbHostGetDeviceConfig(addr, endp, dev_cfg);

        USBDISPPKT ("\n\nVUserMain0: received device configuration of 0x%02x (%s)\n\n", dev_cfg, dev_cfg ? "enabled" : "disabled");

        //-------------------------------------------------------------
        // Set/clear device, interface and endpoint features
        //-------------------------------------------------------------

        // Clear a device feature
        host.usbHostClearDeviceFeature(addr, endp, 0);

        // Set a device feature
        host.usbHostSetDeviceFeature(addr, endp, 1);

        // Get interface0 status
        host.usbHostGetInterfaceStatus(addr, endp, 0, status);

        USBDISPPKT ("\nVUserMain0: received interface status of 0x%04x\n\n", status);

        host.usbHostClearInterfaceFeature(addr, endp, 0, 0);
        host.usbHostSetInterfaceFeature(addr, endp, 0, 0);

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

        //-------------------------------------------------------------
        // Get an endpoint's synch frame number
        //-------------------------------------------------------------

        host.usbHostGetEndpointSynchFrame(addr, endp, status);

        USBDISPPKT ("\nVUserMain0: received endpoint synch frame of 0x%04x\n\n", status);

        //-------------------------------------------------------------
        // Do some BULK transfers
        //-------------------------------------------------------------

        // Send some data
        endp = 1;

        // Define some endpoint descriptors for the endpoint, OUT and IN
        usbModel::endpointDesc epdesc1_OUT, epdesc1_IN;

        // Extract the enddpoint decriptors from the saved configuration descriptor data
        host.usbHostFindDescriptor(usbModel::EP_DESCRIPTOR_TYPE, endp | usbModel::DIRTODEV,  cfgdescbuf, wTotalLength, (uint8_t*)&epdesc1_OUT);
        host.usbHostFindDescriptor(usbModel::EP_DESCRIPTOR_TYPE, endp | usbModel::DIRTOHOST, cfgdescbuf, wTotalLength, (uint8_t*)&epdesc1_IN);

        for (int idx = 0; idx < 56; idx++)
        {
            databuf[idx] = idx;
        }
        host.usbHostBulkDataOut(addr, endp, databuf, 56, epdesc1_OUT.wMaxPacketSize);

        // Fetch some data
        endp = 0x81;
        host.usbHostBulkDataIn(addr, endp, databuf, 64, epdesc1_IN.wMaxPacketSize);

         USBDISPPKT ("\nVUserMain0: received data from device:\n");

         for (int idx = 0; idx < 64; idx++)
         {
             if ((idx % 16) == 0)
             {
                 USBDISPPKT ("\n");
             }

             USBDISPPKT (" %02x", databuf[idx]);
         }
         USBDISPPKT ("\n\n");

         //-------------------------------------------------------------
         // Suspend device
         //-------------------------------------------------------------

         host.usbHostSuspendDevice();

    }

    // Wait a bit before halting to let the device receive any last ACK
    host.usbHostSleepUs(10);

    // Halt the simulation
    host.usbHostEndExecution();


}


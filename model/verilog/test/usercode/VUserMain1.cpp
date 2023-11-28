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
// dataCallback
//
// Call back function called by device model whenever a data
// transfer transaction is received. When an OUT transaction
// the endp will indicate the end point being indexed (bits 3:0),
// and the direction bit (bit 7) will be clear. The data argument
// will conating the received data, with its length indicated in
// numbytes. When an IN transfer, the endp argument will
// indicate the end point being indexed (bits 3:0) and the
// direction bit (bit 7) will be set. Data is returned in the
// data buffer and the the returned length indicated by setting
// numbytes. The function should return the appropriate
// acknowledge status. One of:
//
//    usbModel::USBACK      sent when transfer is okay
//    usbModel::USBNAK      sent when no transfer is ready to be sent/accepted
//    usbModel::USBSTALL    sent if some error condition occured
//
//-------------------------------------------------------------

usbDevice::dataResponseType_e dataCallback (const uint8_t endp, uint8_t* data, int &numbytes)
{
    int     idx;

    // If an IN transfer, generate some data
    if (endp & 0x80)
    {
        numbytes = 32;
                
        USBDISPPKT("\n**dataCallback**: IN request endpoint = 0x%02x sending = %d bytes\n", endp, numbytes);

        for (idx = 0; idx < numbytes; idx++)
        {
            data[idx] = idx;
        }
    }
    // If an OUT transfer, display the data.
    else
    {
        USBDISPPKT("\n**dataCallback**: OUT request endpoint = 0x%02x numbytes = %d\n", endp, numbytes);
        
        for (idx = 0; idx < numbytes; idx++)
        {

            if ((idx % 16) == 0)
            {
                USBDISPPKT("\n");
            }
            USBDISPPKT(" %02x", data[idx]);
        }

        if (idx % 16 != 1)
        {
            USBDISPPKT(FMT_NORMAL "\n");
        }
        else
        {
            USBDISPPKT(FMT_NORMAL);
        }

        USBDISPPKT("\n");
    }

    return usbDevice::ACK;
}

//-------------------------------------------------------------
// VUserMain1()
//
// Enrty point for device user code (device is on node 1)
//
//-------------------------------------------------------------

extern "C" void VUserMain1()
{
    char sbuf[usbModel::ERRBUFSIZE];

    // Create a device model object on this node, registering the data callback function
    usbDevice dev(node, dataCallback);

    // Delay some ticks before connecting
    dev.usbDeviceSleepUs(50);

    // Run the device. Will return if some unhandled exception condition occurred,
    // othwerwise it will run indefinitely, processing input packets
    if (dev.usbDeviceRun() != usbModel::USBOK)
    {
        fprintf(stderr, "***ERROR: VUserMain1: runUsbDevice returned bad status\n");
        
        // Retreived the error message from the device model and display
        dev.usbPktGetErrMsg(sbuf);
        fprintf(stderr, "%s\n", sbuf);

        // Halt the simulation
        dev.usbDeviceEndExecution();
    }

    // If the device model returned, sleep for ever (line is idle fro device)
    dev.usbDeviceSleepUs(usbDevice::SLEEP_FOREVER);

}


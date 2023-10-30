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

#include "usbPliApi.h"

static int node = 0;


static usbPliApi::usb_signal_t nrzi[usbPliApi::MAXBUFSIZE];

//-------------------------------------------------------------
// VUserMain0()
//
//-------------------------------------------------------------

extern "C" void VUserMain0()
{
    int numbits;
    uint8_t addr;
    uint8_t endp;

    int                  pid;
    uint32_t             args[4];
    uint8_t              rxdata[4096];
    int                  databytes;
    char                 sbuf[1024];

    // Create interface object to usbModel
    usbPliApi usbapi(node);

    // Wait for reset to be deasserted
    usbapi.waitOnNotReset();

    usbapi.SendIdle(100);

    // SETUP
    addr    = 0x0;
    endp    = 0x0;
    numbits = usbapi.genPkt(nrzi, usbPliApi::PID_TOKEN_SETUP, addr, endp);

    usbapi.SendPacket(nrzi, numbits);
    usbapi.SendIdle(5);

    // DATA0
    uint8_t data[8] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x40, 0x00};
    int len = 8;
    numbits = usbapi.genPkt(nrzi, usbPliApi::PID_DATA_0, data, len);
    usbapi.SendPacket(nrzi, numbits);

    usbapi.SendIdle(1);

    // Wait for ACK
    usbapi.waitForPkt(nrzi);

    if (usbapi.decodePkt(nrzi, pid, args, rxdata, databytes) != usbPkt::USBOK)
    {
        fprintf(stderr, "***ERROR: VUserMain0: received bad packet\n");
        usbapi.getUsbErrMsg(sbuf);
        fprintf(stderr, "%s\n", sbuf);
    }


    usbapi.SendIdle(50);

    // Halt the simulation
    VWrite(UVH_FINISH, 0, 0, node);

}


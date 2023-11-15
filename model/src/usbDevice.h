//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 31st October 2023
//
// Contains the headers for the usbModel device endpoint
//
// This file is part of the C++ usbModel
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================


#include <cstring>

#include "usbCommon.h"
#include "usbPkt.h"
#include "usbPliApi.h"

class usbDevice : public usbPliApi, public usbPkt
{
public:

    static const int      PID_NO_CHECK             = usbModel::PID_INVALID;
    static const int      DEFAULT_IDLE             = 36;

    // Constructor
    usbDevice (int nodeIn, std::string name = std::string(FMT_DEVICE "DEV " FMT_NORMAL)) :
        usbPliApi(nodeIn, name),
        usbPkt(name),
        numendpoints(1),
        remoteWakeup(usbModel::USB_REMOTE_WAKEUP_OFF),
        selfPowered(usbModel::USB_NOT_SELF_POWERED),
        timeSinceLastSof(0)
    {
        reset();
    };

    // User entry method to start the USB device model
    int runUsbDevice(const int idle = DEFAULT_IDLE);

private:

    // Reset method, called on detecting a reset state on the line
    void reset()
    {
        usbPliApi::reset();
        devaddr   = usbModel::USB_NO_ASSIGNED_ADDR;
    }

    // Methods for sending packets back towards the host
    void         sendPktToHost         (const int pid, const uint8_t  data[],   unsigned  datalen, const int idle = DEFAULT_IDLE);   // DATA
    void         sendPktToHost         (const int pid, const uint8_t  addr,     uint8_t   endp,    const int idle = DEFAULT_IDLE);   // Token
    void         sendPktToHost         (const int pid, const uint16_t framenum, const int idle = DEFAULT_IDLE);                      // SOF
    void         sendPktToHost         (const int pid, const int      idle = DEFAULT_IDLE);                                          // Handshake

    // Method to wait for the receipt of a particular packet type
    int          waitForExpectedPacket (const int pktType, int &pid, uint32_t args[], uint8_t data[], int databytes, bool ignorebadpkts = true);

    // Methods for processing different packet types
    int          processControl        (const uint32_t addr,   const uint32_t endp,   const int idle = DEFAULT_IDLE);
    int          processIn             (const uint32_t args[], const uint8_t  data[], const int databytes, const int idle = DEFAULT_IDLE);
    int          processOut            (const uint32_t args[],       uint8_t  data[], const int databytes, const int idle = DEFAULT_IDLE);
    int          processSOF            (const uint32_t args[], const int      idle = DEFAULT_IDLE);
 
    // Method for handling device requests
    int          handleDevReq          (const usbModel::setupRequest* sreq, const int idle = DEFAULT_IDLE);

    // Internal device state
    int          devaddr;
    uint8_t      numendpoints;
    uint8_t      remoteWakeup;
    uint8_t      selfPowered;

    int          timeSinceLastSof;

    // Internal buffers for use by class methods
    uint8_t                rxdata [usbModel::MAXBUFSIZE];
    usbModel::usb_signal_t nrzi   [usbModel::MAXBUFSIZE];
    char                   sbuf   [usbModel::ERRBUFSIZE];

};
//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 9th Novenmber 2023
//
// Contains the code for the usbModel host class header
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

#ifndef _USB_HOST_H_
#define _USB_HOST_H_

#include <cstring>

#include "usbCommon.h"
#include "usbPkt.h"
#include "usbPliApi.h"

class usbHost  : public usbPliApi, public usbPkt
{
public:

    static const int DEFAULTIDLEDELAY = 4; // 0.33us at 12MHz

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    usbHost (int nodeIn, std::string name = std::string(FMT_HOST "HOST" FMT_NORMAL)) :
        usbPliApi(nodeIn, name),
        usbPkt(name)
    {
    }

    // -------------------------------------------------------------------------
    // Public methods
    // -------------------------------------------------------------------------
public:
    int  getDeviceStatus      (const uint8_t  addr,    const uint8_t  endp,           uint16_t &status,    const unsigned idle = DEFAULTIDLEDELAY);
    int  getDeviceDescriptor  (const uint8_t  addr,    const uint8_t  endp,           uint8_t data[],      const uint16_t reqlength,
                                     uint16_t &rxlen,  const bool     chklen = true,  const unsigned idle = DEFAULTIDLEDELAY);
    
    // -------------------------------------------------------------------------
    // Private methods
    // -------------------------------------------------------------------------
private:
    void sendTokenToDevice    (const int     pid,      const uint8_t  addr,   const uint8_t  endp,       const unsigned idle = DEFAULTIDLEDELAY);
    int  sendDataToDevice     (const int     datatype, const uint8_t  data[], const int      len,        const unsigned idle = DEFAULTIDLEDELAY);
    int  getDataFromDevice    (const int     expPID,         uint8_t  data[],       int      &databytes, const unsigned idle = DEFAULTIDLEDELAY);
    int  sendGetStatusRequest (const uint8_t addr,     const uint8_t  endp,                              const unsigned idle = DEFAULTIDLEDELAY);
    int  sendGetDevDescRequest(const uint8_t addr,     const uint8_t  endp,   const uint16_t length,     const unsigned idle = DEFAULTIDLEDELAY);
    int  sendDeviceRequest    (const uint8_t addr,     const uint8_t  endp,
                               const uint8_t request,  const uint16_t length, const uint16_t value = 0,  const uint16_t index = 0,
                               const unsigned idle = DEFAULTIDLEDELAY);

    // -------------------------------------------------------------------------
    // Internal private state
    // -------------------------------------------------------------------------
private:
    // Internal buffers for use by class methods
    usbModel::usb_signal_t nrzi   [usbModel::MAXBUFSIZE];
    uint8_t                rxdata [usbModel::MAXBUFSIZE];
    char                   sbuf   [usbModel::ERRBUFSIZE];

};


#endif
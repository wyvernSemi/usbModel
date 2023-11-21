//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 9th November 2023
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

class usbHost : public usbPliApi, public usbPkt
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

    // Device control methods
    int  usbHostWaitForConnection     (const unsigned polldelay = 10*ONE_US,
                                       const unsigned timeout = 3*ONE_MS);

    int  usbHostGetDeviceStatus       (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &status,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetDeviceDescriptor   (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  data[],    const uint16_t reqlength,
                                             uint16_t &rxlen,    const bool     chklen = true, const
                                             unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetDeviceConfig       (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  &cfgstate,
                                       const uint8_t  index = 1,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetDeviceConfig       (const uint8_t  addr,      const uint8_t  endp,
                                       const uint8_t  index,
                                       const unsigned idle = DEFAULTIDLEDELAY);


    int  usbHostClearDeviceFeature    (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetDeviceFeature      (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t  feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetConfigDescriptor   (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  data[],    const uint16_t reqlen,
                                             uint16_t &rxlen,    const bool     chklen = true,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetStrDescriptor      (const uint8_t  addr,      const uint8_t  endp,
                                       const uint8_t  strindex,  uint8_t        data[],
                                       const uint16_t reqlen,    uint16_t       &rxlen,
                                       const bool     chklen = true,
                                       const uint16_t langid = 0x0809,
                                       const unsigned idle   = DEFAULTIDLEDELAY);

    int  usbHostSetAddress            (const uint8_t  addr,      const uint8_t  endp,
                                       const uint16_t wValue,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    // Interface control methods
    int  usbHostGetInterfaceStatus    (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &status,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostClearInterfaceFeature (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetInterfaceFeature   (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetInterface          (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t index,           uint8_t &altif,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetInterface          (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t index,     const uint8_t altif,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    // Endpoint control methods
    int  usbHostGetEndpointStatus     (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &status,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostClearEndpointFeature  (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);
                                       
    int  usbHostSetEndpointFeature    (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);
                                       
    int  usbHostGetEndpointSynchFrame (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &framenum,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    // -------------------------------------------------------------------------
    // Private methods
    // -------------------------------------------------------------------------
private:
    void sendTokenToDevice            (const int      pid,        const uint8_t  addr,    const uint8_t  endp,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  sendDataToDevice             (const int      datatype,   const uint8_t data[],
                                       const int      len,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  getDataFromDevice            (const int      expPID,          uint8_t  data[],
                                             int      &databytes,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  sendStandardRequest          (const uint8_t  addr,      const uint8_t  endp,
                                       const uint8_t  reqtype,   const uint8_t  request,
                                       const uint16_t value = 0, const uint16_t index = 0, const uint16_t length = 0,
                                       const unsigned idle  = DEFAULTIDLEDELAY);

    int  getStatus                    (const uint8_t addr,       const uint8_t  endp,
                                       const uint8_t type,             uint16_t &status,
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
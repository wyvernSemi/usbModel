//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 19th October 2023
//
// Contains the headers for the USB packet generation and NRZI encoding
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

#include <string>
#include <stdint.h>

#include "usbCommon.h"
#include "usbFormat.h"

#ifndef _USBPKT_H_
#define _USBPKT_H_

class usbPkt
{
public:

    // Constructor
    usbPkt(std::string _name = "ENDP") : rawbuf(), errbuf{ 0 }
    {
        name = _name;
        reset();
    }

    void getUsbErrMsg(char* msg)
    {
        sprintf(msg, "%s", errbuf);
    }

    // Packet generation methods
    int          genUsbPkt    (usbModel::usb_signal_t nrzibuf[], const int pid);                                              // Handshake
    int          genUsbPkt    (usbModel::usb_signal_t nrzibuf[], const int pid, const uint8_t  addr,   const uint8_t endp);   // Token
    int          genUsbPkt    (usbModel::usb_signal_t nrzibuf[], const int pid, const uint16_t framenum);                     // SOF
    int          genUsbPkt    (usbModel::usb_signal_t nrzibuf[], const int pid, const uint8_t  data[], const unsigned len);   // Data

    // Packet decode method
    int          decodePkt (const usbModel::usb_signal_t nrzibuf[], int& pid, uint32_t args[], uint8_t data[], int &databytes);

protected:

    void         reset    (void)
    {
        currspeed = usbModel::usb_speed_e::FS;
    }

    // Buffer for error messages
    char         errbuf[usbModel::ERRBUFSIZE];

    std::string name;

private:

    // CRC generation methods
    int          usbcrc16(const usbModel::usb_signal_t data[], const unsigned len = 1, const unsigned crcinit = 0xffff);
    int          usbcrc5 (const usbModel::usb_signal_t data[], const unsigned len = 1, const int      endbits = 8, const unsigned crcinit = 0x1f);

    // Bit reversal utility method
    uint32_t     bitrev  (const uint32_t     data,   const int      bits);

    // NRZI methods
    int          nrziEnc (const usbModel::usb_signal_t raw[],  usbModel::usb_signal_t   nrzi[],  const unsigned len,         const int start = 1);
    int          nrziDec (const usbModel::usb_signal_t nrzi[], usbModel::usb_signal_t   raw[],   const int start = 1);

    // Debug method to return differential signal state as printable character: K, J, SE0 (0) or SE1 (1)
    char bitenc(usbModel::usb_signal_t raw, const int bit)
    {
        uint8_t se = ~(raw.dp ^ raw.dm);

        return ((se &  raw.dp) & (1 << bit)) ? '1' :
               ((se & ~raw.dp) & (1 << bit)) ? '0' :
               (raw.dp         & (1 << bit)) ? 'J' :
                                               'K';
    }

    // Internal buffer for constructing raw, non-NRZI encoded packets
    usbModel::usb_signal_t rawbuf [usbModel::MAXBUFSIZE];

    // State of current line seed
    usbModel::usb_speed_e  currspeed;

};

#endif

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

#ifndef _USBPKT_H_
#define _USBPKT_H_

class usbPkt {
public:

    // Basic Differential line signal, as bytes
    typedef struct
    {
        uint8_t dp;
        uint8_t dm;
    } usb_signal_t;

    // Line speed types
    enum class usb_speed_e
    {
        LS,
        FS,
        HS
    };

    // USB1.0
    static const int      PID_SPCL_PREAMB   = 0xc;

    // USB1.1
    static const int      NRZI_BITSPERBYTE  = 8;
    static const int      NRZI_BYTELSBMASK  = 1;
    static const int      NRZI_BYTEMSBMASK  = 0x80;

    static const int      PID_TOKEN_OUT     = 0x1;
    static const int      PID_TOKEN_IN      = 0x9;
    static const int      PID_TOKEN_SOF     = 0x5;
    static const int      PID_TOKEN_SETUP   = 0xd;

    static const int      PID_DATA_0        = 0x3;
    static const int      PID_DATA_1        = 0xb;

    static const int      PID_HSHK_ACK      = 0x2;
    static const int      PID_HSHK_NAK      = 0xa;
    static const int      PID_HSHK_STALL    = 0xe;

    // USB2.0
    static const int      PID_HSHK_NYET     = 0x6;
    static const int      PID_TOKEN_ERR     = 0xc;
    static const int      PID_TOKEN_SPLIT   = 0x8;
    static const int      PID_TOKEN_PING    = 0x4;
    static const int      PID_DATA_2        = 0x7;
    static const int      PID_DATA_M        = 0xf;
    
    static const int      PID_INVALID       = -1;

    static const int      SYNC              = 0x80;

    static const int      SE0P              = 0xfc;
    static const int      SE0M              = 0x00;

    static const int      MAXDEVADDR        = 127;
    static const int      MAXENDP           = 15;
    static const int      MAXFRAMENUM       = 4095;

    static const int      POLY16            = 0x8005;
    static const unsigned BIT16             = 0x8000U;
                                           
    static const int      POLY5             = 0x5;
    static const unsigned BIT5              = 0x10U;

    static const int      USBOK             = 0;
    static const int      USBERROR          = -1;
    static const int      USBUNSUPPORTED    = -2;
    static const int      USBRESET          = -3;
    static const int      USBSUSPEND        = -4;
    static const int      ERRBUFSIZE        = 512;
    static const int      MAXBUFSIZE        = 2048;

    static const int      MINPKTSIZEBITS    = 16;

    static const int      SYNCBYTEOFFSET    = 0;
    static const int      PIDBYTEOFFSET     = 1;
    static const int      FRAMEBYTEOFFSET   = 2;
    static const int      ADDRBYTEOFFSET    = 2;
    static const int      ENDPBYTEOFFSET    = 2;
    static const int      DATABYTEOFFSET    = 2;
    static const int      CRC5BYTEOFFSET    = 3;

    static const int      ARGADDRIDX        = 0;
    static const int      ARGENDPIDX        = 1;
    static const int      ARGTKNCRC5IDX     = 2;

    static const int      ARGFRAMEIDX       = 0;
    static const int      ARGSOFCRC5IDX     = 1;

    static const int      ARGCRC16IDX       = 0;
    
    static const int      USB_SE0           = 0;
    static const int      USB_SE1           = 3;
    static const int      USB_J             = 1;
    static const int      USB_K             = 2;

public:

    // Constructor
    usbPkt(std::string _name = "ENDP") : rawbuf(), errbuf{ 0 }
    {
        name = _name;
        reset();
    }

    // Debug method to return differential signal state as printable character: K, J, SE0 (0) or SE1 (1)
    static char bitenc(usbPkt::usb_signal_t raw, const int bit)
    {
        uint8_t se = ~(raw.dp ^ raw.dm);

        return ((se &  raw.dp) & (1 << bit)) ? '1' :
               ((se & ~raw.dp) & (1 << bit)) ? '0' :
               (raw.dp         & (1 << bit)) ? 'J' :
                                               'K';
    }

    // Line speed methods
    int setLineSpeed(usb_speed_e speed)
    {
        // Check validity of argument
        switch (speed)
        {
        case usb_speed_e::LS:
        case usb_speed_e::FS:
        case usb_speed_e::HS:
            break;
        default:
            USBERRMSG("setLineSpeed: Unrecognised line speed (%d)\n", speed);
            return USBERROR;
            break;
        }

        // Update speed
        currspeed = speed;

        return USBOK;
    }

    void getUsbErrMsg(char* msg)
    {
        sprintf(msg, "%s", errbuf);
    }


    // Packet generation methods
    int          genPkt(usb_signal_t nrzibuf[], const int pid);                                              // Handshake
    int          genPkt(usb_signal_t nrzibuf[], const int pid, const uint8_t  addr,   const uint8_t endp);   // Token
    int          genPkt(usb_signal_t nrzibuf[], const int pid, const uint16_t framenum);                     // SOF
    int          genPkt(usb_signal_t nrzibuf[], const int pid, const uint8_t  data[], const unsigned len);   // Data

    // Packet decode method
    int          decodePkt(const usb_signal_t nrzibuf[], int& pid, uint32_t args[], uint8_t data[], int &databytes);

private:
    // CRC generation methods
    int          usbcrc16(const usb_signal_t data[], const unsigned len = 1, const unsigned crcinit = 0xffff);
    int          usbcrc5 (const usb_signal_t data[], const unsigned len = 1, const int      endbits = 8, const unsigned crcinit = 0x1f);

    // Bit reversal utility method
    uint32_t     bitrev  (const uint32_t     data,   const int      bits);


protected:

    void         reset(void)
    {
        currspeed = usb_speed_e::FS;
    }

    // Buffer for error messages
    char         errbuf[ERRBUFSIZE];
    
    std::string name;

private:
    // NRZI methods
    int          nrziEnc (const usb_signal_t raw[],  usb_signal_t   nrzi[],  const unsigned len,         const int start = 1);
    int          nrziDec (const usb_signal_t nrzi[], usb_signal_t   raw[],   const int start = 1);

    // Internal buffer for constructing raw, non-NRZI encoded packets
    usb_signal_t rawbuf[MAXBUFSIZE];

    // State of current line seed
    usb_speed_e  currspeed;

};

#endif

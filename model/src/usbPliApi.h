//=============================================================
// 
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 19th October 2023
//
// This file is part of C++ usbModel pattern generator.
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

#include <stdint.h>

#include "usbPkt.h"
#include "usbMap.h"

extern "C"
{
#include "VUser.h"
}

#ifndef _USB_PLI_API_H_
#define _USB_PLI_API_H_

class usbPliApi : public usbPkt
{
private:

    static const int IDLE_FOREVER = 0;
    static const int ADVANCE_TIME = 0;

public:

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    usbPliApi(const int nodeIn) : node(nodeIn)
    {
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    unsigned getClkCount(int delta = DELTA_CYCLE)
    {
        unsigned clkCount;

        VRead(CLKCOUNT, &clkCount, delta, node);

        return clkCount;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    unsigned readLineState(int delta = 0)
    {
        unsigned rawline;

        VRead(LINE, &rawline, delta, node);

        return rawline;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void SendIdle(const unsigned ticks = 1)
    {
        unsigned time;
        unsigned currtime;

        // Sample Current clock count
        time = getClkCount();

        // Disable outputs
        VWrite(OUTEN, 0, DELTA_CYCLE, node);

        // Keep reading clock count for 'ticks' number of cycles
        do {
            currtime = getClkCount(ADVANCE_TIME);
        } while ((ticks == IDLE_FOREVER) || ((currtime-time) < ticks));
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void SendPacket(const usbPkt::usb_signal_t nrzi[], const int bitlen)
    {
        // Enable outputs
        VWrite(OUTEN, 1, DELTA_CYCLE, node);

        // Number of bytes in data, rounded up.
        int bytelen  = ((bitlen+7)/8);

        // Number of bits in last byte
        int lastbits = (bitlen & 0x7) ? (bitlen & 0x7) : 8;

        for (int bytes = 0; bytes < bytelen; bytes++)
        {
            bool lastbyte = (bytes == (bytelen-1));
            int bitcnt    = lastbyte ? lastbits : 8;

            for (int bits = 0; bits < bitcnt; bits++)
            {
                if (lastbyte && bits == (bitcnt-1))
                {
                    // Disable outputs on last bit
                    VWrite(OUTEN, 0, DELTA_CYCLE, node);
                }

                // Output data values
                unsigned lineval = ((nrzi[bytes].dp >> bits) & 1) | (((nrzi[bytes].dm >> bits) & 1) << 1);
                VWrite(LINE, lineval, ADVANCE_TIME, node);
            }
        }
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------
    int waitForPkt(usb_signal_t nrzi[])
    {
        unsigned     line;
        bool         idle      = true;
        int          eop_count = 0;
        int          bitcount  = 0;

        // Disable outputs
        VWrite(OUTEN, 0, DELTA_CYCLE, node);

        do {
            line = readLineState();

            if (line == USB_K)
                idle = false;

            if (!idle)
            {
                //fprintf(stderr, "%c", (line == USB_K) ? 'K' : (line == USB_J) ? 'J' : (line == USB_SE0) ? '0' : 'X');

                if (!(bitcount%8))
                {
                    nrzi[bitcount/8].dp = nrzi[bitcount/8].dm = 0;
                }

                nrzi[bitcount/8].dp |= (line        & 0x1) << bitcount%8;
                nrzi[bitcount/8].dm |= ((line >> 1) & 0x1) << bitcount%8;
                bitcount++;

                if (!eop_count && line == USB_SE0)
                {
                    eop_count++;
                }
                else if (eop_count)
                {
                    eop_count++;

                    if (eop_count == 3)
                    {
                        break;
                    }
                }
            }
        } while (true);

        return bitcount;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void waitOnNotReset(void)
    {
        unsigned reset;

        do {
            VRead(RESET_STATE, &reset, ADVANCE_TIME, node);
        } while (reset);
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void enablePullup(void)
    {
        VWrite(PULLUP, 1, ADVANCE_TIME, node);
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void disablePullup(void)
    {
        VWrite(PULLUP, 0, ADVANCE_TIME, node);
    }


private:

    int node;

};

#endif
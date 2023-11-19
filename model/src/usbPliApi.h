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

#include <string>
#include <stdint.h>

#include "usbCommon.h"
#include "usbFormat.h"
#include "usbMap.h"

extern "C"
{
#include "VUser.h"
}

#ifndef _USB_PLI_API_H_
#define _USB_PLI_API_H_

class usbPliApi
{
public:
    static const int ONE_US          = 12;
    static const int ONE_MS          = ONE_US * 1000;
private:

    static const int IDLE_FOREVER = 0;
    static const int ADVANCE_TIME = 0;
    static const int MINIMUMIDLE  = 1;

#ifndef USBTESTMODE
    static const int MINRSTCOUNT     = ONE_MS * 10;  // 10ms for fullspeed device
    static const int MINSUSPENDCOUNT = ONE_MS * 3;   // 3ms for fullspeed device
#else
    static const int MINRSTCOUNT     = ONE_US * 25;
    static const int MINSUSPENDCOUNT = ONE_US * 100;
#endif

public:

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    usbPliApi(const int nodeIn, std::string name = std::string("ENDP")) : node(nodeIn)
    {
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    unsigned apiGetClkCount(int delta = DELTA_CYCLE)
    {
        unsigned clkCount;

        VRead(CLKCOUNT, &clkCount, delta, node);

        return clkCount;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    unsigned apiReadLineState(int delta = 0)
    {
        unsigned rawline;

        VRead(LINE, &rawline, delta, node);

        return rawline;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiSendIdle(const unsigned ticks = 1)
    {
        unsigned time;
        unsigned currtime;

        // Sample Current clock count
        time = apiGetClkCount();

        // Disable outputs
        VWrite(OUTEN, 0, DELTA_CYCLE, node);

        // Keep reading clock count for 'ticks' number of cycles
        do {
            currtime = apiGetClkCount(ADVANCE_TIME);
        } while ((ticks == IDLE_FOREVER) || ((currtime-time) < ticks));
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiSendPacket(const usbModel::usb_signal_t nrzi[], const int bitlen, const int delay = 50)
    {
        if (delay >= MINIMUMIDLE)
        {
            apiSendIdle(delay);
        }
        else
        {
            apiSendIdle(MINIMUMIDLE);
        }

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
    // apiWaitForPkt()
    //
    // Monitors the USB line for a new packet and saves the raw
    // NRZI data into the provided buffer, returning the bit count
    // of the extracted packet. Will also detect a reset state on
    // the line (continuous SE0s for a minimum period), and flag to
    // the calling code.
    //
    //-------------------------------------------------------------

    int apiWaitForPkt(usbModel::usb_signal_t nrzi[])
    {
        unsigned     line;
        bool         idle         = true;
        bool         lookforreset = false;
        int          rstcount     = 0;
        int          idlecount    = 0;
        int          eop_count    = 0;
        int          bitcount     = 0;

        // Disable outputs
        VWrite(OUTEN, 0, DELTA_CYCLE, node);

        do {
            // Get status on USB line
            line = apiReadLineState();

            // If anything other than a J (when already idle) come out of suspend
            if (suspended && (line == usbModel::USB_K || line == usbModel::USB_SE0))
            {
                USBDISPPKT("Device activated from suspension\n");
                suspended = false;
            }

            // If not in the middle of a reset detection and K seen, then line is not idle
            if (!lookforreset && line == usbModel::USB_K)
            {
                idle = false;
            }
            // If idle and an SE0 seem then this may be a reset
            else if (idle && line == usbModel::USB_SE0)
            {
                lookforreset = true;
                rstcount++;
            }

            // If in the middle of a potential reset, keep track of the number of consecutive SE0 line states
            if (lookforreset)
            {
                // Another SE0 increments the count
                if (line == usbModel::USB_SE0)
                {
                    rstcount++;
                }
                // When a non-SE0 state occurs, return the status to the calling method
                else
                {
                    // The return status is reset if seen sufficient consecutive SE0s,
                    // else flag an error.
                    rstcount = 0;

                    if (rstcount >= MINRSTCOUNT)
                    {
                        USBDISPPKT("Device reset\n");
                        return usbModel::USBRESET;
                    }

                    lookforreset = false;
                    idle         = (line == usbModel::USB_K) ? false : true;
                }
            }

            // If not idle, then process the packet data
            if (!idle)
            {
                idlecount = 0;

                // At each byte boundary, clear the new byte buffer entry
                if (!(bitcount%8))
                {
                    nrzi[bitcount/8].dp = nrzi[bitcount/8].dm = 0;
                }

                // Add the line values to the buffer entries and increment the packet bit count
                nrzi[bitcount/8].dp |= (line        & 0x1) << bitcount%8;
                nrzi[bitcount/8].dm |= ((line >> 1) & 0x1) << bitcount%8;
                bitcount++;

                // If not yet seen an SE0, the keep a bit count for 3 bits for EOP
                // (the decoding of the packet will detect any correupt EOP)
                if (!eop_count && line == usbModel::USB_SE0)
                {
                    eop_count++;
                }
                else if (eop_count)
                {
                    eop_count++;

                    // After 3 bits of EOP break out of the loop.
                    if (eop_count == 3)
                    {
                        break;
                    }
                }
            }
            else
            {
                if (++idlecount >= MINSUSPENDCOUNT)
                {
                    USBDISPPKT("Device suspended\n");
                    suspended = true;
                    return usbModel::USBSUSPEND;
                }
            }
        } while (true);

        // Return the bitcount of the packet
        return bitcount;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiWaitOnNotReset(void)
    {
        unsigned reset;

        do {
            VRead(RESET_STATE, &reset, ADVANCE_TIME, node);
        } while (reset);
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiEnablePullup(void)
    {
        VWrite(PULLUP, 1, ADVANCE_TIME, node);
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiDisablePullup(void)
    {
        VWrite(PULLUP, 0, ADVANCE_TIME, node);
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiReset()
    {
        suspended = false;
    }

    //-------------------------------------------------------------
    //-------------------------------------------------------------

    void apiHaltSimulation()
    {
        VWrite(UVH_FINISH, 0, 0, node);
    }

private:

    int  node;
    bool suspended;

};

#endif
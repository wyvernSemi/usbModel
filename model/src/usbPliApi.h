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
// VProc API
#include "VUser.h"
}

#ifndef _USB_PLI_API_H_
#define _USB_PLI_API_H_

class usbPliApi
{
public:
    static const int major_ver       = 1;
    static const int minor_ver       = 1;
    static const int patch_ver       = 2;
    
    static const int ONE_US          = 12;
    static const int ONE_MS          = ONE_US * 1000;

    static const int IS_HOST         = false;
    static const int IS_DEVICE       = true;


#ifndef USBTESTMODE
    static const int MINRSTCOUNT     = ONE_MS * 10;  // 10ms for fullspeed device
    static const int MINSUSPENDCOUNT = ONE_MS * 3;   // 3ms for fullspeed device
#else
    static const int MINRSTCOUNT     = ONE_US * 25;
    static const int MINSUSPENDCOUNT = ONE_US * 100;
#endif

private:

    static const int IDLE_FOREVER = 0;
    static const int ADVANCE_TIME = 0;
    static const int MINIMUMIDLE  = 1;

public:

    //-------------------------------------------------------------
    // Constructor
    //
    // Must give a node number which must be unique for all usbDevice
    // and usbHost objects and match the NODENUM parameter of the
    // usbModel instantiation in the verilog that it's meant to drive.
    //
    // An option string argument can be supplied to use in formatted
    // output.
    //
    //-------------------------------------------------------------

    usbPliApi(const int nodeIn, std::string name = std::string("DEV ")) : node(nodeIn)
    {
    }
    
    void usbGetVersionStr(char *vstr, unsigned len = 12)
    {
        snprintf(vstr, len, "%d.%d.%d", major_ver, minor_ver, patch_ver);
    }

protected:

    //-------------------------------------------------------------
    // apiSendIdle
    //
    // Advance simulation time for specified number of clock ticks
    // (default 1) whilst drive the line at the idle state
    // (OE inactive).
    //
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
    // apiSendReset
    //
    // Advance simulation time for specified number of clock ticks
    // (default 1) whilst drive the line at the SE0 state
    // (OE active).
    //
    //-------------------------------------------------------------

    void apiSendReset (const unsigned ticks = 1)
    {
        unsigned time;
        unsigned currtime;

        // Sample Current clock count
        time = apiGetClkCount();
        // Enable outputs
        VWrite(OUTEN, 1, DELTA_CYCLE, node);

        // Set the line to SE0
        VWrite(LINE, 0, ADVANCE_TIME, node);

        // Keep reading clock count for 'ticks' number of cycles
        do {
            currtime = apiGetClkCount(ADVANCE_TIME);
        } while ((ticks == IDLE_FOREVER) || ((currtime-time) < ticks));

        // Disable outputs
        VWrite(OUTEN, 0, DELTA_CYCLE, node);
    }

    //-------------------------------------------------------------
    // apiWaitOnNotReset
    //
    // Polls the state of the nreset input port of usbModel module
    // until it becomes inactive.
    //
    //-------------------------------------------------------------

    void apiWaitOnNotReset(void)
    {
        unsigned reset;

        do {
            VRead(RESET_STATE, &reset, ADVANCE_TIME, node);
        } while (reset);
    }

    //-------------------------------------------------------------
    // usbEnablePullup
    //
    // Enables the pullup(s) on the usbModel module instantiation.
    // The default of this at time 0 is dependent on whether
    // configured as a host (default enabled) or a device (default
    // disabled). Enabling on a device can be seen by the host
    // a device connection, and disabling as a diconnection.
    //
    //-------------------------------------------------------------

    void apiEnablePullup(void)
    {
        VWrite(PULLUP, 1, ADVANCE_TIME, node);
    }

    //-------------------------------------------------------------
    // usbDisablePullup
    //
    // Diables the pullup(s) on the usbModel module instantiation.
    // The default of this at time 0 is dependent on whether
    // configured as a host (default enabled) or a device (default
    // disabled). Enabling on a device can be seen by the host
    // a device connection, and disabling as a diconnection.
    //
    //-------------------------------------------------------------

    void apiDisablePullup(void)
    {
        VWrite(PULLUP, 0, ADVANCE_TIME, node);
    }

    //-------------------------------------------------------------
    // apiHaltSimulation
    //
    // Halts the simulation.
    //
    // When run in batch mode this will call $finish in the usbModel
    // model and $stop when in GUI mode.
    //
    //-------------------------------------------------------------

    void apiHaltSimulation()
    {
        VWrite(UVH_FINISH, 0, 0, node);
    }

    //-------------------------------------------------------------
    // apiGetClkCount
    //
    // Returns the value of the clkcount register in the verilog
    // which increments from time 0 one per system clock.
    //
    // An optional delta argument allows simuation time to advance
    // If set to DELTA_CYCLE (default), simulation time is not
    // advanced. If set to 0, time is advanced.
    //
    //-------------------------------------------------------------

    unsigned apiGetClkCount(int delta = DELTA_CYCLE)
    {
        unsigned clkCount;

        VRead(CLKCOUNT, &clkCount, delta, node);

        return clkCount;
    }

    //-------------------------------------------------------------
    // apiReset
    //
    // Externally called method to force intenal state to be reset
    // separate from a USB reset condition.
    //
    //-------------------------------------------------------------

    void apiReset()
    {
        suspended = false;
    }

    //-------------------------------------------------------------
    // apiReadLineState
    //
    // Returns the state of the USB line as a two bits in an
    // unsigned number with D+ in bit 0 an D- in bit 1.
    //
    //-------------------------------------------------------------

    unsigned apiReadLineState(const int delta = DELTA_CYCLE)
    {
        unsigned rawline;

        VRead(LINE, &rawline, delta, node);

        return rawline;
    }

    //-------------------------------------------------------------
    // apiSendPacket
    //
    // Sends an NRZI encoded packet (nrzi[]) over the USB interface
    // for the specified number of bits (bitlen). An idle period
    // is generated first as specified by delay. The output enable
    // is activated when sending the packet and deactivated when
    // complete.
    //
    //-------------------------------------------------------------

    void apiSendPacket(const usbModel::usb_signal_t nrzi[], const int bitlen, const int delay = 50)
    {
        // Idle the bus for a time
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
    // the calling code. Will detect suspension (idle for a minimum
    // period) and will timeout if a period specified (time > 0).
    // The method also monitors for disconnction (SE0 when idle).
    //
    // The possible return values are:
    //
    //   A bit count (return integer >= 0)
    //   usbModel::DISCONNECTED
    //   usbModel::USBRESET
    //   usbModel::USBSUSPEND
    //   usbModel::USBNORESPONSE
    //   usbModel::USBERROR
    //
    //-------------------------------------------------------------

    int apiWaitForPkt(usbModel::usb_signal_t nrzi[], const bool isDevice = true, const unsigned timeout = 0)
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
            line = apiReadLineState(0);

            // If a host and SE0 seen when idle, there is no device connected
            if (!isDevice)
            {
                if (idle && line == usbModel::USB_SE0)
                {
                    return usbModel::USBDISCONNECTED;
                }
            }

            // If anything other than a J (when already idle) come out of suspend
            if (suspended && (line == usbModel::USB_K || (isDevice && line == usbModel::USB_SE0)))
            {
                USBDISPPKT("Device activated from suspension\n");
                suspended = false;
            }

            // If not in the middle of a reset detection and K seen, then line is not idle
            if (!lookforreset && line == usbModel::USB_K)
            {
                idle = false;
            }
            // If idle and an SE0 seen then this may be a reset
            else if (isDevice && idle && line == usbModel::USB_SE0)
            {
                idle         = false;
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
                    // else return an error for unexplained SE0s
                    if (rstcount >= MINRSTCOUNT)
                    {
                        return usbModel::USBRESET;
                    }
                    else
                    {
                        return usbModel::USBERROR;
                    }

                    rstcount = 0;
                    lookforreset = false;
                    idle         = (line == usbModel::USB_K) ? false : true;
                }
            }

            // If not idle, then process the packet data
            if (!idle && !lookforreset)
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
                idlecount++;
                if (isDevice && idlecount >= MINSUSPENDCOUNT)
                {
                    suspended = true;
                    return usbModel::USBSUSPEND;
                }

                if (timeout != usbModel::NOTIMEOUT && idlecount >= timeout)
                {
                    return usbModel::USBNORESPONSE;
                }

            }
        } while (true);

        // Return the bitcount of the packet
        return bitcount;
    }

private:

    //-------------------------------------------------------------
    // Internal private state.
    //-------------------------------------------------------------

    // Node number of VProc module for this API object
    int  node;

    // Suspended state
    bool suspended;

};

#endif